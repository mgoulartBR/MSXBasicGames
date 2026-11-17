// SPDX-License-Identifier: GPL-2.0-or-later
// Original Melon Mines roster. Included after Hill/Bridge in bank 15.
#include "actornext.inc"
#include "mines.h"
#include "mines_spawns.h"
#include "mines_boxes.h"
#include "cart.h"
#include "keydoor.h"
#include "combat_assets.h"
#include "benchmark.h"
static u8 MinesBase(CombatObject* a){return a->dir?112:108;}
void MinesSelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;
 if(active_level!=2 || bench_requested)return;
 for(i=0;i<9;++i){
  if(i<6){a=mines_walkers+i;x=a->x>>4;y=a->y>>4;}
  else{x=(u16)mines_spawn[i][0]*16;y=(u16)mines_spawn[i][1]*16;}
  x-=camera_px;y-=camera_py;
  if(i<6){
   if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
   else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;
  }else{
   if(mines_active[i-6]){if(x<-96 || x>352 || y<-96 || y>288)mines_active[i-6]=0;}
   else if(x>=-48 && x<=304 && y>=-48 && y<=240)mines_active[i-6]=1;
  }
 }
 for(i=0;i<8;++i)wall_key[i]=65535;
}
static u8 MinesTouch(CombatObject* a,u16 x,u16 y,u8 w,u8 h)
{
 const i8* b=mines_boxes[a->frame-108];
 return (i16)x<=(i16)(a->x>>4)+b[2] && (i16)(x+w)>(i16)(a->x>>4)+b[0] &&
        (i16)y<=(i16)(a->y>>4)+b[3] && (i16)(y+h)>(i16)(a->y>>4)+b[1];
}
u8 MinesHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=2 || bench_requested)return 0;
 for(i=0;i<6;++i){a=mines_walkers+i;if(!a->active || a->state==1)continue;
  if(i<4){if(!BridgeTouch(a,shot->x>>4,shot->y>>4,8,8))continue;}
  else if(!MinesTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
  if(i>=4)a->y-=256; // Preserve the feet when a 16px Aspar becomes a 32px flower.
  a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;
  shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;
 }
 // Melon Lips and hostile seeds are not PowerContact transformable classes.
 return 0;
}
static void MinesAspar(CombatObject* a)
{
 u16 ox,oy,mid,bottom,surface;u8 wall,found=0;i8 row; i16 delta;
 if(a->state==1){
  if(!--a->timer){a->state=0;a->frame=MinesBase(a);a->tick=0;a->y+=256;a->vy=0;a->ground=0;++flower_returns;return;}
  a->frame=26+((a->timer/20)&3);++a->tick;BridgeFall(a);return;
 }
 if(!a->ground){
  bottom=a->y+240;mid=a->x+312;++a->tick;
  if(a->tick&1){a->vy+=3;if(a->vy>80)a->vy=80;}a->y+=a->vy;
  for(row=(bottom>>8)-1;row<=(i8)((a->y+240)>>8);++row){wall=Wall(mid>>8,row,0);if(!wall)continue;
   surface=(u16)row*256+Height(wall,(mid>>4)&15)-1;
   if((i16)(surface-bottom)>=-16 && (i16)(surface-a->y-240)<=0){a->y=surface-240;a->vy=0;a->ground=1;a->tick=0;break;}
  }
 }else if(++a->tick>=3){
  a->tick=0;ox=a->x;oy=a->y;bottom=oy+240;delta=a->dir?100:-100;a->x+=delta;mid=a->x+312;
  if(Wall((a->x+(a->dir?624:0))>>8,a->y>>8,a->dir?3:1) ||
     Wall((a->x+(a->dir?624:0))>>8,(a->y+240)>>8,a->dir?3:1)){a->x=ox;a->dir^=1;a->frame=MinesBase(a);}
  else{
   for(row=(bottom>>8)-1;row<=(i8)((bottom+116)>>8);++row){wall=Wall(mid>>8,row,0);if(!wall)continue;
    surface=(u16)row*256+Height(wall,(mid>>4)&15)-1;
    if((i16)(surface-bottom)>=-116 && (i16)(surface-bottom)<=116){a->y=surface-240;found=1;break;}
   }
   if(!found){a->x=ox;a->y=oy;a->dir^=1;a->frame=MinesBase(a);}
   else a->frame=MinesBase(a)+((a->frame-MinesBase(a)+1)&3);
  }
 }
 if(MinesTouch(a,(player_x>>4)+4,(player_y>>4)+3,16,28))BridgeKill();
}
static void MinesMelon(u8 i)
{
 u8 n,down=mines_spawn[i+6][2]==52;MinesSeed* s;
 if(!mines_active[i])return;
 if(mines_timer[i])--mines_timer[i];
 if(mines_timer[i]==6){
  for(n=0;n<4;++n){s=mines_seeds+n;if(s->active)continue;
   s->x=(u16)mines_spawn[i+6][0]*256+(down?128:0);
   s->y=(u16)mines_spawn[i+6][1]*256+(down?384:128);
   mines_random=mines_random*33+17;
   s->vx=down?0:48+(mines_random>>4);s->vy=down?20:-20;
   s->active=1;s->tick=0;s->frame=down?142:140;break;
  }
 }
 if(!mines_timer[i])mines_timer[i]=212; // 200 waiting + 6 windup + 6 follow-through.
}
static void MinesShot(MinesSeed* s)
{
 u16 ox=s->x,oy=s->y,bottom=s->y+112,mid,surface;u8 wall,mx;
 if(!s->active)return;
 s->x+=s->vx;s->y+=s->vy;if(++s->tick&1){s->vy+=3;if(s->vy>80)s->vy=80;}
 if(!(s->tick&3))s->frame^=1;
 if((i16)((s->x>>4)-camera_px)<-64 || (i16)((s->x>>4)-camera_px)>320 ||
    (i16)((s->y>>4)-camera_py)<-64 || (i16)((s->y>>4)-camera_py)>256){s->active=0;return;}
 mx=(s->x+(s->vx>0?112:0))>>8;
 if(s->vx && Wall(mx,(s->y+48)>>8,s->vx>0?3:1)){s->x=ox;s->vx=-s->vx/2;}
 mid=s->x+56;mx=mid>>8;
 if(s->vy<0 && Wall(mx,s->y>>8,2)){s->y=oy;s->vy=-s->vy/2;}
 wall=Wall(mx,(s->y+112)>>8,0);
 if(wall && s->vy>=0){surface=((s->y+112)&0xff00)+Height(wall,(mid>>4)&15);
  if(s->y+112>=surface && bottom<=surface+80){s->y=surface-113;MinesBounce(s,wall);}
 }
 if(s->active && (i16)(player_x+64-s->x)<128 && (i16)(s->x-player_x-64)<256 &&
    (i16)(player_y+48-s->y)<128 && (i16)(s->y-player_y-48)<448)BridgeKill();
}
void MinesTick(void) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=2 || bench_requested)return;
 for(i=ACTOR_FIRST(mines_walkers,6);i<6;i=ACTOR_AFTER(mines_walkers,6,i)){a=mines_walkers+i;combat_dirty=1;
  if(i<4)BridgeWalker(a,0);else MinesAspar(a);if(death_ticks)return;
 }
 for(i=0;i<3;++i)if(mines_active[i]){combat_dirty=1;MinesMelon(i);}
 for(i=0;i<4;++i)if(mines_seeds[i].active){combat_dirty=1;MinesShot(mines_seeds+i);if(death_ticks)return;}
}
