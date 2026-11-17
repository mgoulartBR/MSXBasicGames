// SPDX-License-Identifier: GPL-2.0-or-later
// Spud City's original object plane. No new persistent RAM allocation.
#include "ticks.h"
#include "city.h"
#include "city_spawns.h"
#include "city_boxes.h"
#include "combat_assets.h"
#include "campaign.h"
#include "game.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static u8 CityBase(u8 i,u8 dir){return i<7?(dir?124:130):i<10?(dir?112:108):i==10?(dir?21:16):i==11?138:(dir?152:158);}
static u8 CityBox(u8 frame){return frame<26?frame-16:frame<116?10+frame-108:frame<136?18+frame-124:frame<144?30+frame-136:38+frame-152;}
static u8 CityTouch(CombatObject* a,u16 x,u16 y,u8 w,u8 h){const i8* b=city_boxes[CityBox(a->frame)];return (i16)x<=(i16)(a->x>>4)+b[2] && (i16)(x+w)>(i16)(a->x>>4)+b[0] && (i16)y<=(i16)(a->y>>4)+b[3] && (i16)(y+h)>(i16)(a->y>>4)+b[1];}
void CityReset(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=6)return;
 for(i=0;i<15;++i){a=city_actors+i;a->x=(u16)city_spawn[i][0]*256;a->y=(u16)city_spawn[i][1]*256;
 if(i<7)a->y-=497;else if(i==10 || i>=12)a->y-=512;
 a->vx=a->vy=0;a->timer=i==11?200:0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=CityBase(i,1);}
 city_random=73;city_bombs=city_overflow=0;
 for(i=0;i<6;++i)city_rise[i]=0;
 for(i=0;i<4;++i)city_seeds[i].active=0;
 for(i=0;i<3;++i)pass_fry[i].active=0;
}
void CitySelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;if(active_level!=6)return;
 for(i=0;i<15;++i){a=city_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
 if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
 else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;}
 EnemyCache();
}
u8 CityHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;if(active_level!=6)return 0;
 for(i=0;i<15;++i){a=city_actors+i;if(i==11 || !a->active || a->state==1 || !CityTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
 if(i>=7 && i<10)a->y-=256;a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}return 0;
}
static void CityMelon(CombatObject* a)
{
 u8 i;MinesSeed* seed;if(a->timer)--a->timer;
 if(a->timer==6)for(i=0;i<4;++i){seed=city_seeds+i;if(seed->active)continue;seed->x=a->x+128;seed->y=a->y+384;seed->vx=0;seed->vy=20;seed->active=1;seed->tick=0;seed->frame=142;break;}
 if(!a->timer)a->timer=212;a->frame=138+(a->timer<=12);
}
static u8 CityWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static u16 CityHeight(u8 s,u8 x){switch(s&7){case 0:return 256;case 1:return 0;case 2:return (u16)x*8;case 3:return 128+(u16)x*8;case 4:return (u16)x*16;case 5:return 120-(u16)x*8;case 6:return 248-(u16)x*8;default:return 240-(u16)x*16;}}
static void CityKill(void){if(!death_ticks){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}}
static void CitySeed(MinesSeed* s)
{
 u16 ox=s->x,oy=s->y,bottom=s->y+112,mid,surface;u8 wall,mx;
 if(!s->active)return;
 s->x+=s->vx;s->y+=s->vy;if(++s->tick&1){s->vy+=3;if(s->vy>80)s->vy=80;}
 if(!(s->tick&3))s->frame^=1;
 if((i16)((s->x>>4)-camera_px)<-64 || (i16)((s->x>>4)-camera_px)>320 ||
    (i16)((s->y>>4)-camera_py)<-64 || (i16)((s->y>>4)-camera_py)>256){s->active=0;return;}
 mx=(s->x+(s->vx>0?112:0))>>8;
 if(s->vx && CityWall(mx,(s->y+48)>>8,s->vx>0?3:1)){s->x=ox;s->vx=-s->vx/2;}
 mid=s->x+56;mx=mid>>8;
 if(s->vy<0 && CityWall(mx,s->y>>8,2)){s->y=oy;s->vy=-s->vy/2;}
 wall=CityWall(mx,(s->y+112)>>8,0);
 if(wall && s->vy>=0){surface=((s->y+112)&0xff00)+CityHeight(wall,(mid>>4)&15);
  if(s->y+112>=surface && bottom<=surface+80){s->y=surface-113;MinesBounce(s,wall);}
 }
 if(s->active && (i16)(player_x+64-s->x)<128 && (i16)(s->x-player_x-64)<256 &&
    (i16)(player_y+48-s->y)<128 && (i16)(s->y-player_y-48)<448)CityKill();
}void CityTick(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=6)return;
 for(i=0;i<15;++i){a=city_actors+i;if(!a->active)continue;combat_dirty=1;
 if(i<10)EnemyStep(a,i>=7);else if(i==10)CityCarrotTick(a);else if(i==11)CityMelon(a);else CityFrenchTick(a);
 if(death_ticks)return;}
 CityFriesTick();if(death_ticks)return;
 for(i=0;i<4;++i)if(city_seeds[i].active){combat_dirty=1;CitySeed(city_seeds+i);if(death_ticks)return;}
}
void CityObjectsTick(void) __banked
{
 u8 i,bit;u16 x,y;if(active_level!=6)return;if(Tick20Zero())combat_dirty=1;
 for(i=0,bit=1;i<6;++i,bit<<=1){x=(u16)city_spawn[15+i][0]*256;y=(u16)city_spawn[15+i][1]*256;
 if(city_rise[i]){--city_rise[i];combat_dirty=1;}
 if(!(city_bombs&bit) && (i16)(player_x+64-x)<256 && (i16)(x-player_x-64)<256 && (i16)(player_y+48-y)<256 && (i16)(y-player_y-48)<448){city_bombs|=bit;city_rise[i]=40;++boobus_bombs;combat_dirty=1;}}
}
void CityLoseBombs(void) __banked {u8 i,bit;for(i=0,bit=1;i<6;++i,bit<<=1)if((city_bombs&bit) && boobus_bombs)--boobus_bombs;city_bombs=0;}
static void CityPut(DrawObject* out,u8* count,u16 x,u16 y,u8 frame)
{
 const SpriteDef* sp=sprites[5]+frame;
 if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))return;
 if(*count>=13){city_overflow=1;return;}out+=(*count)++;out->x=x;out->y=y;out->frame=frame;out->visible=1;
}
void CityCollect(DrawObject* out) __banked
{
 u8 i,n=0,bit;CombatObject* a;MinesSeed* seed;for(i=0;i<13;++i)out[i].visible=0;city_overflow=0;
 if(!(enemy_mode&2)){
 for(i=0;i<15;++i){a=city_actors+i;if(a->active)CityPut(out,&n,(a->x>>4)-((i==10 || a->state==1)?0:24),(a->y>>4)-((i==10 || a->state==1)?0:4),a->frame);}
 for(i=0;i<3;++i){a=pass_fry+i;if(a->active)CityPut(out,&n,(a->x>>4)-24,(a->y>>4)-4,a->frame);}
 for(i=0;i<4;++i){seed=city_seeds+i;if(seed->active)CityPut(out,&n,(seed->x>>4)-24,(seed->y>>4)-4,seed->frame);}}
 for(i=0,bit=1;i<6;++i,bit<<=1)if(!(city_bombs&bit) || city_rise[i])CityPut(out,&n,(u16)city_spawn[15+i][0]*16,(u16)city_spawn[15+i][1]*16-(city_rise[i]?(40-city_rise[i])/2:0),city_rise[i]?89:102+(simulation_ticks/20)%3);
}
