/* V9990 P1 renderer (KAW_P1).
 * Layer B = world tiles (64x64 names, hardware scroll, fog baked into the name choice),
 * layer A = HUD tiles (fixed), sprites = actors, selection rings and the mouse pointer.
 * VRAM map (byte addresses):
 *   0x00000  layer A pattern table (HUD: blank, solid colours, font)
 *   0x02000  pose bank: 256 px wide 4bpp bitmap (128 bytes per line), 48x48 cells, 5 per row
 *   0x38000  sprite pattern table: 256 patterns of 16x16 laid out as a 256x256 bitmap
 *   0x3FE00  sprite attribute table
 *   0x40000  layer B pattern table (world: fog, plain tiles, dithered tiles)
 *   0x7C000  layer A names, 0x7E000 layer B names
 * VDP commands in P1 address VRAM as a 256 px wide bitmap, so one pose = one rectangle copy.
 */
#define P1_SGT 0x38000UL
#define P1_PGT_B_BASE 0x40000UL
#define P1_SGT_LINE 1792U
#define POSE32_Y 192U
#define POSE48_Y 576U
#define CACHE_COUNT 1
#define PB_PLAIN 8U
#define PB_DITH 800U
#define PA_SOLID 641U
#define PA_FONT 672U
#define PA_HUDTOP 660U
const u16 render_bit[RN]={1,2,4,8,16,32,64,128,256,512};
u8 tile_buffer[128],boot_poses;
i16 strip_l[2],strip_t[2],strip_r[2],strip_b[2];u8 strip_n;
u8 box_old[2];u16 box_l[2],box_t[2],box_r[2],box_b[2];
u8 cache_key[CACHE_COUNT],cache_stamp[CACHE_COUNT],cache_tick,unit_cache[RN];
u16 frame_uploads,frame_hits,rendered_frames;
#include "render_types.h"
Rect rect_now[RN],rect_old[2][RN];
i16 actor_x[RN],actor_y[RN];u8 actor_visible[RN],actor_key[RN],old_hp[2][RN],old_hit[2][RN],actor_sel[2],old_key[2][RN],field_valid[2];
u16 field_x[2],field_y[2];u8 changed_x[2][32],changed_y[2][32],changed_count[2],ghost_old[2],ghost_size[2],ghost_color_old[2];u16 ghost_x[2],ghost_y[2];
u8 intersect(const Rect* a,const Rect* b) {return a->w && b->w && a->x+a->w>b->x && b->x+b->w>a->x && a->y+a->h>b->y && b->y+b->h>a->y;}
u8 shown;u16 hud_stamp[2];volatile u8 flip_pending,flip_page,v9_irq_on;
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
void video_present(u8 p) {p;}
void video_camera(void) {sprites_dirty|=1;}
u8 video_camera_ready(u16 y) {y;return 1;}
void video_stream(void) {}
/* No bitmap pages to repair: these remain only for the shared game code. */
void clip_copy(i16 wx,i16 wy,u16 sx,u16 sy,u8 w,u8 h,u8 p,u16 cx,u16 cy) {wx;wy;sx;sy;w;h;p;cx;cy;}
void world_fill(i16 wx,i16 wy,u8 w,u8 h,u8 p,u8 c) {wx;wy;w;h;p;c;}
void restore_field(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy) {x;y;w;h;p;cx;cy;}
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
void video_world_cell(u8 x,u8 y) {p1_cell(x,y);}
void video_world_tile(u8 x,u8 y) {p1_cell(x,y);}
void video_world_dither(u8 x,u8 y,u8 n) {u8 i;for(i=0;i<n;++i)p1_cell(x+i,y);}
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
#define MAXR 16
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
#define CELL_BAR_ENEMY 209
#define CELL_ARROW 226
static void pose_pos(u8 kind,u8 f,u16* x,u16* y) {
 if(kind_sizes[kind]==32) {u8 k=(kind==0?0:kind==1?1:kind==3?2:3)*24+f;*x=(u16)(k&7)*32;*y=POSE32_Y+(u16)(k>>3)*32;}
 else {u8 k=(kind==2?0:kind==4?1:2)*24+f,q=(u8)(((u16)k*205)>>10);*x=(u16)(k-q*5)*48;*y=POSE48_Y+(u16)q*48;}
}
static void px4(u8* buf,u8 x,u8 y,u8 c) {u8* p=buf+((u16)y<<3)+(x>>1);if(x&1)*p=(*p&0xF0)|c;else *p=(*p&0x0F)|(c<<4);}
static void cell_write(u8 cell,const u8* src16rows) {
 u8 k;for(k=0;k<16;++k)V9_WriteVRAM(P1_SGT+((u32)((cell>>4)<<4)+k)*128+((cell&15)<<3),src16rows+(k<<3),8);
}
static void spr_attr(u8* o,i16 x,i16 y,u8 pat,u8 info) {o[0]=(u8)(y-1);o[1]=pat;o[2]=(u8)x;o[3]=((u8)((x>>8)&3))|info;}
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
  io_int=1;
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
static u16 e_o;static i16 e_x,e_y;static u8 e_cell,e_n,e_r,e_c;
static void emit_block(void) __naked {
 __asm
 ld hl,(_e_o)
 ld a,(_e_y)
 dec a
 ld d,a
 ld a,(_e_cell)
 ld e,a
 ld a,(_e_n)
 ld (_e_r),a
eb_row:
 ld bc,(_e_x)
 ld a,(_e_n)
 ld (_e_c),a
 push de
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
 ld a,b
 adc a,#0
 ld b,a
 ld a,(_e_c)
 dec a
 ld (_e_c),a
 jp nz,eb_col
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
static u16 sc_cull,sc_bitw;
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
 ld e,0(ix)
 ld d,1(ix)
 ld hl,#56
 add hl,de
 ld de,(_cam_x)
 or a,a
 sbc hl,de
 jp c,cl_cull
 ld hl,(_cam_x)
 ld de,#288
 add hl,de
 ld e,0(ix)
 ld d,1(ix)
 or a,a
 sbc hl,de
 jp c,cl_cull
 ld e,2(ix)
 ld d,3(ix)
 ld hl,#32
 add hl,de
 ld de,(_cam_y)
 or a,a
 sbc hl,de
 jp c,cl_cull
 ld hl,(_cam_y)
 ld de,#208
 add hl,de
 ld e,2(ix)
 ld d,3(ix)
 or a,a
 sbc hl,de
 jp c,cl_cull
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
static u16 bar_sig[RN];static u8 bar_w[RN];static u8 ord_prev[RN],ord_n;
void video_sprites(void) {
 u8 order[RN],count=0,i,j,k,n;u8* o=sattr+4;u16 vis=0;
 ++cache_tick;
 /* copies issued earlier are finished once the engine is idle: switch those blocks in */
 if(!v9_busy())for(i=0;i<RN;++i)if(pend_key[i]!=NONE) {act_blk[i]^=1;disp_key[i]=pend_key[i];pend_key[i]=NONE;}
 sp_cull();
 for(i=0;i<RN;++i) {
  if(sc_cull&render_bit[i]) {actor_visible[i]=0;rect_now[i].w=0;actor_key[i]=NONE;continue;}
  {u8 f=unit_art_frame(i);actor_visible[i]=unit_position(i);actor_x[i]=unit_draw_x+cam_x;actor_y[i]=unit_draw_y+cam_y;actor_key[i]=unit_kind[i]*24+f;}
  if(actor_visible[i])vis|=render_bit[i];
 }
 /* keep last frame's order (nearly sorted already), then append actors that just appeared */
 for(j=0;j<ord_n;++j){i=ord_prev[j];if(vis&render_bit[i]){order[count++]=i;vis&=~render_bit[i];}}
 for(i=0;i<RN;++i)if(vis&render_bit[i])order[count++]=i;
 /* front to back: larger py first (lower sprite index = in front) */
 for(i=1;i<count;++i) {k=order[i];j=i;while(j && units[order[j-1]].py<units[k].py) {order[j]=order[j-1];--j;}order[j]=k;}
 for(j=0;j<count;++j)ord_prev[j]=order[j];
 ord_n=count;
 __asm di __endasm;p1_ready=0;__asm ei __endasm;
 /* arrows: 5x5 px projectiles, in front of everything in the field */

 for(k=0;k<PS;++k)if(shot_time[k] && o<sattr+496) {
  i16 ax=shot_x[k],ay=shot_y[k];
  if(fog_at(ax>>4,ay>>4)!=2 || ax+2<(i16)cam_x || ax-2>=(i16)cam_x+256 || ay+2<(i16)cam_y || ay-2>=(i16)cam_y+160)continue;
  SPR(ax-8-cam_x,ay-8-cam_y,CELL_ARROW+(shot_dx[k]?(shot_dx[k]>0?0:1):(shot_dy[k]>0?2:3)),0x20);
 }
 for(j=0;j<count;++j) {
  u8 sz,nn,sx,sy,cell;i16 x,y;
  i=order[j];sz=kind_sizes[unit_kind[i]];nn=sz>>4;
  if(actor_key[i]!=disp_key[i] && pend_key[i]==NONE) {
   u8 key=actor_key[i],c2=CELL_POSE(i,act_blk[i]^1);
   {u16 px,py;pose_pos(unit_kind[i],key-unit_kind[i]*24,&px,&py);p1_copy(px,py,(u16)(c2&15)<<4,P1_SGT_LINE+((u16)(c2>>4)<<4),sz,sz);}
   pend_key[i]=key;++frame_uploads;
  }
  if(disp_key[i]==NONE)continue;
  x=actor_x[i]-cam_x;y=actor_y[i]-cam_y;cell=CELL_POSE(i,act_blk[i]);
  if((units[i].owner || unit_hp[i]<unit_max_hp[unit_kind[i]] || combat_target[i]!=NONE) && o<sattr+496) {
   u16 sig=unit_hp[i]|((u16)unit_kind[i]<<8);
   if(bar_sig[i]!=sig){bar_sig[i]=sig;bar_w[i]=(u8)((u16)unit_hp[i]*16/unit_max_hp[unit_kind[i]]);}
   SPR(x+(sz>>1)-8,y+pose_bounds[((u16)actor_key[i]<<2)+1]-4,(units[i].owner?CELL_BAR_ENEMY:CELL_BAR_ALLY)+bar_w[i],0x20);
  }
  if((u16)o+((u16)nn*nn<<2)<=(u16)sattr+500) {e_o=(u16)o;e_x=x;e_y=y;e_cell=cell;e_n=nn;emit_block();o=(u8*)e_o;}
  if(i<N && (selection_mask&render_bit[i]) && o<sattr+496)SPR(x+(sz>>1)-8,y+sz-16,CELL_RING,0x20);
 }
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
 for(k=0;k<building_count;++k)if(buildings[k].state && (building_owner[k] || building_hp[k]<(buildings[k].type==0?(building_owner[k]?180:240):96)) && fog_target(128+k)) {
  i16 bx=(i16)buildings[k].x*16-(i16)cam_x,by=(i16)buildings[k].y*16-(i16)cam_y;u8 max=buildings[k].type==0?(building_owner[k]?180:240):96;
  if(building_owner[k] || building_hp[k]<max) {
   u8 w=(u8)((u16)building_hp[k]*24/max);
   rl_add(bx+16,by-4,24,2,12);if(w)rl_add(bx+16,by-4,w,2,building_owner[k]?9:5);
  }
  if(building_owner[k]){rl_add(bx+17,by+1,1,10,11);rl_add(bx+18,by+1,7,4,9);}
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
 for(i=0;i<2;++i)field_valid[i]=changed_count[i]=ghost_old[i]=box_old[i]=0;
 hud_stamp[0]=hud_stamp[1]=65535;
 shown=0;cache_tick=frame_uploads=frame_hits=rendered_frames=0;
 flip_pending=0;v9_irq_on=0;
 for(i=0;i<RN;++i){disp_key[i]=pend_key[i]=NONE;act_blk[i]=0;bar_sig[i]=65535;}
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
   for(col=0;col<2;++col)for(w=0;w<=16;++w) {
    for(j=0;j<128;++j)c2[j]=0;
    for(k=0;k<2;++k)for(x=0;x<16;++x)px4(c2,x,k,x<w?(col?9:5):12);
    cell_write((col?CELL_BAR_ENEMY:CELL_BAR_ALLY)+w,c2);
   }
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
 hud_stamp[0]=hud_stamp[1]=65535;video_sprites();V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);video_irq_install();
 video_enabled=1;
}
