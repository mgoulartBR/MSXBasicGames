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
#define POSE_Y0 64U
#define CACHE_COUNT 1
#define PB_PLAIN 8U
#define PB_DITH 768U
#define PA_SOLID 1U
#define PA_FONT 32U
#define PA_HUDTOP 100U
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
static void name_set2(u32 addr,u16 a,u16 b) {V9_SetWriteAddress(addr);V9_Poke16_CurrentAddr(a);V9_Poke16_CurrentAddr(b);}
static void p1_cell(u8 x,u8 y) {
 u8 st=fog_at(x,y);u16 n;u32 a;
 a=PNT_B+((u16)y<<8)+((u16)x<<2);
 if(!st) {name_set2(a,1,1);name_set2(a+128,1,1);return;}
 n=(st==1?PB_DITH:PB_PLAIN)+(u16)tile_source[fog_art(x,y)]*4;
 name_set2(a,n,n+1);name_set2(a+128,n+2,n+3);
}
void video_world_cell(u8 x,u8 y) {p1_cell(x,y);}
void video_world_tile(u8 x,u8 y) {p1_cell(x,y);}
void video_world_dither(u8 x,u8 y,u8 n) {u8 i;for(i=0;i<n;++i)p1_cell(x+i,y);}
void video_known_tile(u8 x,u8 y) {p1_cell(x,y);sprites_dirty|=1;}
void video_tile(u8 x,u8 y) {
 u16 cell=((u16)y<<5)+x;if(x>=32 || y>=32 || fog_at(x,y)!=2)return;fog_memory[cell]=art_map[cell];video_known_tile(x,y);
}
/* ---- HUD (layer A) ---- */
static void hud_name(u8 col,u8 row,u16 n) {V9_SetWriteAddress(PNT_A+((u16)row<<7)+((u16)col<<1));V9_Poke16_CurrentAddr(n);}
void video_char(u8 x,u16 y,u8 c) {hud_name(x>>3,(u8)(y>>3),PA_FONT+(u8)(c-32));}
void video_text(u8 x,u16 y,const c8* s) {while(*s && x<249){video_char(x,y,*s++);x+=8;}}
void video_fill(u16 x,u16 y,u16 w,u16 h,u8 c) {
 u8 col,row;u16 n=h<8?PA_HUDTOP:PA_SOLID+c;
 if(video_enabled)++hud_erase_count;
 for(row=y>>3;row<=(u8)((y+h-1)>>3);++row)for(col=x>>3;col<=(u8)((x+w-1)>>3);++col)hud_name(col,row,n);
}
/* ---- sprites ---- */
static u8 slot_key[RN],sattr[132*4],sused,spr_dirty_tail;
#define SPAT 0x3FE00UL
#define CELL_POSE(i) (((i)%5)*3+(((i)/5)*3)*16)
#define CELL_PTR 96
#define CELL_RING 97
static void cell_write(u8 cell,const u8* src16rows) {
 u8 k;for(k=0;k<16;++k)V9_WriteVRAM(P1_SGT+((u32)((cell>>4)<<4)+k)*128+((cell&15)<<3),src16rows+(k<<3),8);
}
static void spr_attr(u8* o,i16 x,i16 y,u8 pat,u8 info) {o[0]=(u8)(y-1);o[1]=pat;o[2]=(u8)x;o[3]=((u8)((x>>8)&3))|info;}
void video_cursor(void) {
 u8 a[4];spr_attr(a,mouse_x,mouse_y,CELL_PTR,0);
 __asm di __endasm;V9_WriteVRAM(SPAT,a,4);__asm ei __endasm;
}
void video_sprites(void) {
 u8 order[RN],count=0,i,j,k,n=1;
 ++cache_tick;
 V9_SetScrollingB(cam_x,cam_y);
 for(i=0;i<RN;++i) {
  i16 ax,ay;
  if(!combat_alive(i) || units[i].px+56<cam_x || units[i].px>cam_x+288 || units[i].py+32<cam_y || units[i].py>cam_y+208) {actor_visible[i]=0;rect_now[i].w=0;actor_key[i]=NONE;continue;}
  {u8 f=unit_art_frame(i);actor_visible[i]=unit_position(i);actor_x[i]=unit_draw_x+cam_x;actor_y[i]=unit_draw_y+cam_y;actor_key[i]=unit_kind[i]*24+f;}
  if(actor_visible[i])order[count++]=i;
 }
 /* front to back: larger py first (lower sprite index = in front) */
 for(i=1;i<count;++i) {k=order[i];j=i;while(j && units[order[j-1]].py<units[k].py) {order[j]=order[j-1];--j;}order[j]=k;}
 spr_attr(sattr,mouse_x,mouse_y,CELL_PTR,0);
 for(j=0;j<count;++j) {
  u8 sz,nn,sx,sy,cell;i16 x,y;
  i=order[j];sz=kind_sizes[unit_kind[i]];nn=sz>>4;
  x=actor_x[i]-cam_x;y=actor_y[i]-cam_y;cell=CELL_POSE(i);
  if(slot_key[i]!=actor_key[i]) {
   u8 key=actor_key[i];
   p1_copy((u16)(key%5)*48,POSE_Y0+(u16)(key/5)*48,(u16)(cell&15)<<4,P1_SGT_LINE+((u16)(cell>>4)<<4),sz,sz);
   slot_key[i]=key;++frame_uploads;
  }
  for(sy=0;sy<nn;++sy)for(sx=0;sx<nn;++sx) {
   if(n<125)spr_attr(sattr+((u16)n++<<2),x+sx*16,y+sy*16,cell+sx+(sy<<4),0x20);
  }
  if(i<N && (selection_mask&render_bit[i]) && n<125)spr_attr(sattr+((u16)n++<<2),x+sz/2-8,y+sz-16,CELL_RING,0x20);
 }
 for(k=n;k<sused;++k)sattr[(u16)k*4+3]=0x10;
 {u8 total=n>sused?n:sused;V9_WriteVRAM(SPAT,sattr,(u16)total*4);}
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
 for(i=0;i<RN;++i)slot_key[i]=NONE;
 for(i=0;i<125;++i)V9_Poke(SPAT+(u16)i*4+3,0x10);
 sused=0;
 /* clear both name tables and the HUD pattern table head */
 V9_FillVRAM(PNT_A,0,8192);V9_FillVRAM(PNT_B,0,8192);V9_FillVRAM(0x00000UL,0,4096);V9_FillVRAM(P1_PGT_B_BASE,0,1024);
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
   u16 x=(i%5)*48+(q%n)*16,yy=POSE_Y0+(u16)(i/5)*48+(q/n)*16;u8 k;
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
 hud_stamp[0]=hud_stamp[1]=65535;video_sprites();V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);
 video_enabled=1;video_cursor();
}
