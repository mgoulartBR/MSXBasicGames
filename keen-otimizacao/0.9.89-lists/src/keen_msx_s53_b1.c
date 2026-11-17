// SPDX-License-Identifier: GPL-2.0-or-later
// Rhubarb Rapids actors and inventory; private state reuses unused packed-map RAM.
#include "actornext.inc"
#include "objtouch.inc"
#include "ticks.h"
#include "rapids.h"
#include "game.h"
#include "rapids_spawns.h"
#include "rapids_boxes.h"
#include "keydoor.h"
#include "campaign.h"
#include "tomato.h"
extern volatile u16 camera_px,camera_py;
static u8 RapidWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static u16 RapidHeight(u8 s,u8 x){switch(s&7){case 0:return 256;case 1:return 0;case 2:return (u16)x*8;case 3:return 128+(u16)x*8;case 4:return (u16)x*16;case 5:return 120-(u16)x*8;case 6:return 248-(u16)x*8;default:return 240-(u16)x*16;}}
static u8 RapidBase(u8 i,u8 dir){return i<7?(dir?124:130):i==7?(dir?112:108):(dir?116:120);}
void RapidSelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;if(active_level!=4)return;
 for(i=0;i<10;++i){a=rapid_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
 if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
 else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;}
 EnemyCache();
}
static u8 RapidTouch(CombatObject* a,u16 x,u16 y,u8 w,u8 h){const i8* b=rapids_boxes[a->frame-108];return (i16)x<=(i16)(a->x>>4)+b[2] && (i16)(x+w)>(i16)(a->x>>4)+b[0] && (i16)y<=(i16)(a->y>>4)+b[3] && (i16)(y+h)>(i16)(a->y>>4)+b[1];}
static void RapidKill(void){if(!death_ticks){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}}
u8 RapidHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;if(active_level!=4)return 0;
 for(i=0;i<10;++i){a=rapid_actors+i;if(!a->active || a->state==1 || !RapidTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
 if(i==7)a->y-=256;a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}return 0;
}
static void RapidFall(CombatObject* a)
{
 u16 oy=a->y,bottom=oy+496,mid=a->x+184,surface;u8 wall;i8 row;if(a->ground){a->vy=0;return;}
 if(a->tick&1){a->vy+=3;if(a->vy>80)a->vy=80;}a->y+=a->vy;
 if(a->vy<0){if(RapidWall(mid>>8,a->y>>8,2)){a->y=oy;a->vy=0;}return;}
 for(row=(bottom>>8)-1;row<=(i8)((a->y+496)>>8);++row){wall=RapidWall(mid>>8,row,0);if(!wall)continue;surface=(u16)row*256+RapidHeight(wall,(mid>>4)&15)-1;
 if((i16)(surface-bottom)>=-16 && (i16)(surface-a->y-496)<=0){a->y=surface-496;a->vy=0;a->ground=1;return;}}
}
static void RapidWalk(CombatObject* a)
{
 u16 ox=a->x,oy=a->y,bottom=oy+496,mid,surface;u8 wall;i8 row;
 a->x+=a->dir?128:-128;mid=a->x+248;
 if(!RapidWall((a->x+(a->dir?432:32))>>8,(a->y+64)>>8,a->dir?3:1) && !RapidWall((a->x+(a->dir?432:32))>>8,(a->y+464)>>8,a->dir?3:1)){
 for(row=(bottom>>8)-1;row<=(i8)((bottom+144)>>8);++row){wall=RapidWall(mid>>8,row,0);if(!wall)continue;surface=(u16)row*256+RapidHeight(wall,(mid>>4)&15)-1;
 if((i16)(surface-bottom)>=-144 && (i16)(surface-bottom)<=144){a->y=surface-496;return;}}}
 a->x=ox;a->y=oy;a->dir^=1;
}
static void RapidSquasher(CombatObject* a,u8 i)
{
 i16 dx,dy,left,right;u16 ox;
 if(a->state==1){if(!--a->timer){a->state=0;a->frame=RapidBase(i,a->dir);a->tick=0;a->vy=0;++flower_returns;return;}a->frame=26+((a->timer/20)&3);++a->tick;RapidFall(a);return;}
 if(a->state==2 || a->state==3){ox=a->x;a->x+=a->vx;++a->tick;
 if(RapidWall((a->x+(a->vx>0?432:32))>>8,(a->y+64)>>8,a->vx>0?3:1) || RapidWall((a->x+(a->vx>0?432:32))>>8,(a->y+464)>>8,a->vx>0?3:1))a->x=ox;
 RapidFall(a);if(a->ground){a->state=4;a->tick=0;a->vx=0;}else if(a->state==2 && a->tick>=20)a->state=3;
 a->frame=RapidBase(i,a->dir)+(a->state==2?2:3);
 }else if(a->state==4){a->frame=RapidBase(i,a->dir)+3;if(++a->tick>=10){a->state=0;a->tick=0;a->frame=RapidBase(i,a->dir);}}
 else if(!a->ground){++a->tick;RapidFall(a);if(a->ground)a->tick=0;}
 else if(++a->tick>=10){a->tick=0;dx=player_x-a->x;dy=player_y-a->y;
 if(dy>768 || dy<-768){rapid_random=rapid_random*33+17;if(rapid_random<8)a->dir^=1;RapidWalk(a);}
 else if(a->dir?dx>1536:dx<-1536)RapidWalk(a);
 else if(a->dir?dx<-128:dx>128){a->dir=1;RapidWalk(a);} // Original kd_act2.c facing rule.
 else{a->vy=-50;a->vx=dx/60;a->ground=0;a->state=2;a->frame=RapidBase(i,a->dir)+2;return;}
 a->frame=RapidBase(i,a->dir)+((a->frame-RapidBase(i,a->dir)+1)&1);
 }
 if(RapidTouch(a,(player_x>>4)+4,(player_y>>4)+3,16,28)){
 if(a->state==3)RapidKill();else{left=player_x+304-a->x-32;right=a->x+432-player_x-64;if(left>0 && right>0)RapidPush(left<right?-left:right);}}
}
void RapidTick(void) __banked{u8 i;CombatObject* a;if(active_level!=4)return;for(i=ACTOR_FIRST(rapid_actors,10);i<10;i=ACTOR_AFTER(rapid_actors,10,i)){a=rapid_actors+i;combat_dirty=1;if(i<8)EnemyStep(a,i==7);else RapidSquasher(a,i);if(death_ticks)return;}}
static u8 RapidObjectTouch(u16 x,u16 y,u16 w,u16 h){ca_wx=w;ca_wy=h;return ObjTouch(x,y);}
void RapidObjectsTick(void) __banked
{
 u8 i,bit;u16 x,y;if(active_level!=4)return;if(Tick20Zero())combat_dirty=1;
 for(i=0,bit=1;i<2;++i,bit<<=1){x=(u16)rapids_spawn[11+i][0]*256;y=(u16)rapids_spawn[11+i][1]*256;
 if(rapid_rise[i]){--rapid_rise[i];combat_dirty=1;}
 if(!(rapid_keys&bit) && RapidObjectTouch(x,y,240,112)){rapid_keys|=bit;++key_owned;rapid_rise[i]=40;combat_dirty=1;}}
 for(i=0;i<2;++i){if(rapid_doors[i]==2)continue;
 if(rapid_doors[i]==1){++rapid_door_ticks[i];combat_dirty=1;if(rapid_door_ticks[i]>=24){rapid_doors[i]=2;continue;}}
 x=(u16)rapids_spawn[13+i][0]*256;y=(u16)(rapids_spawn[13+i][1]-2)*256-(u16)rapid_door_ticks[i]*32;
 if(RapidObjectTouch(x,y,240,1008)){if(!rapid_doors[i] && key_owned){--key_owned;rapid_doors[i]=1;combat_dirty=1;}player_x=player_x<x?x-305:x+241-64;player_vx=0;}}
 for(i=0,bit=1;i<3;++i,bit<<=1){x=(u16)rapids_spawn[15+i][0]*256;y=(u16)rapids_spawn[15+i][1]*256;
 if(rapid_bomb_rise[i]){--rapid_bomb_rise[i];combat_dirty=1;}
 if(!(rapid_bombs&bit) && RapidObjectTouch(x,y,240,240)){rapid_bombs|=bit;rapid_bomb_rise[i]=40;++boobus_bombs;combat_dirty=1;}}
}
void RapidLoseBombs(void) __banked{u8 bit;for(bit=1;bit<8;bit<<=1)if((rapid_bombs&bit) && boobus_bombs)--boobus_bombs;rapid_bombs=0;}
static void RapidPut(DrawObject* d,u16 x,u16 y,u8 f){d->x=x;d->y=y;d->frame=f;d->visible=1;}
void RapidCollect(DrawObject* out) __banked
{
 u8 i,n=0,bit;u16 x,y;CombatObject* a;for(i=0;i<11;++i)out[i].visible=0;
 for(i=0;i<10 && n<11;++i){a=rapid_actors+i;if(!a->active || (enemy_mode&2))continue;x=(a->x>>4)-(a->state==1?0:24);y=(a->y>>4)-(a->state==1?0:4);
 if((i16)(x-camera_px)<-72 || (i16)(x-camera_px)>=280 || (i16)(y-camera_py)<-36 || (i16)(y-camera_py)>=192)continue;RapidPut(out+n++,x,y,a->frame);}
 if(tomato_count && tomatoes[0].active && !(enemy_mode&2) && n<11)RapidPut(out+n++,tomatoes[0].x>>4,tomatoes[0].y>>4,tomatoes[0].frame);
 for(i=0,bit=1;i<2 && n<11;++i,bit<<=1){if((rapid_keys&bit) && !rapid_rise[i])continue;x=(u16)rapids_spawn[11+i][0]*16;y=(u16)rapids_spawn[11+i][1]*16;if(rapid_rise[i])y-=(40-rapid_rise[i])/2;
 if((i16)(x-camera_px)<-16 || (i16)(x-camera_px)>=256 || (i16)(y-camera_py)<-8 || (i16)(y-camera_py)>=192)continue;RapidPut(out+n++,x,y,rapid_rise[i]?105:86+((simulation_ticks/20)&1));}
 for(i=0;i<2 && n<11;++i){if(rapid_doors[i]==2)continue;x=(u16)rapids_spawn[13+i][0]*16;y=(u16)(rapids_spawn[13+i][1]-2)*16-(u16)rapid_door_ticks[i]*2;
 if((i16)(x-camera_px)<-16 || (i16)(x-camera_px)>=256 || (i16)(y-camera_py)<-64 || (i16)(y-camera_py)>=192)continue;RapidPut(out+n++,x,y,88);}
 for(i=0,bit=1;i<3 && n<11;++i,bit<<=1){if((rapid_bombs&bit) && !rapid_bomb_rise[i])continue;x=(u16)rapids_spawn[15+i][0]*16;y=(u16)rapids_spawn[15+i][1]*16;if(rapid_bomb_rise[i])y-=(40-rapid_bomb_rise[i])/2;
 if((i16)(x-camera_px)<-16 || (i16)(x-camera_px)>=256 || (i16)(y-camera_py)<-16 || (i16)(y-camera_py)>=192)continue;RapidPut(out+n++,x,y,rapid_bomb_rise[i]?89:102+(simulation_ticks/20)%3);}
}

