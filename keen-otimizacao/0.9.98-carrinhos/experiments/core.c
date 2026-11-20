#define KEEN_CORE15
#define KEEN_CASTLE_H
void CastleObjectsTick(void);
#define KEEN_GROVE_H
void GroveObjectsTick(void);
#define KEEN_APPLE_H
void AppleObjectsTick(void);
#define KEEN_CITY_H
void CityObjectsTick(void);
#define KEEN_PASS_H
void PassObjectsTick(void);
#define KEEN_RAPIDS_H
void RapidObjectsTick(void);
// GPL-2.0-or-later. Shared ROM/DOS payload; linked once at 8000h.
// Adapter removes cartridge bank ABI, not gameplay source or algorithms.
typedef unsigned char u8;
typedef signed char i8;
typedef unsigned int u16;
typedef signed int i16;
#define __banked
#define SoundPlay(effect) ((void)0)
#define KEEN_GAME_H
#define KEEN_WORLD_H
#define KEEN_COMBAT_H
#define KEEN_KEYDOOR_H
#define MAP_WIDTH (world_collision[1120])
#define MAP_HEIGHT (world_collision[1121])
#define LEVEL_SPAWN_X (*((u16*)(world_collision+1122)))
#define LEVEL_SPAWN_Y (*((u16*)(world_collision+1124)))
#define LEVEL_LEFT (world_collision[1126])
#define KEY_X (77U*256)
#define KEY_Y (59U*256)
#define DOOR_X (76U*256)
#define DOOR_Y (81U*256)
typedef struct {u16 x,y;u8 frame,visible;} DrawObject;
void CoreAction(u8 action);
// Fixed entry; argument in A, as in SDCC call(1).
void CoreEntry(u8 action) __naked {action; __asm
 jp _CoreAction
 __endasm;}
#include "assets.h"
#define world_map core_map
#define world_collision core_collision
u8 active_level;
volatile u8 death_ticks,combat_dirty;
volatile u16 defeats,camera_px,camera_py;
void KeyDoorReset(void);
void KeyDoorTick(void);
u16 MapTile(u16 i){u16 n=i+(i>>3);u8 s=i&7;return (((u16)world_map[n]|((u16)world_map[n+1]<<8))>>s)&511;}
#include "../../src/game.c"
#include "../../src/keydoor.c"
// Fixed, separately reserved report; RAM globals at C140 stay within C240.
// 0 magic,1 done,2 ticks,3 total steps,4 trace,5 x,6 y,7 vx,8 vy,
// 9 wall hits,10 ceiling hits,11 slope hits,12 respawns,13 state,14 cpu-unused.
#define R ((volatile u16*)0xc100)
static u8 Input(u8 phase){u8 in=phase<70?2:1;if(phase<24 || (phase>=70 && phase<94))in|=16;return in;}
void CoreAction(u8 action){
 u16 i,start,trace,totalwalls=0,totalceil=0,totalslope=0;u8 cycle,phase;
 if(!action){
  // Volatile is essential: this SDCC miscompiles the nonvolatile address loop.
  for(i=0xc140;i<0xc240;++i)*((volatile u8*)i)=0;
  for(i=0;i<15;++i)R[i]=0;
  R[0]=0x1515;active_level=1;return;
 }
 trace=0x4b44;start=*((volatile u16*)0xfc9e);
 for(cycle=0;cycle<4;++cycle){
  Game_Reset();simulation_ticks=wall_hits=ceiling_hits=slope_hits=respawns=0;
  death_ticks=defeats=0;player_x=102U*256;player_y=29U*256;
  phase=0;
  for(i=0;i<700;++i){
   Game_Tick(Input(phase));if(++phase==140)phase=0;
   trace=(trace<<1)|(trace>>15);
   trace^=player_x;trace+=player_y;trace^=(u16)player_vx+(u16)player_vy;
   trace+=(u16)grounded+((u16)player_frame<<8);
   if(death_ticks||level_done){R[1]=2;return;}
  }
  totalwalls+=wall_hits;totalceil+=ceiling_hits;totalslope+=slope_hits;
 }
 R[2]=*((volatile u16*)0xfc9e)-start;R[3]=2800;R[4]=trace;
 R[5]=player_x;R[6]=player_y;R[7]=player_vx;R[8]=player_vy;
 R[9]=totalwalls;R[10]=totalceil;R[11]=totalslope;R[12]=respawns;
 R[13]=grounded|((u16)player_frame<<8);R[1]=1;
}

void RapidObjectsTick(void){}

void PassObjectsTick(void){}

void CityObjectsTick(void){}

void AppleObjectsTick(void){}

void GroveObjectsTick(void){}

void CastleObjectsTick(void){}
