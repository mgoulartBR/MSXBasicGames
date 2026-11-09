/* V9990 P1 renderer (KAW_P1).
 *
 * Layer B  = the world: 64x64 names (32x32 cells of 16x16 art), hardware scroll. Fog is the choice of pattern
 *            (1 = unknown, PB_PLAIN + 4*tile = visible, PB_DITH + 4*tile = remembered); see p1_cell().
 * Layer A  = rows 0..19: a constant 1:1 map onto a 256x160 bitmap that holds the drag box, building bars and the
 *            placement ghost (rects painted with LMMV); rows 20..26: HUD text and frame.
 * Sprites  = actors (pose blocks), health bars, arrows, selection rings, building occluders, mouse pointer.
 *
 * VRAM (a byte address is 128 * line + x/2 when viewed as the 256 px wide 4bpp bitmap the VDP commands use):
 *   lines    0..159   layer A pattern table: the box/bars bitmap
 *   lines  160..183   layer A HUD patterns: solid colours (PA_SOLID), HUD top line, font (PA_FONT)
 *   lines  192..575   pose bank, 32 px poses (8 per row)
 *   lines  576..1295  pose bank, 48 px poses (5 per row)
 *   lines 1296..1439  building tile art without grass, 16x16 cells (occlusion), index = tile - 50
 *   lines 1792..2047  sprite pattern table (R#25 = 0x38000): 256 cells of 16x16
 *        cells   0..191   pose blocks, two 3x3 blocks per actor (double buffer)
 *        cells 192..209   unit health bars (9 widths x 2 colours), 210..213 arrows, 214..222 white bars,
 *        cell  223        damage cross, 224..237 occluder slots, 238 pointer, 239 selection ring
 *        (cell row 15 shares lines 252..255 with the attribute table: do not use)
 *   0x3FE00           sprite attribute table (SPAT)
 *   0x40000           layer B pattern table
 *   0x7C000 / 0x7E000 layer A / layer B names
 *
 * The vertical-blank handler (irq_p1) applies scroll, the sprite list and the rectangle list together, so a frame
 * never shows a half-updated state. Main-thread VRAM access that uses the write address runs with interrupts off.
 */
