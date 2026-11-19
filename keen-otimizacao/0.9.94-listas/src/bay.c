// SPDX-License-Identifier: GPL-2.0-or-later
// Original map 12: Asparagusto and three Broccolashes; shared accepted physics.
#include "bay.h"
#include "bay_spawns.h"
#include "bay_boxes.h"
#include "grove.h"
#include "pass.h"
#include "game.h"
#include "combat_assets.h"
#include "actornext.inc"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
void BayLoad(void) __banked
{
 u8 i;for(i=64;i<68;++i)BayAtlas(i);
 GroveCopy(68,(const u8*)0x8000,world_map,6830);GroveCopy(68,(const u8*)0xa000,world_collision,1168);
}
void BayReset(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=9)return;
 for(i=0;i<4;++i){a=bay_actors+i;a->x=(u16)bay_spawn[i][0]*256;a->y=(u16)bay_spawn[i][1]*256-(i?512:0);
 a->vx=a->vy=a->timer=0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=i?144:112;}
 bay_overflow=0;
}
void BaySelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;if(active_level!=9)return;
 for(i=0;i<4;++i){a=bay_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
 if(a->state==1){a->active=1;continue;}
 if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
 else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;}
 EnemyCache();
}
void BayTick(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=9)return;
 for(i=ACTOR_FIRST(bay_actors,4);i<4;i=ACTOR_AFTER(bay_actors,4,i)){a=bay_actors+i;combat_dirty=1;
 if(!i)EnemyStep(a,1);else{if(a->frame>=136)a->frame-=28;EnemyBrocco(a);if(a->frame>=108)a->frame+=28;}
 if(death_ticks)return;}
}
u8 BayHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;const i8* b;if(active_level!=9)return 0;
 for(i=0;i<4;++i){a=bay_actors+i;if(!a->active || a->state==1)continue;
 b=bay_boxes[a->frame>=136?8+a->frame-136:a->frame-108];
 if((i16)(shot->x>>4)>(i16)(a->x>>4)+b[2] || (i16)((shot->x>>4)+8)<=(i16)(a->x>>4)+b[0] || (i16)(shot->y>>4)>(i16)(a->y>>4)+b[3] || (i16)((shot->y>>4)+8)<=(i16)(a->y>>4)+b[1])continue;
 if(!i)a->y-=256;a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}
 return 0;
}
void BayCollect(DrawObject* out) __banked
{
 u8 i,n=0;CombatObject* a;const SpriteDef* sp;u16 x,y;
 for(i=0;i<13;++i)out[i].visible=0;bay_overflow=0;
 if(enemy_mode&2)return;
 for(i=ACTOR_FIRST(bay_actors,4);i<4;i=ACTOR_AFTER(bay_actors,4,i)){a=bay_actors+i;x=(a->x>>4)-(a->state==1?0:24);y=(a->y>>4)-(a->state==1?0:4);sp=sprites[8]+a->frame;
 if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))continue;
 out[n].x=x;out[n].y=y;out[n].frame=a->frame;out[n].visible=1;++n;}
}
