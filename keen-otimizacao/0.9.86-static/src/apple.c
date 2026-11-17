// SPDX-License-Identifier: GPL-2.0-or-later
// Apple Acres: original object positions, pole checks, step timing and four pea limit.
#include "apple.h"
#include "apple_spawns.h"
#include "apple_boxes.h"
#include "campaign.h"
#include "combat_assets.h"
#include "game.h"
#include "cart.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
void EnemyStep(CombatObject*,u8) __banked;
void EnemyCache(void) __banked;
static u8 AppleRandom(void){apple_random=apple_random*13+71;return apple_random;}
static CombatObject* AppleObject(u8 i){return i<5?apple_actors+i:apple_peas+i-5;}
static u8 AppleBase(u8 i,u8 dir){return i==0?(dir?124:130):i<4?(dir?108:111):i==4?(dir?136:141):(dir?146:150);}
static const i8* AppleBox(CombatObject* a){u8 f=a->frame;return apple_boxes[f>=146?34+f-146:f>=136?24+f-136:f>=124?f-124:12+f-108];}
static u8 AppleTouch(CombatObject* a,u16 x,u16 y,u8 w,u8 h){const i8* b=AppleBox(a);return (i16)x<=(i16)(a->x>>4)+b[2] && (i16)(x+w)>(i16)(a->x>>4)+b[0] && (i16)y<=(i16)(a->y>>4)+b[3] && (i16)(y+h)>(i16)(a->y>>4)+b[1];}
static void AppleKill(void){if(!death_ticks){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}}
static u8 AppleWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static u16 AppleHeight(u8 s,u8 x){switch(s&7){case 0:return 256;case 1:return 0;case 2:return (u16)x*8;case 3:return 128+(u16)x*8;case 4:return (u16)x*16;case 5:return 120-(u16)x*8;case 6:return 248-(u16)x*8;default:return 240-(u16)x*16;}}
static u8 ApplePole(u8 x,u8 y){u16 tile;if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 0;tile=MapTile((u16)y*MAP_WIDTH+x);return world_collision[480+(tile>>3)]&(1<<(tile&7));}
void AppleReset(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=7)return;
 for(i=0;i<9;++i){a=AppleObject(i);a->x=i<5?(u16)apple_spawn[i][0]*256:0;a->y=i<5?(u16)apple_spawn[i][1]*256-(i?512:497):0;a->vx=a->vy=a->timer=0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=AppleBase(i,1);apple_resume[i]=apple_foot[i]=0;}
 apple_random=73;apple_spits=apple_bombs=apple_overflow=0;for(i=0;i<6;++i)apple_rise[i]=0;
}
void AppleSelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;if(active_level!=7)return;
 for(i=0;i<5;++i){a=apple_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
 if(a->state==1){a->active=1;continue;}if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;}
 EnemyCache();
}
u8 AppleHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;const i8* b;if(active_level!=7)return 0;
 for(i=0;i<9;++i){a=AppleObject(i);if(!a->active || a->state==1 || !AppleTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
 b=AppleBox(a);apple_resume[i]=a->state;apple_foot[i]=b[3];if(i)a->y+=(i16)b[3]*16-496;
 a->state=1;a->timer=350;a->frame=26;a->vy=0;if(i)a->vx=0;a->tick=a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}return 0;
}
// Clip a movement against original slopes and directional walls. Walking actors turn
// at a missing ledge; airborne peas land and continue walking rather than bouncing.
static void AppleMove(CombatObject* a,i16 dx,u8 height,u8 width,u8 ledge)
{
 u16 ox=a->x,oy=a->y,oldbottom=oy+(u16)height*16,mid,surface,newbottom;
 i16 dy,limit,adjust; i8 row,end;u8 wall,mx,was=a->ground;
 if(was)dy=(dx<0?-dx:dx)+16;else{if(simulation_ticks&1){a->vy+=3;if(a->vy>80)a->vy=80;}dy=a->vy;}
 a->x+=dx;a->y+=dy;mid=a->x+(u16)width*8;mx=mid>>8;newbottom=a->y+(u16)height*16;a->ground=0;limit=-(dx<0?-dx:dx)-dy-16;
 end=newbottom>>8;
 for(row=(oldbottom>>8)-1;row<=end;++row){wall=AppleWall(mx,row,0);if(!wall)continue;surface=(u16)row*256+AppleHeight(wall,(mid>>4)&15)-1;adjust=(i16)(surface-newbottom);
 if(dy>=0 && adjust<=0 && adjust>=limit){a->y+=adjust;a->ground=1;a->vy=0;break;}}
 if(ledge && was && !a->ground){a->x=ox;a->y=oy;a->ground=1;a->vy=0;a->dir^=1;return;}
 if(a->vy<0 && AppleWall(mx,a->y>>8,2)){a->y=oy;a->vy=0;}
 if(dx){mx=(a->x+(dx>0?(u16)width*16:0))>>8;
 for(row=(a->y+64)>>8;row<=(i8)((a->y+(u16)height*16-64)>>8);++row)if(AppleWall(mx,row,dx>0?3:1)){a->x=ox;a->dir^=1;a->vx=0;break;}}
}
static void AppleSpit(CombatObject* pod)
{
 u8 i;CombatObject* a;for(i=0;i<4;++i){a=apple_peas+i;if(a->active)continue;
 a->x=pod->x+(pod->dir?128:0);a->y=pod->y+128;a->vx=(pod->dir?48:-48)-(AppleRandom()>>4);a->vy=-20;a->active=1;a->ground=0;a->state=7;a->dir=pod->dir;a->tick=0;a->timer=0;a->frame=AppleBase(5+i,a->dir);break;}
}
static void AppleActor(CombatObject* a,u8 i)
{
 const i8* b;u8 x,y,top,bottom;u16 mid; i16 dx=0;
 if(a->state==1){
  if(--a->timer){a->frame=26+((a->timer/20)&1);AppleMove(a,0,31,23,0);return;}
  a->y+=496-(u16)apple_foot[i]*16;a->state=apple_resume[i];a->frame=AppleBase(i,a->dir);a->tick=0;a->vy=0;++flower_returns;
  if(a->state==5 || a->state==6){a->timer=30;a->frame=a->dir?140:145;}
  if(a->state==2 || a->state==3){a->frame=a->state==2?114:116;}
 }
 if(i<4){
  if(a->state==2 || a->state==3){
   b=AppleBox(a);mid=a->x+((i16)b[0]+b[2])*8;x=mid>>8;y=(a->y+(i16)(a->state==2?b[1]:b[3])*16)>>8;
   if(!ApplePole(x,y)){a->state=4;a->vy=0;}
   else{a->y+=a->state==2?-16:16;if(++a->tick>=6){a->tick=0;a->frame=a->state==2?114+((a->frame-114+1)&1):116+((a->frame-116+1)&3);}goto contact;}
  }
  if(a->state==4){a->frame=AppleBase(i,a->dir);AppleMove(a,0,31,23,0);if(a->ground)a->state=0;goto contact;}
 }
 if(i==4 && (a->state==5 || a->state==6)){
  AppleMove(a,0,31,23,1);if(--a->timer==0){if(a->state==5){AppleSpit(a);a->state=6;a->timer=30;}else{a->state=0;a->frame=AppleBase(i,a->dir);a->tick=0;}}return;
 }
 if(i>=5 && a->state==7){AppleMove(a,a->vx,15,14,0);if(a->ground){a->state=0;a->tick=0;a->vx=0;}goto contact;}
 if(++a->tick>=10){
  a->tick=0;dx=a->dir?128:-128;a->frame=AppleBase(i,a->dir)+(a->frame-AppleBase(i,a->dir)+1)%(i<4?3:4);
  if(i<4){b=AppleBox(a);top=(a->y+(i16)b[1]*16)>>8;bottom=(a->y+(i16)b[3]*16)>>8;mid=a->x+((i16)b[0]+b[2])*8;x=mid>>8;
   if(a->y+(i16)b[3]*16<player_y+48 || a->y+(i16)b[1]*16>player_y+496){y=a->y<player_y?bottom:top;
    if(ApplePole(x,y)){a->x=(u16)x*256;a->state=a->y<player_y?3:2;a->frame=a->state==2?114:116;a->ground=0;return;}}
   if(AppleRandom()<=32)a->dir=player_x>a->x;
  }else if(i==4 && (i16)(a->y-player_y)>=-768 && (i16)(a->y-player_y)<=768 && ((a->dir && a->x<player_x) || (!a->dir && a->x>player_x)) && apple_spits<4 && AppleRandom()<8){++apple_spits;a->state=5;a->timer=30;a->frame=a->dir?140:145;return;}
 }
 AppleMove(a,dx,i>=5?15:31,i>=5?14:23,1);
 a->frame=AppleBase(i,a->dir)+(a->frame-AppleBase(i,a->dir))%(i<4?3:4);
contact:
 if(i!=4 && AppleTouch(a,(player_x>>4)+4,(player_y>>4)+3,16,28))AppleKill();
}
void AppleTick(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=7)return;
 for(i=0;i<9;++i){a=AppleObject(i);if(!a->active)continue;combat_dirty=1;
 if(!i)EnemyStep(a,0);else AppleActor(a,i);if(death_ticks)return;
 if(i>=5 && a->state!=1 && ((i16)((a->x>>4)-camera_px)<-96 || (i16)((a->x>>4)-camera_px)>352 || (i16)((a->y>>4)-camera_py)<-96 || (i16)((a->y>>4)-camera_py)>288))a->active=0;}
}
void AppleObjectsTick(void) __banked
{
 u8 i,bit;u16 x,y;if(active_level!=7)return;if(!(simulation_ticks%20))combat_dirty=1;
 for(i=0,bit=1;i<6;++i,bit<<=1){x=(u16)apple_spawn[6+i][0]*256;y=(u16)apple_spawn[6+i][1]*256;
 if(apple_rise[i]){--apple_rise[i];combat_dirty=1;}
 if(!(apple_bombs&bit) && (i16)(player_x+64-x)<256 && (i16)(x-player_x-64)<256 && (i16)(player_y+48-y)<256 && (i16)(y-player_y-48)<448){apple_bombs|=bit;apple_rise[i]=40;++boobus_bombs;combat_dirty=1;}}
}
void AppleLoseBombs(void) __banked {u8 i,bit;for(i=0,bit=1;i<6;++i,bit<<=1)if((apple_bombs&bit) && boobus_bombs)--boobus_bombs;apple_bombs=0;}

