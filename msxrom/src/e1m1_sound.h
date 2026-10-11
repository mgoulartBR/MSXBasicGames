/* OPL4 RAM waves 384+, header bank 4. YMF278B manual: BUSY/LD, signed PCM8. */
__sfr __at(0x7E) opl_a;
__sfr __at(0x7F) opl_d;
__sfr __at(0xC4) opl_status;
__sfr __at(0xC6) opl_fm_a;
__sfr __at(0xC7) opl_fm_d;
__sfr __at(0xE7) sound_timer;

#define SOUND_VOICES 8
u8 audio_moon, audio_next;
u8 audio_wave[SOUND_VOICES], audio_clock;
u16 audio_left[SOUND_VOICES];

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
static void sound_service(void){
  /* E7 wraps every ~256 ms: a longer stall delays cleanup, not PCM playback. */
  u8 now=sound_timer,elapsed=now-audio_clock,i;
  audio_clock=now;
  if(!elapsed)return;
  for(i=0;i<SOUND_VOICES;i++)if(audio_left[i]){
    if(audio_left[i]<=elapsed){ audio_left[i]=0; opl_write(0x68+i,0x60); }
    else audio_left[i]-=elapsed;
  }
}
static void sound_reset(void){
  u8 i;
  sfx=audio_next=0; audio_clock=sound_timer; psg(8,0);
  for(i=0;i<SOUND_VOICES;i++){
    audio_left[i]=0;
    if(audio_moon)opl_write(0x68+i,0x60);
  }
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
  for(i=0;i<SOUND_VOICES;i++){
    audio_wave[i]=255;
    opl_write(0x20+i,1);
    opl_write(0x38+i,0xF0);
    opl_write(0x50+i,41);
  }
  sound_reset();
  return;
no_ram:
  audio_moon=0;
}
void sound_play(u8 id){
  u8 voice;
  if(id>=NSFX)return;
  if(!audio_moon){ sfx=3; return; }
  sound_service();
  voice=audio_next; audio_next=(audio_next+1)&(SOUND_VOICES-1);
  opl_write(0x68+voice,0x20);
  if(audio_wave[voice]!=id){
    opl_write(0x08+voice,128+id);
    if(!opl_wait(3))return;
    audio_wave[voice]=id;
  }
  opl_write(0x68+voice,0xA0); /* key on, LFO disabled, center pan */
  audio_clock=sound_timer;
  audio_left[voice]=sfx_ticks[id];
}
static void sound_tick(void){
  if(audio_moon){ sound_service(); return; }
  if(sfx){
    psg(6,10); psg(7,0xB7); psg(8,sfx+3); sfx--;
  } else psg(8,0);
}
