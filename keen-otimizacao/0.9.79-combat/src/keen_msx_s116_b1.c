// SPDX-License-Identifier: GPL-2.0-or-later
// PSG channel A, updated by a fixed-ROM H.TIMI handler. No bank switch,
// BIOS/library call, graphics command or game-state access in the interrupt.
#include "sound.h"
#include "combat.h"
#include "campaign.h"
#include "pickups.h"
#include "keydoor.h"
__sfr __at(0xa0) sound_addr;
__sfr __at(0xa1) sound_write;
__sfr __at(0xa2) sound_read;
u8 sound_old_hook[5];
void SoundISR(void);
void LineISR(void);
u8 bar_on,cur_r2,cur_r23,cur_r26,cur_r27,cur_r8;
volatile u8 sound_duration,sound_volume,sound_priority;
volatile u16 sound_period;
volatile i16 sound_step;
typedef struct {u16 period;i16 step;u8 duration,volume,noise,priority;} Effect;
static const Effect effects[11]={
 {640,-22,18,12,0,1}, {180,26,12,12,0,2},
 {420,-28,14,11,0,3}, {120,16,18,13,5,4},
 {360,24,44,14,12,8}, {660,-14,40,13,0,7},
 {560,-20,25,12,0,5}, {1500,0,6,7,0,1},
 {900,-12,60,13,0,9}, {500,-24,16,10,0,2},
 {100,28,25,14,14,5}
};
void SoundPlay(u8 id) __banked
{
 const Effect* e;u8 mixer,ticks;
 if(!id || id>11)return;
 e=effects+id-1;
 if(sound_duration && e->priority<sound_priority)return;
 ticks=VDP_GetFrequency()?(u16)e->duration*5/6:e->duration;
 DisableInterrupt();
 sound_period=e->period;sound_step=e->step;sound_volume=e->volume;sound_priority=e->priority;
 // Preserve the PSG I/O direction and channels B/C, used by BIOS/joysticks.
 sound_addr=7;mixer=sound_read;mixer=(mixer|8)&~1;
 if(e->noise)mixer&=~8;
 sound_addr=7;sound_write=mixer;
 sound_addr=6;sound_write=e->noise;
 sound_addr=0;sound_write=e->period&255;sound_addr=1;sound_write=e->period>>8;
 sound_addr=8;sound_write=e->volume;sound_duration=ticks;
 EnableInterrupt();
}
void SoundInit(void) __banked
{
 u8 i;
 // Keep the original hook's five-byte interslot trampoline intact.
 DisableInterrupt();
 for(i=0;i<5;++i)sound_old_hook[i]=((u8*)0xfd9f)[i];
 sound_duration=sound_priority=0;
 sound_addr=8;sound_write=0;
 *((u8*)0xf3db)=0; // BIOS CLIKSW: no keyboard click over game effects.
 *((u8*)0xfd9f)=0xc3;*((u16*)0xfda0)=(u16)SoundISR;
 *((u8*)0xfda2)=*((u8*)0xfda3)=0;
 *((u8*)0xfd9a)=0xc3;*((u16*)0xfd9b)=(u16)LineISR;   // H.KEYI: HUD-bar line interrupts
 EnableInterrupt();
}
