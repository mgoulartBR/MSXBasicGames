// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from kd_act1/2: Tater, Broccolash and Melon Lips.
// Included in bank 15 after tomato.c, sharing the cached map collision helpers.
#include "actornext.inc"
#include "bridge.h"
#include "bridge_boxes.h"
#include "bridge_spawns.h"
static u8 BridgeBase(u8 i,u8 right)
{
 return i<3?(right?124:130):i<5?(right?116:108):(i==5?138:136);
}
void BridgeReset(void) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=3)return;
 for(i=0;i<11;++i){
  a=bridge_actors+i;a->x=a->y=a->vx=a->vy=a->timer=0;
  a->active=a->frame=a->ground=a->state=a->dir=a->tick=0;
  if(i<7){a->x=(u16)bridge_spawn[i][0]*256;a->y=(u16)bridge_spawn[i][1]*256;
   if(i<5)a->y-=512;if(i<3)a->y+=15;
   a->dir=1;a->frame=BridgeBase(i,1);a->ground=0;
   if(i>=5)a->timer=i==5?90:150;
  }
 }
 bridge_random=73;
}
void BridgeSelect(void) __banked
{
 u8 i,awake=0;CombatObject* a;i16 x,y;
 if(active_level!=3)return;

 for(i=0;i<7;++i){a=bridge_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
  if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
  else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;
  awake|=a->active;
 }
 for(i=7;i<11;++i)awake|=bridge_actors[i].active;
 if(!awake)return;

 for(i=0;i<8;++i)wall_key[i]=65535;
}
static u8 BridgeTouch(CombatObject* a,u16 x,u16 y,u8 w,u8 h)
{
 const i8* b=bridge_boxes[a->frame-108];
 i16 l=(a->x>>4)+b[0],t=(a->y>>4)+b[1];
 return (i16)x<=((i16)(a->x>>4)+b[2]) && (i16)(x+w)>l &&
        (i16)y<=((i16)(a->y>>4)+b[3]) && (i16)(y+h)>t;
}
static void BridgeKill(void)
{
 if(death_ticks)return;
 death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;
}
u8 BridgeHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=3)return 0;
 for(i=0;i<5;++i){a=bridge_actors+i;
  if(a->active && a->state!=1 && BridgeTouch(a,shot->x>>4,shot->y>>4,8,8)){
   a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;
   shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;
  }
 }
 // Melon Lips are not in the original PowerContact transformable classes.
 return 0;
}
static void BridgeWalk(CombatObject* a,i16 dx)
{
 u16 ox=a->x,oy=a->y,bottom=a->y+496,mid,surface;u8 mx,wall,found=0; i8 row;
 a->x+=dx;mid=a->x+248;mx=mid>>8;
 if(dx && (Wall((a->x+(dx>0?432:32))>>8,(a->y+64)>>8,dx>0?3:1) ||
           Wall((a->x+(dx>0?432:32))>>8,(a->y+464)>>8,dx>0?3:1))){a->x=ox;a->dir^=1;return;}
 for(row=(bottom>>8)-1;row<=(i8)((bottom+144)>>8);++row){
  wall=Wall(mx,row,0);if(!wall)continue;
  surface=(u16)row*256+Height(wall,(mid>>4)&15)-1;
  if((i16)(surface-bottom)>=-144 && (i16)(surface-bottom)<=144){a->y=surface-496;found=1;break;}
 }
 if(!found){a->x=ox;a->y=oy;a->dir^=1;}
}
static void BridgeFall(CombatObject* a)
{
 u16 bottom=a->y+496,mid=a->x+184,surface;u8 wall; i8 row;
 if(!a->vy && a->ground)return;
 if(a->tick&1){a->vy+=3;if(a->vy>80)a->vy=80;}
 a->y+=a->vy;
 for(row=(bottom>>8)-1;row<=(i8)((a->y+496)>>8);++row){wall=Wall(mid>>8,row,0);if(!wall)continue;
  surface=(u16)row*256+Height(wall,(mid>>4)&15)-1;
  if((i16)(surface-bottom)>=-16 && (i16)(surface-a->y-496)<=0){a->y=surface-496;a->vy=0;a->ground=1;break;}
 }
}
static void BridgeFlower(CombatObject* a,u8 i)
{
 if(!--a->timer){a->state=0;a->frame=BridgeBase(i,a->dir);a->tick=0;a->vy=0;++flower_returns;return;}
 a->frame=26+((a->timer/20)&3);++a->tick;BridgeFall(a);
}
static void BridgeWalker(CombatObject* a,u8 i)
{
 u8 base=BridgeBase(i,a->dir),danger=0; i16 delta,vertical;
 if(a->state==1){BridgeFlower(a,i);return;}
 if(!a->ground){++a->tick;BridgeFall(a);if(a->ground)a->tick=0;return;}
 // Brocco smash9: six ticks of grounded recovery, then the original step.
 if(i>=3 && a->state==3){
  a->frame=base;if(++a->tick>=6){BridgeWalk(a,a->dir?128:-128);a->state=0;a->tick=0;a->frame=BridgeBase(i,a->dir);}return;
 }
 if(a->state==2){
  ++a->tick;
  if(i<3){
   if(a->tick>=40){a->state=0;a->tick=0;a->frame=base;return;}
   danger=a->tick>=12 && a->tick<32;a->frame=base+4+danger;
  }else{
   if(a->tick>=40){a->state=3;a->tick=0;a->frame=base;return;}
   if(a->tick<3)a->frame=base+4;
   else if(a->tick<6)a->frame=base+5;
   else if(a->tick<9)a->frame=base+6;
   else if(a->tick<16)a->frame=base+7;
   else if(a->tick<22)a->frame=base+6;
   else if(a->tick<28)a->frame=base+5;
   else if(a->tick<34)a->frame=base+4;
   else a->frame=base;
   danger=a->tick>=6 && a->tick<16;
  }
  if(danger && BridgeTouch(a,(player_x>>4)+4,(player_y>>4)+3,16,28))BridgeKill();
  return;
 }
 if(++a->tick<(i<3?10:7))return;
 a->tick=0;vertical=(i16)(player_y-a->y);
 if(vertical>-496 && vertical<496){
  delta=a->dir?(i16)(player_x+64-a->x-(i<3?432:368)):(i16)(a->x-player_x-320);
  if(delta<-128){a->dir^=1;a->frame=BridgeBase(i,a->dir);}
  else if(delta<=(i<3?256:768)){a->state=2;a->frame=base+4;return;}
 }
 base=BridgeBase(i,a->dir);a->frame=base+((a->frame-base+1)&3);
 BridgeWalk(a,a->dir?128:-128);
 if(base!=BridgeBase(i,a->dir))a->frame=BridgeBase(i,a->dir);
}
static void BridgeSeed(CombatObject* a)
{
 u16 ox=a->x,oy=a->y,mid,bottom=a->y+112,surface;u8 wall,mx;
 a->x+=a->vx;a->y+=a->vy;
 if((a->tick+1)&1){a->vy+=3;if(a->vy>80)a->vy=80;}
 a->frame=(a->dir?140:142)+((++a->tick>>2)&1);
 if((i16)((a->x>>4)-camera_px)<-64 || (i16)((a->x>>4)-camera_px)>320 ||
    (i16)((a->y>>4)-camera_py)<-64 || (i16)((a->y>>4)-camera_py)>256){a->active=0;return;}
 mx=(a->x+(a->vx>0?112:0))>>8;
 if(a->vx && Wall(mx,(a->y+48)>>8,a->vx>0?3:1)){a->x=ox;a->vx=-a->vx/2;}
 mid=a->x+56;mx=mid>>8;
 if(a->vy<0 && Wall(mx,a->y>>8,2)){a->y=oy;a->vy=-a->vy/2;}
 wall=Wall(mx,(a->y+112)>>8,0);
 if(wall && a->vy>=0){surface=((a->y+112)&0xff00)+Height(wall,(mid>>4)&15);
  if(a->y+112>=surface && bottom<=surface+80){a->y=surface-113;BridgeBounce(a,wall);
  }
 }
 if(a->active && BridgeTouch(a,(player_x>>4)+4,(player_y>>4)+3,16,28))BridgeKill();
}
void BridgeTick(void) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=3)return;
 for(i=ACTOR_FIRST(bridge_actors,11);i<11;i=ACTOR_AFTER(bridge_actors,11,i)){a=bridge_actors+i;
  combat_dirty=1;
  if(i<5)BridgeWalker(a,i);else if(i<7)BridgeMelon(a,i);else BridgeSeed(a);
  if(death_ticks)return;
 }
}
void BridgeCollect(DrawObject* out) __banked
{
 u8 i;CombatObject* a;
 for(i=0;i<11;++i,++out){a=bridge_actors+i;out->visible=0;
  if(!a->active || (enemy_mode&2))continue;
  if((i16)((a->x>>4)-camera_px)<-64 || (i16)((a->x>>4)-camera_px)>=280 ||
     (i16)((a->y>>4)-camera_py)<-32 || (i16)((a->y>>4)-camera_py)>=192)continue;
  out->x=(a->x>>4)-(a->state==1 && i<5?0:24);out->y=(a->y>>4)-(a->state==1 && i<5?0:4);
  out->frame=a->frame;out->visible=1;
 }
}
