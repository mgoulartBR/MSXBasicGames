/* V9990 B1 bitmap mode: full background and all unit poses resident in VRAM. */
#define STATIC_Y 1024
#define FONT_Y 1216
#define CACHE_COUNT 1
u8 shown;u16 hud_stamp[2];
static u8 v9_busy(void) __naked {__asm
 in a,(#0x65)
 and #1
 ret
 __endasm;}
/* Command block R#32..R#45 (SX,SY,DX,DY,NX,NY,ARG,LOP) sent with auto-increment. */
volatile struct {u16 r[6];u8 arg,lop;} v9c;
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
static void gpu_copy(u16 sx,u16 sy,u16 dx,u16 dy,u16 w,u16 h,u8 trans) {gpu_wait();v9c.r[0]=sx;v9c.r[1]=sy;v9c.r[2]=dx;v9c.r[3]=dy;v9c.r[4]=w;v9c.r[5]=h;v9c.arg=0;v9c.lop=trans?0x1C:0x0C;v9_lmmm();}
static void gpu_move(u16 sx,u16 sy,u16 dx,u16 dy,u16 w,u16 h,u8 arg) {gpu_wait();v9c.r[0]=sx;v9c.r[1]=sy;v9c.r[2]=dx;v9c.r[3]=dy;v9c.r[4]=w;v9c.r[5]=h;v9c.arg=arg;v9c.lop=0x0C;v9_lmmm();v9c.arg=0;}
/* Solid 16x16 swatches share the verified bitmap copy path. */
static void gpu_fill(u16 x,u16 y,u16 w,u16 h,u8 c) {
 u16 xx,yy;u8 ww,hh;
 for(yy=0;yy<h;yy+=16)for(xx=0;xx<w;xx+=16) {
  ww=w-xx>16?16:w-xx;hh=h-yy>16?16:h-yy;
  gpu_copy((u16)c*16,1248,x+xx,y+yy,ww,hh,0);
 }
}
static void gpu_write_tile(u16 x,u16 y,const u8* tilebuf) {u8 j;for(j=0;j<16;++j)V9_WriteVRAM((u32)(y+j)*256+(x>>1),tilebuf+j*8,8);}
u16 video_clock(void) {u16 t;__asm di __endasm;t=*((volatile u16*)0xFC9E);__asm ei __endasm;return t;}
void video_hud_swap(void) {if(hud_pending) {hud_live=hud_ready;hud_pending=0;++hud_swaps;sprites_dirty|=1;}}
void video_wait(void) {video_hud_swap();if(video_clock()!=last_time)return;while(V9_IsVBlank()){}while(!V9_IsVBlank()){};}
static u16 hud_y(u16 y) {return (hud_draw?64:0)+y-160;}
void video_char(u8 x,u16 y,u8 c) {c-=32;gpu_copy((c&31)*8,FONT_Y+(c>>5)*8,256+x,hud_y(y),8,8,0);}
void video_text(u8 x,u16 y,const c8* s) {while(*s && x<249){video_char(x,y,*s++);x+=8;}}
void video_fill(u16 x,u16 y,u16 w,u16 h,u8 c) {
 /* Keep the offscreen X coordinate across hud_y's DE/HL exchange. */
 volatile u16 dx=256+x,dy=hud_y(y);
 if(video_enabled)++hud_erase_count;gpu_fill(dx,dy,w,h,c);
}
void video_hud_begin(void) {}
void video_hud_end(void) {}
void video_hud_flush(void) {gpu_wait();}
void video_cursor(void) {__asm di __endasm;V9_SetCursorAttribute(0,mouse_x,mouse_y,3);__asm ei __endasm;}
/* v0.13: the page flip is done by the V9990 vertical-blank interrupt (via the BIOS H.KEYI hook), so the
 * main loop never idles waiting for the retrace; simulation and drawing continue meanwhile. */
volatile u8 flip_pending,flip_page,v9_irq_on;u8 r18_hi;
static u8 v9_in_flags(void) __naked {__asm
 in a,(#0x66)
 ret
 __endasm;}
static void v9_clear_vi(void) __naked {__asm
 ld a,#1
 out (#0x66),a
 ret
 __endasm;}
/* R#17 = 0, R#18 = (upper bits) | page: vertical scroll = page*256. A = page. */
static void v9_flip_regs(u8 page) __naked {page;__asm
 ld c,a
 ld a,#17
 out (#0x64),a
 xor a
 out (#0x63),a
 ld a,(_r18_hi)
 or a,c
 out (#0x63),a
 ret
 __endasm;}
static void irq_v9990(void) {
 if(v9_in_flags()&1) {
  v9_clear_vi();
  if(flip_pending) {v9_flip_regs(flip_page);shown=flip_page;flip_pending=0;}
 }
}
void video_irq_install(void) {
 __asm di __endasm;
 r18_hi=V9_GetRegister(18)&0xE0;
 BIOS_SetHookDirectCallback(H_KEYI,irq_v9990);
 v9_irq_on=1;flip_pending=0;
 V9_SetInterrupt(V9_INT_VBLANK);
 __asm ei __endasm;
}
void video_present(u8 p) {
 if(hud_stamp[p]!=hud_swaps){gpu_copy(256,hud_live?64:0,0,(u16)p*256+160,256,52,0);hud_stamp[p]=hud_swaps;}
 gpu_wait();
 if(v9_irq_on) {flip_page=p;flip_pending=1;return;}
 if(video_enabled) {while(V9_IsVBlank()){}while(!V9_IsVBlank()){};}
 shown=p;V9_SetScrolling(0,(u16)p*256);
}