#include "apple_bonus.h"
extern u16 ca_x,ca_r,ca_wx,ca_wy;
u8 BonusList(u8* idx);
static void AppleStaticPut(DrawObject* out,u8* n,u16 x,u16 y,u8 f)
{
 DrawObject* d;const SpriteDef* sp=sprites[6]+f;if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))return;
 if(*n>=12){apple_overflow=2;return;}d=out+(*n<10?4+*n:*n==10?1:28);++*n;d->x=x;d->y=y;d->frame=f;d->visible=1;
}
void AppleStatic(DrawObject* out) __banked
{
 u8 i,n=0;u32 bit=1;const Pickup* p=pickups;
 for(i=4;i<14;++i)out[i].visible=0;out[1].visible=out[28].visible=0;
 {u8 pl[32],k,cnt,m=LEVEL_PICKUPS>32?32:LEVEL_PICKUPS;ca_x=(u16)pickups;ca_r=m;ca_wx=sizeof(Pickup);ca_wy=(u16)&pickup_taken;cnt=BonusList(pl);for(k=0;k<cnt;++k){p=pickups+pl[k];AppleStaticPut(out,&n,p->x,p->y,p->frame);}for(i=32,p=pickups+32;i<LEVEL_PICKUPS;++i,++p)AppleStaticPut(out,&n,p->x,p->y,p->frame);}
 {u8 list[36],k,cnt;ca_x=(u16)apple_bonus;ca_r=36;ca_wx=5;ca_wy=(u16)&bonus_taken;cnt=BonusList(list);for(k=0;k<cnt;++k){i=list[k];AppleStaticPut(out,&n,apple_bonus[i].x,apple_bonus[i].y,94+apple_bonus[i].kind);}}
}
