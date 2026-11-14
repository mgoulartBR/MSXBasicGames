// SPDX-License-Identifier: GPL-2.0-or-later
// Included after bridge.c in bank 15, sharing the original Tater behavior.
#include "hill.h"
#include "hill_spawns.h"
void HillReset(void) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=1)return;
 for(i=0;i<11;++i){a=hill_actors+i;
  a->x=(u16)hill_spawn[i][0]*256;a->y=((u16)hill_spawn[i][1]-2)*256;
  if(i>=4)a->y+=15;
  a->vx=a->vy=a->timer=0;a->active=a->state=a->tick=0;
  a->ground=i<4;a->dir=1;a->frame=i<4?21:124;
 }
}
void HillSelect(void) __banked
{
 // World coordinates are 12.4 fixed point (u16, level < 4096 px): compare every actor against
 // the camera-relative windows in 1/16 px units with unsigned wrap, no per-actor shifts.
 //  keep active: -96 <= x <= 352 and -96 <= y <= 288;  activate: -48 <= x <= 304, -48 <= y <= 240.
 u8 i;CombatObject* a=hill_actors;
 u16 lx=(u16)((camera_px-96)<<4),ly=(u16)((camera_py-96)<<4),dx,dy;
 if(active_level!=1)return;
 for(i=0;i<12;++i,++a){
  if(i==11)a=&enemy;
  dx=a->x-lx;dy=a->y-ly;
  if(a->active){if(dx>7183 || dy>6159)a->active=0;}
  else if((u16)(dx-768)<=5647 && (u16)(dy-768)<=4623)a->active=1;
 }
 for(i=0;i<8;++i)wall_key[i]=65535;
}
void HillTick(void) __banked
{
 u8 i;CombatObject* a=hill_actors+4;
 if(active_level!=1)return;
 for(i=4;i<11;++i,++a){if(!a->active)continue;
  combat_dirty=1;BridgeWalker(a,0);if(death_ticks)return;
 }
}
u8 HillHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=1)return 0;
 for(i=0;i<11;++i){a=hill_actors+i;
  if(!a->active || a->state==1)continue;
  if(i<4){
   if((i16)(shot->x-a->x)>=512 || (i16)(a->x-shot->x)>=128 ||
      (i16)(shot->y-a->y)>=512 || (i16)(a->y-shot->y)>=128)continue;
  }else if(!BridgeTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
  a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;
  shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;
 }
 return 0;
}
void HillCollect(DrawObject* out) __banked
{
 u8 i;CombatObject* a;
 for(i=0;i<11;++i,++out){a=hill_actors+i;out->visible=0;
  if(!a->active || (enemy_mode&2))continue;
  if((i16)((a->x>>4)-camera_px)<-64 || (i16)((a->x>>4)-camera_px)>=280 ||
     (i16)((a->y>>4)-camera_py)<-32 || (i16)((a->y>>4)-camera_py)>=192)continue;
  out->x=(a->x>>4)-(i>=4 && a->state!=1?24:0);
  out->y=(a->y>>4)-(i>=4 && a->state!=1?4:0);
  out->frame=a->frame;out->visible=1;
 }
}