#define P1_SGT 0x38000UL
#define P1_PGT_B_BASE 0x40000UL
#define P1_SGT_LINE 1792U
#define POSE32_Y 192U
#define POSE48_Y 576U
#define PB_PLAIN 8U
#define PB_DITH 800U
#define PA_SOLID 641U
#define PA_FONT 672U
#define PA_HUDTOP 660U
const u16 render_bit[RN]={1,2,4,8,16,32,64,128,256,512};
u8 tile_buffer[128],boot_poses,cache_tick;
u16 frame_uploads,rendered_frames;
i16 actor_x[RN],actor_y[RN];u8 actor_visible[RN],actor_key[RN];
volatile struct {u16 r[6];u8 arg,lop;} v9c;
static u8 v9_busy(void) __naked {__asm
 in a,(#0x65)
 and #1
 ret
 __endasm;}
static void v9_lmmm(void) __naked {__asm
 di
 ld a,#32
 out (#0x64),a
 ld hl,#_v9c
 ld bc,#0x0E63
 otir
 ld a,#52
 out (#0x64),a
 ld a,#0x40
 out (#0x63),a
 ei
 ret
 __endasm;}
static void gpu_wait(void) {if(*((volatile u8*)0xFC9E)!=(u8)raw_tick)poll_input();while(v9_busy())if(video_enabled && *((volatile u8*)0xFC9E)!=(u8)raw_tick)poll_input();}
/* Rectangle copy inside VRAM seen as a 256 px wide bitmap. */
static void p1_copy(u16 sx,u16 sy,u16 dx,u16 dy,u16 w,u16 h) {gpu_wait();v9c.r[0]=sx;v9c.r[1]=sy;v9c.r[2]=dx;v9c.r[3]=dy;v9c.r[4]=w;v9c.r[5]=h;v9c.arg=0;v9c.lop=0x0C;v9_lmmm();}
u16 video_clock(void) {u16 t;__asm di __endasm;t=*((volatile u16*)0xFC9E);__asm ei __endasm;return t;}
void video_hud_swap(void) {if(hud_pending) {hud_live=hud_ready;hud_pending=0;++hud_swaps;sprites_dirty|=1;}}
void video_wait(void) {video_hud_swap();if(video_clock()!=last_time)return;while(V9_IsVBlank()){}while(!V9_IsVBlank()){};}
void video_hud_begin(void) {}
void video_hud_end(void) {}
void video_hud_flush(void) {gpu_wait();}
void video_camera(void) {sprites_dirty|=1;}
u8 video_camera_ready(u16 y) {y;return 1;}
void video_stream(void) {}
/* ---- pattern memory ---- */
static void pat_write(u32 base,u16 n,const u8* src) {
 u8 k;
 for(k=0;k<8;++k)V9_WriteVRAM(base+((u32)(n>>5)<<10)+((u16)k<<7)+((n&31)<<2),src+(k<<2),4);
}
/* A 16x16 tile (16 rows of 8 bytes) as four 8x8 patterns: TL, TR, BL, BR. */
static void tile_patterns(u32 base,u16 n,const u8* t) {
 u8 pat[32],q,k;
 for(q=0;q<4;++q) {
  for(k=0;k<8;++k) {const u8* r=t+(u16)(((q>>1)<<3)+k)*8+((q&1)<<2);pat[k*4]=r[0];pat[k*4+1]=r[1];pat[k*4+2]=r[2];pat[k*4+3]=r[3];}
  pat_write(base,n+q,pat);
 }
}
/* ---- names ---- */
#define PNT_A 0x7C000UL
#define PNT_B 0x7E000UL
__sfr __at(0x60) io_vram;
__sfr __at(0x63) io_reg;
__sfr __at(0x64) io_sel;
__sfr __at(0x65) io_stat;
__sfr __at(0x66) io_int;
/* Address + data sequences below run with interrupts off: the vertical-blank handler also uses the VRAM address. */
static void io_vset(u32 a) {io_sel=0;io_reg=(u8)a;io_reg=(u8)(a>>8);io_reg=(u8)(a>>16);}
static void io_rw(u8 r,u8 v) {io_sel=r;io_reg=v;}
static void names_put(u32 a,u16 n0,u16 n1,u16 n2,u16 n3) {
 __asm di __endasm;
 io_vset(a);io_vram=(u8)n0;io_vram=(u8)(n0>>8);io_vram=(u8)n1;io_vram=(u8)(n1>>8);
 io_vset(a+128);io_vram=(u8)n2;io_vram=(u8)(n2>>8);io_vram=(u8)n3;io_vram=(u8)(n3>>8);
 __asm ei __endasm;
}
static void p1_cell(u8 x,u8 y) {
 u8 st=fog_at(x,y);u16 n;u32 a;
 a=PNT_B+((u16)y<<8)+((u16)x<<2);
 if(!st) {names_put(a,1,1,1,1);return;}
 n=(st==1?PB_DITH:PB_PLAIN)+(u16)tile_source[fog_art(x,y)]*4;
 names_put(a,n,n+1,n+2,n+3);
}
void video_world_tile(u8 x,u8 y) {p1_cell(x,y);}
void video_known_tile(u8 x,u8 y) {p1_cell(x,y);sprites_dirty|=1;}
void video_tile(u8 x,u8 y) {
 u16 cell=((u16)y<<5)+x;if(x>=32 || y>=32 || fog_at(x,y)!=2)return;fog_memory[cell]=art_map[cell];video_known_tile(x,y);
}
/* ---- HUD (layer A) ---- */
static void hud_name(u8 col,u8 row,u16 n) {__asm di __endasm;io_vset(PNT_A+((u16)row<<7)+((u16)col<<1));io_vram=(u8)n;io_vram=(u8)(n>>8);__asm ei __endasm;}
void video_char(u8 x,u16 y,u8 c) {hud_name(x>>3,(u8)(y>>3),PA_FONT+(u8)(c-32));}
void video_text(u8 x,u16 y,const c8* s) {while(*s && x<249){video_char(x,y,*s++);x+=8;}}
void video_fill(u16 x,u16 y,u16 w,u16 h,u8 c) {
 u8 col,row;u16 n=h<8?PA_HUDTOP:PA_SOLID+c;
 if(video_enabled)++hud_erase_count;
 for(row=y>>3;row<=(u8)((y+h-1)>>3);++row)for(col=x>>3;col<=(u8)((x+w-1)>>3);++col)hud_name(col,row,n);
}
/* ---- sprites ---- */
/* Pose slots are double buffered: a new pose is copied into the idle block and shown only once the copy ended,
 * so a half-copied pose is never on screen. Attributes and scroll are committed in the vertical blank. */
static u8 disp_key[RN],pend_key[RN],act_blk[RN],sused;
#define sattr p1_shared
static u16 p1_bx,p1_by;static u8 p1_ready,p1_n,r22;
#define MAXR 20
/* Screen-space rectangles painted into the layer A bitmap (drag box, building bars and banners): the pending
 * list is compared with the committed one and repainted in the vertical blank, never half drawn. */
static u8 rw_x[MAXR],rw_y[MAXR],rw_w[MAXR],rw_h[MAXR],rw_c[MAXR],rw_n,rw_lock,rw_dirty;
static u8 rc_x[MAXR],rc_y[MAXR],rc_w[MAXR],rc_h[MAXR],rc_c[MAXR],rc_n;
static void io_lmmv(u8 x,u8 y,u8 w,u8 h,u8 c) {
 while(io_stat&1);
 io_sel=36;io_reg=x;io_reg=0;io_reg=y;io_reg=0;io_reg=w;io_reg=0;io_reg=h;io_reg=0;
 io_reg=0;io_reg=0x0C;io_reg=0xFF;io_reg=0xFF;io_reg=c*17;io_reg=c*17;
 io_sel=52;io_reg=0x20;
}
static void rl_add(i16 x,i16 y,u8 w,u8 h,u8 c) {
 i16 x1=x+w,y1=y+h;
 if(x<0)x=0;if(y<0)y=0;if(x1>256)x1=256;if(y1>160)y1=160;
 if(x1<=x || y1<=y || rw_n>=MAXR)return;
 rw_x[rw_n]=x;rw_y[rw_n]=y;rw_w[rw_n]=(x1-x>255)?255:(u8)(x1-x);rw_h[rw_n]=(u8)(y1-y);rw_c[rw_n]=c;++rw_n;
}
#define SPAT 0x3FE00UL
static const u8 cell_tab[2][RN]={{0,3,6,9,12,96,99,102,105,108},{48,51,54,57,60,144,147,150,153,156}};
#define CELL_POSE(i,b) cell_tab[b][i]
#define CELL_PTR 238
#define CELL_RING 239
#define CELL_BAR_ALLY 192
#define CELL_BAR_ENEMY 201
#define CELL_ARROW 210
#define CELL_BAR_WHITE 214
#define CELL_CROSS 223
#define CELL_OCC 224
#define OCC_N 14
#define OV_Y 1296U
static void pose_pos(u8 kind,u8 f,u16* x,u16* y) {
 if(kind_sizes[kind]==32) {u8 k=(kind==0?0:kind==1?1:kind==3?2:3)*24+f;*x=(u16)(k&7)*32;*y=POSE32_Y+(u16)(k>>3)*32;}
 else {u8 k=(kind==2?0:kind==4?1:2)*24+f,q=(u8)(((u16)k*205)>>10);*x=(u16)(k-q*5)*48;*y=POSE48_Y+(u16)q*48;}
}
static void px4(u8* buf,u8 x,u8 y,u8 c) {u8* p=buf+((u16)y<<3)+(x>>1);if(x&1)*p=(*p&0xF0)|c;else *p=(*p&0x0F)|(c<<4);}
static void cell_write(u8 cell,const u8* src16rows) {
 u8 k;for(k=0;k<16;++k)V9_WriteVRAM(P1_SGT+((u32)((cell>>4)<<4)+k)*128+((cell&15)<<3),src16rows+(k<<3),8);
}
static void otir_vram(const u8* p,u16 n) __naked {p;n;__asm
 ld b,e
 dec de
 inc d
 ld c,#0x60
otv_loop:
 otir
 dec d
 jp nz,otv_loop
 ret
 __endasm;}
/* Vertical blank: the pointer follows the mouse every frame; a finished sprite list and its scroll go together. */
static void irq_p1(void) {
 if(io_int&1) {
  io_int=1;*((volatile u8*)0xF3F6)=255;
  io_vset(SPAT);io_vram=(u8)(mouse_y-1);io_vram=CELL_PTR;io_vram=mouse_x;io_vram=0;
  if(p1_ready) {
   io_rw(21,(u8)p1_by);io_rw(22,r22|(u8)((p1_by>>8)&1));io_rw(23,(u8)(p1_bx&7));io_rw(24,(u8)(p1_bx>>3));
   if(p1_n>1) {io_vset(SPAT+4);otir_vram(sattr+4,(u16)(p1_n-1)*4);}
   p1_ready=0;
  }
  if(rw_dirty && !rw_lock && !(io_stat&1)) {
   u8 i,same=(rw_n==rc_n);
   for(i=0;same && i<rc_n;++i)if(rw_x[i]!=rc_x[i] || rw_y[i]!=rc_y[i] || rw_w[i]!=rc_w[i] || rw_h[i]!=rc_h[i] || rw_c[i]!=rc_c[i])same=0;
   if(!same) {
    for(i=0;i<rc_n;++i)io_lmmv(rc_x[i],rc_y[i],rc_w[i],rc_h[i],0);
    for(i=0;i<rw_n;++i) {io_lmmv(rw_x[i],rw_y[i],rw_w[i],rw_h[i],rw_c[i]);rc_x[i]=rw_x[i];rc_y[i]=rw_y[i];rc_w[i]=rw_w[i];rc_h[i]=rw_h[i];rc_c[i]=rw_c[i];}
    rc_n=rw_n;
   }
   rw_dirty=0;
  }
 }
}
void video_irq_install(void) {
 __asm di __endasm;
 r22=V9_GetRegister(22)&0xFE;
 BIOS_SetHookDirectCallback(H_KEYI,irq_p1);
 V9_SetInterrupt(V9_INT_VBLANK);
 __asm ei __endasm;
}
void video_cursor(void) {}
/* Z80: write the n x n sprite attributes (Y, pattern, X, info) of one actor block. */
static u16 e_o;static i16 e_x,e_y,e_bx,e_by;static u8 e_cell,e_n,e_rows,e_bc,e_r,e_c;static u8 bb_off;
static void emit_block(void) __naked {
 __asm
 ld hl,(_e_o)
 ld a,(_e_by)
 dec a
 ld d,a
 ld a,(_e_bc)
 ld e,a
 ld a,(_e_rows)
 ld (_e_r),a
eb_row:
 push de
 ld bc,(_e_bx)
 ld a,(_e_n)
 ex af,af'
eb_col:
 ld (hl),d
 inc hl
 ld (hl),e
 inc hl
 ld (hl),c
 inc hl
 ld a,b
 and a,#3
 or a,#0x20
 ld (hl),a
 inc hl
 inc e
 ld a,c
 add a,#16
 ld c,a
 jr nc,eb_nc
 inc b
eb_nc:
 ex af,af'
 dec a
 jr z,eb_cd
 ex af,af'
 jr eb_col
eb_cd:
 pop de
 ld a,d
 add a,#16
 ld d,a
 ld a,e
 add a,#16
 ld e,a
 ld a,(_e_r)
 dec a
 ld (_e_r),a
 jp nz,eb_row
 ld (_e_o),hl
 ret
 __endasm;
}
u16 sc_cull;static u16 sc_bitw;
static void sp_cull(void) __naked {
 __asm
 ld hl,#0
 ld (_sc_cull),hl
 inc l
 ld (_sc_bitw),hl
 push ix
 ld ix,#_units
 ld c,#0
cl_loop:
 ld b,#0
 ld hl,#_unit_hp
 add hl,bc
 ld a,(hl)
 or a,a
 jp z,cl_cull
 ld a,c
 cp a,#8
 jp nc,cl_alive
 ld a,(_unit_count)
 ld e,a
 ld a,c
 cp a,e
 jp nc,cl_cull
cl_alive:
 ld l,0(ix)
 ld h,1(ix)
 ld de,(_cam_x)
 or a,a
 sbc hl,de
 ld de,#56
 add hl,de
 ld de,#345
 or a,a
 sbc hl,de
 jp nc,cl_cull
 ld l,2(ix)
 ld h,3(ix)
 ld de,(_cam_y)
 or a,a
 sbc hl,de
 ld de,#32
 add hl,de
 ld de,#241
 or a,a
 sbc hl,de
 jp nc,cl_cull
 jp cl_next
cl_cull:
 ld hl,(_sc_cull)
 ld de,(_sc_bitw)
 ld a,l
 or a,e
 ld l,a
 ld a,h
 or a,d
 ld h,a
 ld (_sc_cull),hl
cl_next:
 ld de,#16
 add ix,de
 ld hl,(_sc_bitw)
 add hl,hl
 ld (_sc_bitw),hl
 inc c
 ld a,c
 cp a,#10
 jp nz,cl_loop
 pop ix
 ret
 __endasm;
}
#define SPR(X,Y,P,I) do{i16 _x=(X);*o++=(u8)((Y)-1);*o++=(P);*o++=(u8)_x;*o++=((u8)(_x>>8)&3)|(I);}while(0)
static u8 occ_key[OCC_N],occ_stamp[OCC_N],occ_pend[OCC_N];static i16 ob_l[8],ob_t[8],ob_r[8],ob_b[8],ub_l,ub_t,ub_r,ub_b;
/* Occlusion: building tiles (opaque pixels only) become sprites in front of a unit that stands behind the
 * building. Tile art lives in a bitmap bank; a slot is copied into a sprite cell the first time it is needed. */
static u8 occ_get(u8 idx) {
 u8 s,best=255,age=0;
 for(s=0;s<OCC_N;++s)if(occ_key[s]==idx) {occ_stamp[s]=cache_tick;return occ_pend[s]?255:s;}
 for(s=0;s<OCC_N;++s)if(occ_stamp[s]!=cache_tick) {u8 a=(u8)(cache_tick-occ_stamp[s]);if(best==255 || a>age){best=s;age=a;}}
 if(best==255)return 255;
 {u8 c=CELL_OCC+best;p1_copy((u16)(idx&15)<<4,OV_Y+((u16)(idx>>4)<<4),(u16)(c&15)<<4,P1_SGT_LINE+((u16)(c>>4)<<4),16,16);}
 occ_key[best]=idx;occ_pend[best]=1;occ_stamp[best]=cache_tick;return 255;
}
static u16 bv_epoch;static u8 bv_state[BMAX],bv_x[BMAX],bv_y[BMAX];
/* fog_target for a building, cached per fog epoch: the sight only changes when fog_epoch does. */
static u8 bld_visible(u8 k) {
 u8 x=buildings[k].x,y=buildings[k].y,n,dx,dy,r=1;
 if(bv_epoch!=fog_epoch){for(n=0;n<BMAX;++n)bv_state[n]=0;bv_epoch=fog_epoch;}
 if(bv_state[k] && bv_x[k]==x && bv_y[k]==y)return bv_state[k]==2;
 n=building_size(buildings[k].type);
 for(dy=0;dy<n && r==1;++dy)for(dx=0;dx<n;++dx)if(fog_at(x+dx,y+dy)==2){r=2;break;}
 bv_state[k]=r;bv_x[k]=x;bv_y[k]=y;return r==2;
}
static u8 bw_hp[BMAX],bw_w[BMAX],bw_mx[BMAX];
static u8 bl_x[MAXR],bl_y[MAXR],bl_w[MAXR],bl_h[MAXR],bl_c[MAXR],bl_n,bl_ok,bl_r0,bl_do,bl_bc;static u16 bl_cx,bl_cy,bl_ep,bl_hs;static u16 bar_sig[RN];static u8 bar_w[RN];static u8 ord_prev[RN],ord_n;

static u8 ord_cur[RN],os_cnt,os_j,os_i,os_ko;static u16 os_kv;
/* Visible actors in front-to-back order: last frame's order first (nearly sorted), then new arrivals,
 * then an insertion sort on the feet line (larger py first). Returns the count; result in ord_cur. */
static u8 ord_sort(void) __naked {
 __asm
 xor a,a
 ld (_os_cnt),a
 ld (_os_j),a
os_l1:
 ld a,(_os_j)
 ld hl,#_ord_n
 cp a,(hl)
 jp nc,os_p2
 ld e,a
 ld d,#0
 ld hl,#_ord_prev
 add hl,de
 ld e,(hl)
 ld hl,#_actor_visible
 add hl,de
 ld a,(hl)
 cp a,#1
 jp nz,os_n1
 ld (hl),#2
 call os_append
os_n1:
 ld hl,#_os_j
 inc (hl)
 jp os_l1
os_p2:
 xor a,a
 ld (_os_j),a
os_l2:
 ld a,(_os_j)
 cp a,#10
 jp nc,os_sort
 ld e,a
 ld d,#0
 ld hl,#_actor_visible
 add hl,de
 ld a,(hl)
 cp a,#1
 jp nz,os_n2
 ld (hl),#2
 call os_append
os_n2:
 ld hl,#_os_j
 inc (hl)
 jp os_l2
os_append:
 ld a,(_os_cnt)
 ld c,a
 ld b,#0
 ld hl,#_ord_cur
 add hl,bc
 ld (hl),e
 inc a
 ld (_os_cnt),a
 ret
os_py:
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,#_units+2
 add hl,de
 ld e,(hl)
 inc hl
 ld d,(hl)
 ex de,hl
 ret
os_sort:
 ld a,(_os_cnt)
 cp a,#2
 jp c,os_done
 ld a,(_ord_cur)
 call os_py
 ld (_os_kv),hl
 ld a,#1
 ld (_os_i),a
os_ck:
 ld a,(_os_i)
 ld hl,#_os_cnt
 cp a,(hl)
 jp nc,os_done
 ld c,a
 ld b,#0
 ld hl,#_ord_cur
 add hl,bc
 ld a,(hl)
 call os_py
 ld de,(_os_kv)
 ex de,hl
 or a,a
 sbc hl,de
 jp c,os_full
 ld (_os_kv),de
 ld hl,#_os_i
 inc (hl)
 jp os_ck
os_full:
 ld a,#1
 ld (_os_i),a
os_si:
 ld a,(_os_i)
 ld hl,#_os_cnt
 cp a,(hl)
 jp nc,os_done
 ld c,a
 ld b,#0
 ld hl,#_ord_cur
 add hl,bc
 ld a,(hl)
 ld (_os_ko),a
 call os_py
 ld (_os_kv),hl
 ld a,(_os_i)
 ld (_os_j),a
os_sj:
 ld a,(_os_j)
 or a,a
 jp z,os_place
 dec a
 ld c,a
 ld b,#0
 ld hl,#_ord_cur
 add hl,bc
 ld a,(hl)
 call os_py
 ld de,(_os_kv)
 or a,a
 sbc hl,de
 jp nc,os_place
 ld a,(_os_j)
 ld c,a
 ld b,#0
 ld hl,#_ord_cur
 add hl,bc
 dec hl
 ld a,(hl)
 inc hl
 ld (hl),a
 ld hl,#_os_j
 dec (hl)
 jp os_sj
os_place:
 ld a,(_os_j)
 ld c,a
 ld b,#0
 ld hl,#_ord_cur
 add hl,bc
 ld a,(_os_ko)
 ld (hl),a
 ld hl,#_os_i
 inc (hl)
 jp os_si
os_done:
 ld a,(_os_cnt)
 ld (_ord_n),a
 or a,a
 jp z,os_fix
 ld c,a
 ld b,#0
 ld hl,#_ord_cur
 ld de,#_ord_prev
 ldir
os_fix:
 ld hl,#_actor_visible
 ld b,#10
os_fl:
 ld a,(hl)
 cp a,#2
 jp nz,os_fn
 ld (hl),#1
os_fn:
 inc hl
 djnz os_fl
 ld a,(_os_cnt)
 ret
 __endasm;
}

u8 draw_opt;
static u8 ae_nb,ae_j,ae_i,ae_sz;static const u8 cell_ring_v=CELL_RING;
void ae_up(u8 i) {
 u8 sz=kind_sizes[unit_kind[i]],key=actor_key[i],c2=CELL_POSE(i,act_blk[i]^1);u16 px,py;
 pose_pos(unit_kind[i],key-unit_kind[i]*24,&px,&py);p1_copy(px,py,(u16)(c2&15)<<4,P1_SGT_LINE+((u16)(c2>>4)<<4),sz,sz);
 pend_key[i]=key;++frame_uploads;
}
void ae_occ(u8 i) {
 u8 sz=kind_sizes[unit_kind[i]],nb=ae_nb,q;u8* o=(u8*)e_o;
 if(actor_x[i]+(i16)sz>ub_l && actor_x[i]<ub_r && actor_y[i]+(i16)sz>ub_t && actor_y[i]<ub_b) {
  const u8* pb=pose_bounds+((u16)actor_key[i]<<2);
  i16 rl=actor_x[i]+pb[0],rt=actor_y[i]+pb[1],rr=rl+pb[2],rb=rt+pb[3];u16 foot=units[i].py+16;
  for(q=0;q<nb;++q) {
   u8 cx,cy,cx0,cx1,cy0,cy1;i16 l2,t2,r2,b2;
   if(rr<=ob_l[q] || rl>=ob_r[q] || rb<=ob_t[q] || rt>=ob_b[q] || (u16)ob_b[q]<foot)continue;
   l2=ob_l[q]>rl?ob_l[q]:rl;t2=ob_t[q]>rt?ob_t[q]:rt;r2=ob_r[q]<rr?ob_r[q]:rr;b2=ob_b[q]<rb?ob_b[q]:rb;
   cx0=l2>>4;cx1=(r2-1)>>4;cy0=t2>>4;cy1=(b2-1)>>4;
   for(cy=cy0;cy<=cy1;++cy)for(cx=cx0;cx<=cx1;++cx) {
    u8 t,s2;
    if(fog_at(cx,cy)!=2)continue;
    t=fog_art(cx,cy);if(t<64)continue;
    s2=occ_get(tile_source[t]-50);
    if(s2!=255 && o<sattr+496)SPR(((i16)cx<<4)-(i16)cam_x,((i16)cy<<4)-(i16)cam_y,CELL_OCC+s2,0x20);
   }
  }
 }
 e_o=(u16)o;
}
void bar_calc(u8 i) {
 bar_sig[i]=unit_hp[i]|((u16)unit_kind[i]<<8);
 {u8 w=(u8)(((u8)((u16)unit_hp[i]*16/unit_max_hp[unit_kind[i]])+1)>>1);bar_w[i]=w>8?8:w;}
}
static u8 ab_i,ab_w,ab_y;static u16 ab_x;
static const u8 cbw=CELL_BAR_WHITE,ccross=CELL_CROSS,cba=CELL_BAR_ALLY,cbe=CELL_BAR_ENEMY;
/* Z80: life bar of the actor in A (one sprite, or bar + cross while the hurt flash is on). */
static void ae_bar(u8 i) __naked {
 i;
 __asm
 ld (_ab_i),a
 ld hl,(_e_o)
 ld de,#_p1_shared+496
 or a,a
 sbc hl,de
 ret nc
 ld a,(_ab_i)
 ld e,a
 ld d,#0
 ld hl,#_unit_hp
 add hl,de
 ld b,(hl)
 ld hl,#_unit_kind
 add hl,de
 ld c,(hl)
 ld hl,#_bar_sig
 add hl,de
 add hl,de
 ld a,(hl)
 cp a,b
 jp nz,ab_re
 inc hl
 ld a,(hl)
 cp a,c
 jp z,ab_ok
ab_re:
 ld a,(_ab_i)
 call _bar_calc
ab_ok:
 ld a,(_ab_i)
 ld e,a
 ld d,#0
 ld hl,#_bar_w
 add hl,de
 ld a,(hl)
 ld (_ab_w),a
 ld a,(_ae_sz)
 rrca
 and a,#0x7F
 sub a,#8
 ld l,a
 ld h,#0
 ld bc,(_e_x)
 add hl,bc
 ld (_ab_x),hl
 ld hl,#_actor_key
 add hl,de
 ld l,(hl)
 ld h,#0
 add hl,hl
 add hl,hl
 inc hl
 ld bc,#_pose_bounds
 add hl,bc
 ld a,(hl)
 sub a,#5
 ld hl,#_e_y
 add a,(hl)
 ld (_ab_y),a
 ld hl,#_hurt_flash
 add hl,de
 ld a,(hl)
 cp a,#5
 jp c,ab_norm
 ld a,(_cbw)
 ld hl,#_ab_w
 add a,(hl)
 call ab_put
 ld a,(_ccross)
 jp ab_put
ab_norm:
 ld a,(_ab_i)
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,#_units+10
 add hl,de
 ld a,(hl)
 or a,a
 ld a,(_cba)
 jp z,ab_pa
 ld a,(_cbe)
ab_pa:
 ld hl,#_ab_w
 add a,(hl)
ab_put:
 ld hl,(_e_o)
 ld c,a
 ld a,(_ab_y)
 ld (hl),a
 inc hl
 ld (hl),c
 inc hl
 ld a,(_ab_x)
 ld (hl),a
 inc hl
 ld a,(_ab_x+1)
 and a,#3
 or a,#0x20
 ld (hl),a
 inc hl
 ld (_e_o),hl
 ret
 __endasm;
}
void ae_ring(u8 i) {
 u8* o=(u8*)e_o;u8 sz=kind_sizes[unit_kind[i]];i16 x=e_x,y=e_y;
 if(o<sattr+496)SPR(x+(sz>>1)-8,y+sz-16,CELL_RING,0x20);
 e_o=(u16)o;
}
/* Z80: the per-actor emission loop (pose upload, occluders, life bar, sprite block, selection ring). */
static void ae_run(void) __naked {
 __asm
 xor a,a
 ld (_ae_j),a
ae_lp:
 ld a,(_ae_j)
 ld hl,#_os_cnt
 cp a,(hl)
 jp nc,ae_end
 ld e,a
 ld d,#0
 ld hl,#_ord_cur
 add hl,de
 ld a,(hl)
 ld (_ae_i),a
 ld e,a
 ld hl,#_unit_kind
 add hl,de
 ld c,(hl)
 ld b,#0
 ld hl,#_kind_sizes
 add hl,bc
 ld a,(hl)
 ld (_ae_sz),a
 ld hl,#_actor_key
 add hl,de
 ld b,(hl)
 ld hl,#_disp_key
 add hl,de
 ld a,(hl)
 cp a,b
 jp z,ae_noup
 ld hl,#_pend_key
 add hl,de
 ld a,(hl)
 inc a
 jp nz,ae_noup
 ld a,(_ae_i)
 call _ae_up
ae_noup:
 ld a,(_ae_i)
 ld e,a
 ld d,#0
 ld hl,#_disp_key
 add hl,de
 ld a,(hl)
 inc a
 jp z,ae_next
 ld hl,#_actor_x
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld hl,(_cam_x)
 ld a,c
 sub a,l
 ld c,a
 ld a,b
 sbc a,h
 ld b,a
 ld (_e_x),bc
 ld hl,#_actor_y
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld hl,(_cam_y)
 ld a,c
 sub a,l
 ld c,a
 ld a,b
 sbc a,h
 ld b,a
 ld (_e_y),bc
 ld hl,#_act_blk
 add hl,de
 ld a,(hl)
 ld hl,#_cell_tab
 or a,a
 jp z,ae_c0
 ld bc,#10
 add hl,bc
ae_c0:
 add hl,de
 ld a,(hl)
 ld (_e_cell),a
 ld a,(_ae_nb)
 or a,a
 jp z,ae_noocc
 ld hl,#_actor_x
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld hl,(_ub_r)
 or a,a
 sbc hl,bc
 jp m,ae_noocc
 jp z,ae_noocc
 ld a,(_ae_sz)
 ld l,a
 ld h,#0
 add hl,bc
 ld de,(_ub_l)
 or a,a
 sbc hl,de
 jp m,ae_noocc
 jp z,ae_noocc
 ld a,(_ae_i)
 ld e,a
 ld d,#0
 ld hl,#_actor_y
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld hl,(_ub_b)
 or a,a
 sbc hl,bc
 jp m,ae_noocc
 jp z,ae_noocc
 ld a,(_ae_sz)
 ld l,a
 ld h,#0
 add hl,bc
 ld de,(_ub_t)
 or a,a
 sbc hl,de
 jp m,ae_noocc
 jp z,ae_noocc
 ld a,(_ae_i)
 call _ae_occ
ae_noocc:
 ld a,(_draw_opt)
 and a,#1
 jp nz,ae_nobar
 ld a,(_ae_i)
 ld hl,#_enemy_selected
 cp a,(hl)
 jp z,ae_dobar
 cp a,#8
 jp nc,ae_hpchk
 ld b,a
 ld a,#1
 inc b
ae_bs:
 dec b
 jp z,ae_bsd
 add a,a
 jp ae_bs
ae_bsd:
 ld hl,#_selection_mask
 and a,(hl)
 jp nz,ae_dobar
ae_hpchk:
 ld a,(_ae_i)
 ld e,a
 ld d,#0
 ld hl,#_unit_hp
 add hl,de
 ld b,(hl)
 ld hl,#_unit_kind
 add hl,de
 ld e,(hl)
 ld hl,#_unit_max_hp
 add hl,de
 ld a,b
 cp a,(hl)
 jp nc,ae_nobar
ae_dobar:
 ld a,(_ae_i)
 call _ae_bar
ae_nobar:
 ld a,(_ae_i)
 ld e,a
 ld d,#0
 ld hl,#_disp_key
 add hl,de
 ld l,(hl)
 ld h,#0
 add hl,hl
 ld de,#_pose_cells
 add hl,de
 ld a,(hl)
 ld (_bb_off),a
 inc hl
 ld b,(hl)
 ld a,b
 and a,#15
 ld (_e_rows),a
 ld a,b
 rrca
 rrca
 rrca
 rrca
 and a,#15
 ld (_e_n),a
 ld a,(_bb_off)
 ld hl,#_e_cell
 add a,(hl)
 ld (_e_bc),a
 ld a,(_bb_off)
 and a,#0xF0
 ld e,a
 ld d,#0
 ld hl,(_e_y)
 add hl,de
 ld (_e_by),hl
 ld a,(_bb_off)
 and a,#15
 add a,a
 add a,a
 add a,a
 add a,a
 ld e,a
 ld d,#0
 ld hl,(_e_x)
 add hl,de
 ld (_e_bx),hl
 ld a,(_e_n)
 ld b,a
 ld a,(_e_rows)
 ld c,a
 xor a,a
ae_mul:
 add a,c
 djnz ae_mul
 add a,a
 add a,a
 ld e,a
 ld d,#0
 ld hl,(_e_o)
 add hl,de
 ld de,#_p1_shared+500
 or a,a
 sbc hl,de
 jp c,ae_emit
 jp nz,ae_ring
ae_emit:
 call _emit_block
ae_ring:
 ld a,(_draw_opt)
 and a,#2
 jp nz,ae_next
 ld a,(_ae_i)
 cp a,#8
 jp nc,ae_next
 ld b,a
 ld a,#1
 inc b
ae_sh:
 dec b
 jp z,ae_shd
 add a,a
 jp ae_sh
ae_shd:
 ld hl,#_selection_mask
 and a,(hl)
 jp z,ae_next
 ld hl,(_e_o)
 ld de,#_p1_shared+496
 or a,a
 sbc hl,de
 jp nc,ae_next
 ld hl,(_e_o)
 ld a,(_ae_sz)
 sub a,#17
 ld de,#_e_y
 ex de,hl
 add a,(hl)
 ex de,hl
 ld (hl),a
 inc hl
 ld a,(_cell_ring_v)
 ld (hl),a
 inc hl
 push hl
 ld a,(_ae_sz)
 rrca
 and a,#0x7F
 sub a,#8
 ld e,a
 ld d,#0
 ld hl,(_e_x)
 add hl,de
 pop de
 ld a,l
 ld (de),a
 inc de
 ld a,h
 and a,#3
 or a,#0x20
 ld (de),a
 inc de
 ld (_e_o),de
ae_next:
 ld hl,#_ae_j
 inc (hl)
 jp ae_lp
ae_end:
 ret
 __endasm;
}
void video_sprites(void) {
 u8 count,i,j,k,n,nb;u8* o=sattr+4;
 ++cache_tick;
 /* copies issued earlier are finished once the engine is idle: switch those blocks in */
 if(!v9_busy()) {for(i=0;i<RN;++i)if(pend_key[i]!=NONE) {act_blk[i]^=1;disp_key[i]=pend_key[i];pend_key[i]=NONE;}for(i=0;i<OCC_N;++i)occ_pend[i]=0;}
 sp_cull();
 units_prep();
 count=ord_sort();
 __asm di __endasm;p1_ready=0;__asm ei __endasm;
 /* arrows: 5x5 px projectiles, in front of everything in the field */

 for(k=0;k<PS;++k)if(shot_time[k] && o<sattr+496) {
  i16 ax=shot_x[k],ay=shot_y[k];
  if(fog_at(ax>>4,ay>>4)!=2 || ax+2<(i16)cam_x || ax-2>=(i16)cam_x+256 || ay+2<(i16)cam_y || ay-2>=(i16)cam_y+160)continue;
  SPR(ax-8-cam_x,ay-8-cam_y,CELL_ARROW+(shot_dx[k]?(shot_dx[k]>0?0:1):(shot_dy[k]>0?2:3)),0x20);
 }

 nb=0;ub_l=ub_t=32767;ub_r=ub_b=-32767;
 for(k=0;k<building_count && nb<8;++k)if(buildings[k].state) {
  i16 bl=(i16)buildings[k].x<<4,bt=(i16)buildings[k].y<<4,sd=(i16)build_sizes[buildings[k].type]<<4;
  if(bl+sd<=(i16)cam_x || bl>=(i16)cam_x+256 || bt+sd<=(i16)cam_y || bt>=(i16)cam_y+160)continue;
  ob_l[nb]=bl;ob_t[nb]=bt;ob_r[nb]=bl+sd;ob_b[nb]=bt+sd;if(bl<ub_l)ub_l=bl;if(bt<ub_t)ub_t=bt;if(bl+sd>ub_r)ub_r=bl+sd;if(bt+sd>ub_b)ub_b=bt+sd;++nb;
 }
 e_o=(u16)o;ae_nb=nb;ae_run();o=(u8*)e_o;
 n=(u8)((u16)(o-sattr)>>2);
 __asm di __endasm;rw_lock=1;__asm ei __endasm;
 rw_n=0;
 {u8 box=drag_down&&drag_moved;u16 l=drag_l,t=drag_t,r=drag_r,b=drag_b;
  if(box && (r<cam_x || l>=cam_x+256 || b<cam_y || t>=cam_y+160))box=0;
  if(box) {
   if(l<cam_x)l=cam_x;if(r>=cam_x+256)r=cam_x+255;if(t<cam_y)t=cam_y;if(b>=cam_y+160)b=cam_y+159;
   if(r>l){rl_add(l-cam_x,t-cam_y,r-l,1,15);rl_add(l-cam_x,b-cam_y,r-l,1,15);}
   rl_add(l-cam_x,t-cam_y,1,b-t+1,15);rl_add(r-cam_x,t-cam_y,1,b-t+1,15);
  }
 }
 if(placement_mode && mouse_y<160) {
  u8 sz=building_size(build_kind)*16,c=place_valid?10:7;i16 gx=(i16)place_x*16-(i16)cam_x,gy=(i16)place_y*16-(i16)cam_y;
  rl_add(gx,gy,sz,1,c);rl_add(gx,gy+sz-1,sz,1,c);rl_add(gx,gy,1,sz,c);rl_add(gx+sz-1,gy,1,sz,c);
 }
{u8 rn0=rw_n;u16 hs=0;
 for(k=0;k<building_count;++k)hs+=(u16)building_hp[k]+((u16)buildings[k].state<<8)+building_owner[k]*7;
 if(bl_ok && bl_cx==cam_x && bl_cy==cam_y && bl_ep==fog_epoch && bl_hs==hs && bl_bc==building_count && bl_r0==rn0 && bl_do==(draw_opt&4)) {
  __builtin_memcpy(rw_x+rn0,bl_x,bl_n);__builtin_memcpy(rw_y+rn0,bl_y,bl_n);__builtin_memcpy(rw_w+rn0,bl_w,bl_n);__builtin_memcpy(rw_h+rn0,bl_h,bl_n);__builtin_memcpy(rw_c+rn0,bl_c,bl_n);rw_n=rn0+bl_n;
 } else {
 for(k=0;k<building_count;++k)if(!(draw_opt&4) && buildings[k].state && (building_owner[k] || building_hp[k]<(buildings[k].type==0?(building_owner[k]?180:240):96)) && bld_visible(k)) {
  i16 bx=(i16)buildings[k].x*16-(i16)cam_x,by=(i16)buildings[k].y*16-(i16)cam_y;u8 max=buildings[k].type==0?(building_owner[k]?180:240):96;
  if(building_owner[k] || building_hp[k]<max) {
   u8 w;if(bw_hp[k]!=building_hp[k] || bw_mx[k]!=max){bw_hp[k]=building_hp[k];bw_mx[k]=max;bw_w[k]=(u8)((u16)building_hp[k]*24/max);}w=bw_w[k];
   rl_add(bx+16,by-4,24,2,12);if(w)rl_add(bx+16,by-4,w,2,building_owner[k]?9:5);
  }
  if(building_owner[k]){rl_add(bx+17,by+1,1,10,11);rl_add(bx+18,by+1,7,4,9);}
 }
 bl_n=rw_n-rn0;__builtin_memcpy(bl_x,rw_x+rn0,bl_n);__builtin_memcpy(bl_y,rw_y+rn0,bl_n);__builtin_memcpy(bl_w,rw_w+rn0,bl_n);__builtin_memcpy(bl_h,rw_h+rn0,bl_n);__builtin_memcpy(bl_c,rw_c+rn0,bl_n);
 bl_cx=cam_x;bl_cy=cam_y;bl_ep=fog_epoch;bl_hs=hs;bl_bc=building_count;bl_r0=rn0;bl_do=draw_opt&4;bl_ok=1;
 }
 }
 __asm di __endasm;rw_dirty=1;rw_lock=0;__asm ei __endasm;
 for(k=n;k<sused;++k)sattr[(u16)k*4+3]=0x10;
 __asm di __endasm;
 p1_n=n>sused?n:sused;p1_bx=cam_x;p1_by=cam_y;p1_ready=1;
 __asm ei __endasm;
 sused=n;
 ++rendered_frames;
}
static void load_progress(u8 v) {u8 i;if(v>16)v=16;for(i=0;i<v;++i)hud_name(8+i,14,PA_SOLID+10);}
void video_init(void) {
 u8 i,j,q;u16 y;

 VDP_SetMode(VDP_MODE_SCREEN5);VDP_SetFrequency(VDP_FREQ_60HZ);VDP_EnableVBlank(TRUE);VDP_EnableDisplay(FALSE);
 V9_SetPort(V9_P07,2);V9_SetPort(V9_P07,0);
 V9_SetScreenMode(V9_MODE_P1);V9_SetDisplayEnable(FALSE);V9_SetInterrupt(V9_INT_NONE);
 V9_SetCursorEnable(FALSE);V9_SetSpriteEnable(TRUE);V9_SetSpritePatternAddr(V9_P1_SGT_38000);
 V9_SetBackgroundColor(0);V9_SetCommandWriteMask(0xFFFF);V9_SetScrolling(0,0);V9_SetScrollingB(0,0);
 for(i=0;i<64;++i)V9_SetPaletteEntry(i,palette9+(i&15)*3);
 cache_tick=frame_uploads=rendered_frames=0;
 for(i=0;i<RN;++i){disp_key[i]=pend_key[i]=NONE;act_blk[i]=0;bar_sig[i]=65535;}
 for(i=0;i<BMAX;++i)bv_state[i]=0;bv_epoch=fog_epoch;
 units_prep_reset();draw_opt=0;bl_ok=0;for(i=0;i<BMAX;++i)bw_hp[i]=bw_w[i]=bw_mx[i]=0;
 for(i=0;i<OCC_N;++i){occ_key[i]=255;occ_stamp[i]=0;occ_pend[i]=0;}
 p1_ready=0;rw_n=rc_n=rw_dirty=rw_lock=0;V9_FillVRAM(P1_SGT,0,32768); for(i=0;i<125;++i)V9_Poke(SPAT+(u16)i*4+3,0x10);
 sused=0;ord_n=0;
 /* clear both name tables and the HUD pattern table head */
 V9_FillVRAM(PNT_A,0,8192);V9_FillVRAM(PNT_B,0,8192);
 {u16 row[32];u8 cy,cx;for(cy=0;cy<20;++cy) {for(cx=0;cx<32;++cx)row[cx]=(u16)cy*32+cx;V9_WriteVRAM(PNT_A+((u16)cy<<7),(const u8*)row,64);}}
V9_FillVRAM(0x00000UL,0,24576);V9_FillVRAM(P1_PGT_B_BASE,0,1024);
 /* layer A patterns: solid colours 1..16 (pattern 1+c, c>=1 only), HUD top line */
 for(i=0;i<16;++i) {u8 pat[32];for(j=0;j<32;++j)pat[j]=i*17;pat_write(0,PA_SOLID+i,pat);}
 {u8 pat[32];for(j=0;j<32;++j)pat[j]=(j<8)?0xAA:0x11;pat_write(0,PA_HUDTOP,pat);}
 /* font: 64 chars as 256x16 bitmap strip -> two pattern rows; colour 0 would be transparent, use 1 */
 asset_begin(&font_asset);
 for(i=0;i<16;++i) {u8 b;asset_read(tile_buffer,128);for(b=0;b<128;++b){u8 v=tile_buffer[b];if(!(v&0xF0))v|=0x10;if(!(v&0x0F))v|=0x01;tile_buffer[b]=v;}
  V9_WriteVRAM((u32)((PA_FONT>>5)*8+i)*128,tile_buffer,128);}
 V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);
 /* sprite cells: pointer and selection ring */
 {u8 c[128];
  for(j=0;j<128;++j)c[j]=0;
  for(i=0;i<16;++i)for(q=0;q<8;++q)if(cursor[i]&(128>>q))c[i*8+(q>>1)]|=(q&1)?0x0F:0xF0;
  cell_write(CELL_PTR,c);
  cell_write(CELL_RING,ring_pixels);}
  {u8 c2[128],w,k,col,x,q2;
   for(col=0;col<2;++col)for(w=0;w<=8;++w) {
    for(j=0;j<128;++j)c2[j]=0;
    for(k=0;k<2;++k)for(x=0;x<16;++x)px4(c2,x,k,x<w*2?(col?9:5):12);
    cell_write((col?CELL_BAR_ENEMY:CELL_BAR_ALLY)+w,c2);
   }
   for(w=0;w<=8;++w) {
    for(j=0;j<128;++j)c2[j]=0;
    for(k=0;k<2;++k)for(x=0;x<16;++x)px4(c2,x,k,x<w*2?15:12);
    cell_write(CELL_BAR_WHITE+w,c2);
   }
   for(j=0;j<128;++j)c2[j]=0;
   for(x=6;x<11;++x)px4(c2,x,12,15);
   for(k=10;k<15;++k)px4(c2,8,k,15);
   cell_write(CELL_CROSS,c2);
   for(q2=0;q2<4;++q2) {
    for(j=0;j<128;++j)c2[j]=0;
    if(q2<2) {
     for(x=6;x<10;++x)px4(c2,x,8,11);
     for(k=7;k<10;++k)px4(c2,q2?6:10,k,15);
    } else {
     for(k=6;k<10;++k)px4(c2,8,k,11);
     for(x=7;x<10;++x)px4(c2,x,q2==2?10:6,15);
    }
    cell_write(CELL_ARROW+q2,c2);
   }
  }
 /* world patterns: fog, plain, dithered */
 {u8 pat[32];for(j=0;j<32;++j)pat[j]=0x11;pat_write(P1_PGT_B_BASE,1,pat);}
 for(q=0;q<3;++q) {
  asset_begin(static_assets+q);
  for(i=0;i<64;++i) {
   u8 t=q*64+i,b;u16 phy=t;
   asset_read(tile_buffer,128);
   if(phy>=50) {
    u16 ov=phy-50,bx=(ov&15)<<4,by=OV_Y+((ov>>4)<<4);u8 k;
    for(k=0;k<16;++k)V9_WriteVRAM((u32)(by+k)*128+(bx>>1),tile_buffer+(k<<3),8);
   }
   if(phy>=50)for(b=0;b<128;++b){u8 v=tile_buffer[b];if(!(v&0xF0))v|=0x30;if(!(v&0x0F))v|=0x03;tile_buffer[b]=v;}
   tile_patterns(P1_PGT_B_BASE,PB_PLAIN+phy*4,tile_buffer);
   for(b=0;b<128;++b){u8 row=b>>3,v=tile_buffer[b];if(row&1)v=(v&0xF0)|0x01;else v=(v&0x0F)|0x10;tile_buffer[b]=v;}
   tile_patterns(P1_PGT_B_BASE,PB_DITH+phy*4,tile_buffer);
  }
 }
 for(i=0;i<168;++i)if(poses[i].bank) {
  u8 n=kind_sizes[i/24]>>4;asset_begin(poses+i);
  for(q=0;q<n*n;++q) {
   u16 x,yy,px,py;u8 k;pose_pos(i/24,i-(i/24)*24,&px,&py);x=px+(q%n)*16;yy=py+(q/n)*16;
   asset_read(tile_buffer,128);
   for(k=0;k<16;++k)V9_WriteVRAM((u32)(yy+k)*128+(x>>1),tile_buffer+k*8,8);
  }
  ++boot_poses;load_progress((u8)((u16)boot_poses*16/104));
 }
 for(y=0;y<32;++y)for(j=0;j<32;++j)p1_cell(j,y);
 for(i=0;i<16;++i)hud_name(8+i,14,0);
 for(i=0;i<2;++i) {hud_draw=i;video_fill(0,160,256,52,1);video_fill(0,160,256,2,10);video_text(8,168,"[WOOD");video_text(136,168,"]GOLD");video_text(8,184,"^TIME");video_text(136,184,"_FREE");}
 hud_draw=0;
}
void video_enable(void) {
 video_sprites();V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);video_irq_install();
 video_enabled=1;
}
