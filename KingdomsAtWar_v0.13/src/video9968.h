/* V9968 SCREEN5: two circular field buffers, separate split-screen HUD. */
#define STATIC_Y 768
#define FONT_Y 640
#define CACHE_COUNT 14
u8 vport,chip_id;
volatile u8 video_ready,shown,present_pending,next_page;
volatile u16 display_x,display_y,next_x,next_y;
volatile u16 vframes;
u8 hud_pointer_saved[2],hud_pointer_x[2],hud_pointer_y[2];
static void vp_out(u8 p,u8 v) __naked {p;v;__asm ld c,a
 out (c),l
 ret __endasm;}
static u8 vp_in(u8 p) __naked {p;__asm ld c,a
 in a,(c)
 ret __endasm;}
static void raw_reg(u8 r,u8 v) __naked {r;v;__asm ld b,a
 ld a,(_vport)
 ld c,a
 out (c),l
 ld a,b
 or #128
 out (c),a
 ret __endasm;}
static void reg(u8 r,u8 v) {__asm di __endasm;raw_reg(r,v);__asm ei __endasm;}
static void address(u32 a) {raw_reg(14,a>>14);vp_out(vport,a);vp_out(vport,((a>>8)&63)|64);}
static const u8* upload_source;static u8 upload_size;
/* 32 T/byte: keep the SCREEN5 VRAM access spacing. */
static void upload_stream(void) __naked {
 __asm
 ld hl,(_upload_source)
 ld a,(_upload_size)
 ld b,a
 ld a,(_vport)
 dec a
 ld c,a
kaw012_upload:
 outi
 nop
 jr nz,kaw012_upload
 ret
 __endasm;
}
static void vwrite(u32 a,const u8* s,u8 n) {if(!n)return;upload_source=s;upload_size=n;__asm di __endasm;address(a);__asm ei __endasm;upload_stream();}
/* v0.13: pose piece upload straight from the mapped ROM window: rows of w bytes, one VRAM address per row,
 * 32 T per data byte (SCREEN5 access spacing). Z80: no staging buffer, no 32-bit address math. */
