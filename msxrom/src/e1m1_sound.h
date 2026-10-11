/* OPL4 RAM waves 384+, header bank 4. YMF278B manual: BUSY/LD, signed PCM8. */
__sfr __at(0x7E) opl_a;
__sfr __at(0x7F) opl_d;
__sfr __at(0xC4) opl_status;
__sfr __at(0xC6) opl_fm_a;
__sfr __at(0xC7) opl_fm_d;

#define SOUND_VOICES 8
u8 audio_moon, audio_next;

static u8 opl_wait(u8 mask){
  u16 timeout=2000;
  while((opl_status&mask)&&--timeout);
  if(!timeout)audio_moon=0;
  return timeout!=0;
}
static void opl_select(u8 reg){ if(opl_wait(1))opl_a=reg; }
static void opl_write(u8 reg,u8 value){
  opl_select(reg);
  if(opl_wait(1))opl_d=value;
}
static void opl_addr(u8 hi,u16 lo){
  opl_write(3,hi); opl_write(4,(u8)(lo>>8)); opl_write(5,(u8)lo);
}
static void sound_reset(void){
  u8 i;
  sfx=audio_next=0; psg(8,0);
  if(audio_moon)for(i=0;i<SOUND_VOICES;i++)opl_write(0x68+i,0x40);
}
static void sound_init(void){
  u8 i; u16 n;
  psg(8,0); psg(9,0); psg(10,0);
  if(opl_status==255)return;
  opl_fm_a=5;
  __asm
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
  __endasm;
  opl_fm_d=3;   /* NEW + NEW2 */
  opl_write(2,0x11);
  if(!opl_wait(1))return;
  opl_select(2); if(!opl_wait(1)||(opl_d&0xE0)!=0x20)return;
  audio_moon=1;
  opl_addr(0x21,0xFFFF); opl_write(6,0x5A);
  opl_addr(0x21,0xFFFF); opl_select(6);
  if(!opl_wait(1)||opl_d!=0x5A)goto no_ram;
  opl_addr(0x21,0xFFFF); opl_write(6,0xA5);
  opl_addr(0x21,0xFFFF); opl_select(6);
  if(!opl_wait(1)||opl_d!=0xA5)goto no_ram;
  opl_addr(0x20,0);
  opl_select(6);
  for(i=0;i<NSFX_BANKS;i++){
    BANK(SFX_BANK0+i);
    for(n=0;n<16384;n++){
      if(!opl_wait(1))goto no_ram;
      opl_d=DATA[n];
    }
  }
  opl_write(2,0x10);  /* RAM header bank 4, memory-access mode off */
  opl_write(0xF9,0x12); /* -6 dB on both output channels */
  for(i=0;i<24;i++)opl_write(0x68+i,0x40);
  sound_reset();
  return;
no_ram:
  audio_moon=0;
}
void sound_play(u8 id){
  u8 voice;
  if(id>=NSFX)return;
  if(!audio_moon){ sfx=3; return; }
  voice=audio_next; audio_next=(audio_next+1)&(SOUND_VOICES-1);
  opl_write(0x68+voice,0);
  opl_write(0x20+voice,1); /* wave bit 8 + FN low = 0 */
  opl_write(0x08+voice,128+id);
  if(!opl_wait(3))return;
  opl_write(0x38+voice,0xF0); /* OCT=-1: 44100 * 0.5 * 0.5 = 11025 */
  opl_write(0x50+voice,41);   /* TL=20 (-15 dB), immediate */
  opl_write(0x68+voice,0xA0); /* key on, LFO disabled, center pan */
}
static void sound_tick(void){
  if(audio_moon){ psg(8,0); return; }
  if(sfx){
    psg(6,10); psg(7,0xB7); psg(8,sfx+3); sfx--;
  } else psg(8,0);
}
