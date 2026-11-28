/* FM music and effects (MSX-MUSIC / OPLL YM2413). tools/music.py compiles each song into a stream of OPLL register writes stored in ROM banks;
 * the vertical-blank interrupt plays it: one frame = one music tick. The ISR maps the stream's bank into window 3 (A000h), reads the tokens
 * of the frame, and maps back the bank that g_Bank3Segment says the interrupted code was using (set_bank writes that variable first).
 * Streams: ms = music (OPLL channels 0..4 + rhythm 6..8), sx = effects (channel 5).
 * Tokens: 00..3F register + value | 80+k wait k+1 frames | FD next bank | FE loop | FF end. */
#ifndef KAW_MUSIC_H
#define KAW_MUSIC_H
#include "music_data.h"
typedef struct {u8 on,delay,bank,lbank;u16 ptr,lptr;} MStream;
MStream ms,sx;
u8 fm_ok,fm_found,music_cur,sfx_cur,battle_t;
extern u8 g_Bank3Segment;
static void opll_w(u16 rv) __z88dk_fastcall __naked { /* L = register, H = value */
 (void)rv;
 __asm
 ld a,l
 out (#0x7C),a
 ld b,#3
1$: djnz 1$
 ld a,h
 out (#0x7D),a
 ld b,#24
2$: djnz 2$
 ret
 __endasm;
}
static void stream_run(MStream* s) {
 const u8* p;u8 b;
 if(s->delay) {--s->delay;return;}
 *(volatile u8*)0x7800=s->bank;
 p=(const u8*)s->ptr;
 for(;;) {
  b=*p++;
  if(b<0x40) {opll_w((u16)b|((u16)(*p++)<<8));}
  else if(b>=0x80 && b<0xFD) {s->delay=b-0x80;break;}
  else if(b==0xFD) {++s->bank;*(volatile u8*)0x7800=s->bank;p=(const u8*)0xA000;}
  else if(b==0xFE) {s->bank=s->lbank;*(volatile u8*)0x7800=s->bank;p=(const u8*)s->lptr;}
  else {s->on=0;break;}
 }
 s->ptr=(u16)p;
}
/* Called by the vertical-blank interrupt (interrupts are disabled there). */
void music_isr(void) {
 if(!fm_ok || !(ms.on|sx.on))return;
 if(ms.on)stream_run(&ms);
 if(sx.on)stream_run(&sx);
 *(volatile u8*)0x7800=g_Bank3Segment;
}
void music_play(u8 id) {
 if(!fm_ok || id==music_cur)return;
 __asm di __endasm;
 music_cur=id;ms.on=0;ms.delay=0;ms.bank=song_bank[id];ms.ptr=0xA000;ms.lbank=song_loop_bank[id];ms.lptr=song_loop_ptr[id];ms.on=1;
 __asm ei __endasm;
}
void music_stop(void) {
 u8 c;
 if(!fm_ok)return;
 __asm di __endasm;
 ms.on=0;music_cur=255;
 for(c=0;c<5;++c)opll_w((u16)(0x20+c));
 opll_w(0x000E);
 __asm ei __endasm;
}
void sfx_play(u8 id) {
 if(!fm_ok || (sx.on && sfx_prio[id]<sfx_prio[sfx_cur]))return; /* a more important effect keeps the channel */
 __asm di __endasm;
 sfx_cur=id;sx.on=0;sx.delay=0;sx.bank=SFX_BANK;sx.ptr=sfx_ptr[id];sx.on=1;
 __asm ei __endasm;
}
#endif