static const u8* up_src;static u16 up_y;static u8 up_xb,up_rows,up_w;
static void upload_piece(void) __naked {
 __asm
 ld hl,(_up_src)
 ld de,(_up_y)
up_next:
 ld a,(_vport)
 ld c,a
 di
 ld a,e
 rlca
 ld a,d
 rla
 out (c),a
 ld a,#0x8E
 out (c),a
 ld a,e
 rrca
 and #0x80
 ld b,a
 ld a,(_up_xb)
 or b
 out (c),a
 ld a,e
 srl a
 and #63
 or #64
 out (c),a
 ei
 dec c
 ld a,(_up_w)
 ld b,a
up_byte:
 outi
 nop
 jr nz,up_byte
 inc de
 ld a,(_up_rows)
 dec a
 ld (_up_rows),a
 jr nz,up_next
 ld (_up_src),hl
 ret
 __endasm;
}
/* One pixel row of 4 bpp data at (byte column xb, line y) without 32-bit arithmetic. */
static void vwrite_row(u16 y,u8 xb,const u8* s,u8 n) {
 u8 yl=(u8)y,yh=(u8)(y>>8);
 upload_source=s;upload_size=n;
 __asm di __endasm;raw_reg(14,(yh<<1)|(yl>>7));vp_out(vport,((yl&1)<<7)|xb);vp_out(vport,((yl>>1)&63)|64);__asm ei __endasm;
 upload_stream();
}
/* Bulk R32..R46 submission; port follows native cartridge detection. */
typedef struct {u16 sx,sy,dx,dy,w,h;u8 color,arg,cmd;} NativeCommand;
typedef char command_must_be_15_bytes[(sizeof(NativeCommand)==15)?1:-1];
volatile NativeCommand vcommand;
static u8 command_busy(void) __naked {
 __asm
 di
 ld a,(_vport)
 ld c,a
 ld a,#2
 out (c),a
 ld a,#143
 out (c),a
 in a,(c)
 ld b,a
 xor a
 out (c),a
 ld a,#143
 out (c),a
 ld a,b
 and #1
 ei
 ret
 __endasm;
}
static void command_submit(void) __naked {
 __asm
 di
 ld a,(_vport)
 ld c,a
 ld a,#32
 out (c),a
 ld a,#145
 out (c),a
 inc c
 inc c
 ld hl,#_vcommand
 ld b,#15
 otir
 ei
 ret
 __endasm;
}
static void gpu_wait(void) {poll_input();while(command_busy())if(video_enabled && (u8)vframes!=(u8)raw_tick)poll_input();}
static void gpu_copy(u16 sx,u16 sy,u16 dx,u16 dy,u16 w,u16 h,u8 trans) {
 gpu_wait();vcommand.sx=sx;vcommand.sy=sy;vcommand.dx=dx;vcommand.dy=dy;vcommand.w=w;vcommand.h=h;vcommand.color=0;vcommand.arg=0;
 vcommand.cmd=trans?0x98:((sx|dx|w)&1)?0x90:0xD0;command_submit();
}
static void gpu_fill(u16 x,u16 y,u16 w,u16 h,u8 c) {
 gpu_wait();vcommand.sx=vcommand.sy=0;vcommand.dx=x;vcommand.dy=y;vcommand.w=w;vcommand.h=h;vcommand.color=((x|w)&1)?c:c*17;vcommand.arg=0;
 vcommand.cmd=((x|w)&1)?0x80:0xC0;command_submit();
}
static void gpu_write_tile(u16 x,u16 y,const u8* tilebuf) {u8 j;for(j=0;j<16;++j)vwrite((u32)(y+j)*128+(x>>1),tilebuf+j*8,8);}
static void cache_xy(u8 slot,u8 q,u16* x,u16* y) {u8 t=(slot<7?slot:slot-7)*9+q;*x=(t&15)*16;*y=(slot<7?704:960)+(t>>4)*16;}
static void field_registers(void) {
 raw_reg(2,shown?0x3F:0x1F);raw_reg(23,display_y);raw_reg(25,0);
#ifdef KAW_PREVIEW
 raw_reg(26,((display_x+7)>>3)&31);raw_reg(27,(-display_x)&7);
#else
 raw_reg(26,(display_x>>3)&31);raw_reg(27,(display_x&7)^7);
#endif
 raw_reg(8,8);raw_reg(19,(display_y+159)&255);raw_reg(0,0x16);
}
static void irq_video(void) {
 u8 s0,s1;raw_reg(15,1);s1=vp_in(vport);raw_reg(15,0);s0=vp_in(vport);if(!video_ready)return;
 if(s0&128) {
  ++vframes;if(hud_pending) {hud_live=hud_ready;hud_pending=0;++hud_swaps;}
  if(present_pending) {shown=next_page;display_x=next_x;display_y=next_y;present_pending=0;}
  field_registers();
 } else if(s1&1) {
  raw_reg(8,10);raw_reg(2,0x5F);raw_reg(23,hud_live?160:96);raw_reg(26,0);raw_reg(27,0);raw_reg(25,0);raw_reg(0,6);
 }
}
u16 video_clock(void) {u16 t;__asm di __endasm;t=vframes;__asm ei __endasm;return t;}
void video_wait(void) {u16 t=video_clock();if(t!=last_time)return;while(video_clock()==t)Halt();}
static u16 hud_y(u16 y) {return 512+(hud_draw?64:0)+y-160;}
void video_char(u8 x,u16 y,u8 c) {c-=32;gpu_copy((c&31)*8,FONT_Y+(c>>5)*8,x,hud_y(y),8,8,0);}
void video_text(u8 x,u16 y,const c8* s) {while(*s && x<249){video_char(x,y,*s++);x+=8;}}
void video_fill(u16 x,u16 y,u16 w,u16 h,u8 c) {if(video_enabled)++hud_erase_count;gpu_fill(x,hud_y(y),w,h,c);}
void video_hud_begin(void) {u8 p=hud_draw;if(hud_pointer_saved[p]) {gpu_copy(32+p*8,656,hud_pointer_x[p],hud_y(hud_pointer_y[p]),8,8,0);hud_pointer_saved[p]=0;}}
void video_hud_end(void) {u8 p=hud_draw;if(mouse_y<160)return;hud_pointer_x[p]=mouse_x;hud_pointer_y[p]=mouse_y;gpu_copy(mouse_x,hud_y(mouse_y),32+p*8,656,8,8,0);gpu_copy(16,656,mouse_x,hud_y(mouse_y),8,8,1);hud_pointer_saved[p]=1;}
void video_hud_flush(void) {gpu_wait();}
void video_cursor(void) {
 u8 a[8];a[0]=(mouse_y<160?mouse_y+display_y-1:215);a[1]=mouse_x;a[2]=a[3]=0;if(a[0]==216){a[0]=215;a[2]=4;}a[4]=216;a[5]=a[6]=a[7]=0;
 upload_source=a;upload_size=8;__asm di __endasm;raw_reg(14,5);vp_out(vport,0x00);vp_out(vport,0x52);__asm ei __endasm;upload_stream();
}
void video_present(u8 p) {
 /* Hide unused circular-buffer rows during the short BIOS raster latency. */
 {u8 y=(cam_y+160)&255,n=8;while(n) {u8 h=256-(u16)y;if(!h || h>n)h=n;gpu_fill(0,(u16)p*256+y,256,h,1);n-=h;y+=h;}}
 gpu_wait();if(!video_enabled) {shown=p;display_x=cam_x;display_y=cam_y;return;}
 __asm di __endasm;next_page=p;next_x=cam_x;next_y=cam_y;present_pending=1;__asm ei __endasm;while(present_pending)Halt();video_cursor();
}
