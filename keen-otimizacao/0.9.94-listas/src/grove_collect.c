// SPDX-License-Identifier: GPL-2.0-or-later
#include "objtouch.inc"
#include "ticks.h"
#include "grove.h"
#include "grove_spawns.h"
#include "grove_boxes.h"
#include "combat_assets.h"
u8 GroveNext(u8 i);
#include "campaign.h"
#include "game.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static u8 GroveTouch(CombatObject* a,CombatObject* shot)
{
 u8 f=a->frame;const i8* b=grove_boxes[f>=152?44+f-152:f>=136?20+f-136:f>=124?f-124:f>=116?36+f-116:12+f-108];
 return (i16)(shot->x>>4)<=(i16)(a->x>>4)+b[2] && (i16)((shot->x>>4)+8)>(i16)(a->x>>4)+b[0] && (i16)(shot->y>>4)<=(i16)(a->y>>4)+b[3] && (i16)((shot->y>>4)+8)>(i16)(a->y>>4)+b[1];
}
u8 GroveHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;if(active_level!=8)return 0;
 for(i=0;i<39;++i){a=grove_actors+i;if(!a->active || a->state==1 || !GroveTouch(a,shot))continue;
 grove_resume[i]=a->state;if(i==2)a->y-=256;else if(i>=5)a->y-=128;
 a->state=1;a->timer=350;a->vy=0;a->ground=a->tick=0;a->frame=26;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}return 0;
}
static void GrovePut(DrawObject* out,u8* n,u16 x,u16 y,u8 f)
{
 const SpriteDef* sp=sprites[7]+f;
 if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))return;
 if(*n>=13){grove_overflow=1;return;}out+=(*n)++;out->x=x;out->y=y;out->frame=f;out->visible=1;
}
void GroveCollect(DrawObject* out) __banked
{
 u8 i,n=0,bit;CombatObject* a;for(i=0;i<13;++i)out[i].visible=0;grove_overflow=0;
 if(!(enemy_mode&2))for(i=GroveNext(0);i<39;i=GroveNext(i+1)){a=grove_actors+i;GrovePut(out,&n,(a->x>>4)-((i>=5 || a->state==1)?0:24),(a->y>>4)-((i>=5 || a->state==1)?0:4),a->frame);}
 for(i=0,bit=1;i<3;++i,bit<<=1)if(!(grove_bombs&bit) || grove_rise[i])GrovePut(out,&n,(u16)grove_spawn[39+i][0]*16,(u16)grove_spawn[39+i][1]*16-(grove_rise[i]?(40-grove_rise[i])/2:0),grove_rise[i]?89:102+(simulation_ticks/20)%3);
}
extern u16 ca_x,ca_map,ca_tile,ca_wx,ca_wy;extern u8 ca_w,ca_h;void ObjList(void);
void GroveObjectsTick(void) __banked
{
 u8 i,bit;u16 x,y;if(active_level!=8)return;if(Tick20Zero())combat_dirty=1;
 ca_x=(u16)&grove_spawn[39][0];ca_map=(u16)grove_rise;ca_tile=(u16)&grove_bombs;ca_w=3;ca_wx=256;ca_wy=256;ca_h=0;ObjList();
}
void GroveLoseBombs(void) __banked{u8 i,bit;for(i=0,bit=1;i<3;++i,bit<<=1)if((grove_bombs&bit) && boobus_bombs)--boobus_bombs;grove_bombs=0;}
