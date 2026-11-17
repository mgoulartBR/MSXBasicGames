// SPDX-License-Identifier: GPL-2.0-or-later
// Original map 14: two Asparagustos, Broccolash, three Squashers; shared accepted physics.
#include "ticks.h"
#include "swamp.h"
#include "campaign.h"
#include "swamp_spawns.h"
#include "swamp_boxes.h"
#include "grove.h"
#include "pass.h"
#include "game.h"
#include "combat_assets.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
void SwampLoad(void) __banked
{
 u8 i;for(i=70;i<74;++i)SwampAtlas(i);
 GroveCopy(74,(const u8*)0x8000,world_map,6830);GroveCopy(74,(const u8*)0xa000,world_collision,1168);
}
void SwampReset(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=10)return;
 for(i=0;i<6;++i){a=swamp_actors+i;a->x=(u16)swamp_spawn[i][0]*256;a->y=(u16)swamp_spawn[i][1]*256-(i>=2?512:0);
 a->vx=a->vy=a->timer=0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=i<2?112:i==2?144:116;}
 swamp_overflow=swamp_bombs=0;for(i=0;i<3;++i)swamp_rise[i]=0;world_map[8635]=73;
}
void SwampSelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;if(active_level!=10)return;
 for(i=0;i<6;++i){a=swamp_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
 if(a->state==1){a->active=1;continue;}
 if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
 else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;}
 EnemyCache();
}
void SwampTick(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=10)return;
 for(i=0;i<6;++i){a=swamp_actors+i;if(!a->active)continue;combat_dirty=1;
 if(i<2)EnemyStep(a,1);else if(i>=3)GroveSquasher(a);else{if(a->frame>=136)a->frame-=28;EnemyBrocco(a);if(a->frame>=108)a->frame+=28;}
 if(death_ticks)return;}
}
u8 SwampHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;const i8* b;if(active_level!=10)return 0;
 for(i=0;i<6;++i){a=swamp_actors+i;if(!a->active || a->state==1)continue;
 b=swamp_boxes[a->frame>=136?8+a->frame-136:a->frame>=116?24+a->frame-116:a->frame-108];
 if((i16)(shot->x>>4)>(i16)(a->x>>4)+b[2] || (i16)((shot->x>>4)+8)<=(i16)(a->x>>4)+b[0] || (i16)(shot->y>>4)>(i16)(a->y>>4)+b[3] || (i16)((shot->y>>4)+8)<=(i16)(a->y>>4)+b[1])continue;
 if(i<2)a->y-=256;a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}
 return 0;
}
void SwampCollect(DrawObject* out) __banked
{
 u8 i,n=0;CombatObject* a;const SpriteDef* sp;u16 x,y;
 for(i=0;i<13;++i)out[i].visible=0;swamp_overflow=0;
 if(!(enemy_mode&2))
 for(i=0;i<6;++i){a=swamp_actors+i;if(!a->active)continue;x=(a->x>>4)-(a->state==1?0:24);y=(a->y>>4)-(a->state==1?0:4);sp=sprites[9]+a->frame;
 if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))continue;
 out[n].x=x;out[n].y=y;out[n].frame=a->frame;out[n].visible=1;++n;}
 for(i=0;i<3;++i)if(!(swamp_bombs&(1<<i)) || swamp_rise[i]){x=(u16)swamp_spawn[6+i][0]*16;y=(u16)swamp_spawn[6+i][1]*16-(swamp_rise[i]?(40-swamp_rise[i])/2:0);sp=sprites[9]+(swamp_rise[i]?89:102+(simulation_ticks/20)%3);
 if((i16)(x-camera_px)<=-24 || (i16)(x-camera_px)>=256 || (i16)(y-camera_py)<=-24 || (i16)(y-camera_py)>=(hardware_scroll?192:176))continue;out[n].x=x;out[n].y=y;out[n].frame=swamp_rise[i]?89:102+(simulation_ticks/20)%3;out[n].visible=1;++n;}
}

void SwampObjectsTick(void) __banked
{
 u8 i,bit;u16 x,y;if(active_level!=10)return;if(Tick20Zero())combat_dirty=1;
 for(i=0,bit=1;i<3;++i,bit<<=1){x=(u16)swamp_spawn[6+i][0]*256;y=(u16)swamp_spawn[6+i][1]*256;if(swamp_rise[i]){--swamp_rise[i];combat_dirty=1;}
 if(!(swamp_bombs&bit) && (i16)(player_x+64-x)<256 && (i16)(x-player_x-64)<256 && (i16)(player_y+48-y)<256 && (i16)(y-player_y-48)<448){swamp_bombs|=bit;swamp_rise[i]=40;++boobus_bombs;combat_dirty=1;}}
}
void SwampLoseBombs(void) __banked{u8 i,bit;for(i=0,bit=1;i<3;++i,bit<<=1)if((swamp_bombs&bit) && boobus_bombs)--boobus_bombs;swamp_bombs=0;}
