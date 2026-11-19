// SPDX-License-Identifier: GPL-2.0-or-later
// Parsnip Pass actors and inventory; private state reuses unused packed-map RAM.
#include "objtouch.inc"
#include "actornext.inc"
#include "ticks.h"
#include "pass.h"
#include "rapids.h"
#include "game.h"
#include "pass_spawns.h"
#include "pass_boxes.h"
#include "keydoor.h"
#include "campaign.h"
#include "combat_assets.h"
#include "bridge.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static u8 PassWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static u16 PassHeight(u8 s,u8 x){switch(s&7){case 0:return 256;case 1:return 0;case 2:return (u16)x*8;case 3:return 128+(u16)x*8;case 4:return (u16)x*16;case 5:return 120-(u16)x*8;case 6:return 248-(u16)x*8;default:return 240-(u16)x*16;}}
static u8 PassBase(u8 i,u8 dir){return i<4?(dir?124:130):i==4?(dir?112:108):i<7?(dir?116:120):i==7?(dir?144:136):(dir?152:158);}
void PassReset(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=5)return;
 for(i=0;i<10;++i){a=pass_actors+i;a->x=(u16)pass_spawn[i][0]*256;a->y=(u16)pass_spawn[i][1]*256;if(i!=4)a->y-=512;if(i<4)a->y+=15;
 a->vx=a->vy=a->timer=0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=PassBase(i,1);}
 pass_keys=0;pass_random=73;
 for(i=0;i<3;++i)pass_fry[i].active=0;
 pass_rise=pass_door=pass_door_tick=0;
}
void PassSelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;if(active_level!=5)return;
 for(i=0;i<10;++i){a=pass_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
 if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
 else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;}
 EnemyCache();
}
static u8 PassTouch(CombatObject* a,u16 x,u16 y,u8 w,u8 h){const i8* b=pass_boxes[a->frame-108];return (i16)x<=(i16)(a->x>>4)+b[2] && (i16)(x+w)>(i16)(a->x>>4)+b[0] && (i16)y<=(i16)(a->y>>4)+b[3] && (i16)(y+h)>(i16)(a->y>>4)+b[1];}
static void PassKill(void){if(!death_ticks){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}}
u8 PassHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;if(active_level!=5)return 0;
 for(i=0;i<10;++i){a=pass_actors+i;if(!a->active || a->state==1 || !PassTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
 if(i==4)a->y-=256;a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}return 0;
}
static void PassFall(CombatObject* a)
{
 u16 oy=a->y,bottom=oy+496,mid=a->x+184,surface;u8 wall;i8 row;if(a->ground){a->vy=0;return;}
 if(a->tick&1){a->vy+=3;if(a->vy>80)a->vy=80;}a->y+=a->vy;
 if(a->vy<0){if(PassWall(mid>>8,a->y>>8,2)){a->y=oy;a->vy=0;}return;}
 for(row=(bottom>>8)-1;row<=(i8)((a->y+496)>>8);++row){wall=PassWall(mid>>8,row,0);if(!wall)continue;surface=(u16)row*256+PassHeight(wall,(mid>>4)&15)-1;
 if((i16)(surface-bottom)>=-16 && (i16)(surface-a->y-496)<=0){a->y=surface-496;a->vy=0;a->ground=1;return;}}
}
static void PassMove(CombatObject* a,i16 dx)
{
 u16 ox=a->x,oy=a->y,bottom=oy+496,mid,surface;u8 wall;i8 row;
 a->x+=dx;mid=a->x+248;
 if(!PassWall((a->x+(a->dir?432:32))>>8,(a->y+64)>>8,a->dir?3:1) && !PassWall((a->x+(a->dir?432:32))>>8,(a->y+464)>>8,a->dir?3:1)){
 for(row=(bottom>>8)-1;row<=(i8)((bottom+144)>>8);++row){wall=PassWall(mid>>8,row,0);if(!wall)continue;surface=(u16)row*256+PassHeight(wall,(mid>>4)&15)-1;
 if((i16)(surface-bottom)>=-144 && (i16)(surface-bottom)<=144){a->y=surface-496;return;}}}
 a->x=ox;a->y=oy;a->dir^=1;
}
static void PassWalk(CombatObject* a){PassMove(a,a->dir?128:-128);}
static void PassSquasher(CombatObject* a,u8 i)
{
 i16 dx,dy,left,right;u16 ox;
 if(a->state==1){if(!--a->timer){a->state=0;a->frame=PassBase(i,a->dir);a->tick=0;a->vy=0;++flower_returns;return;}a->frame=26+((a->timer/20)&3);++a->tick;PassFall(a);return;}
 if(a->state==2 || a->state==3){ox=a->x;a->x+=a->vx;++a->tick;
 if(PassWall((a->x+(a->vx>0?432:32))>>8,(a->y+64)>>8,a->vx>0?3:1) || PassWall((a->x+(a->vx>0?432:32))>>8,(a->y+464)>>8,a->vx>0?3:1))a->x=ox;
 PassFall(a);if(a->ground){a->state=4;a->tick=0;a->vx=0;}else if(a->state==2 && a->tick>=20)a->state=3;
 a->frame=PassBase(i,a->dir)+(a->state==2?2:3);
 }else if(a->state==4){a->frame=PassBase(i,a->dir)+3;if(++a->tick>=10){a->state=0;a->tick=0;a->frame=PassBase(i,a->dir);}}
 else if(!a->ground){++a->tick;PassFall(a);if(a->ground)a->tick=0;}
 else if(++a->tick>=10){a->tick=0;dx=player_x-a->x;dy=player_y-a->y;
 if(dy>768 || dy<-768){pass_random=pass_random*33+17;if(pass_random<8)a->dir^=1;PassWalk(a);}
 else if(a->dir?dx>1536:dx<-1536)PassWalk(a);
 else if(a->dir?dx<-128:dx>128){a->dir=1;PassWalk(a);} // Original kd_act2.c facing rule.
 else{a->vy=-50;a->vx=dx/60;a->ground=0;a->state=2;a->frame=PassBase(i,a->dir)+2;return;}
 a->frame=PassBase(i,a->dir)+((a->frame-PassBase(i,a->dir)+1)&1);
 }
 if(PassTouch(a,(player_x>>4)+4,(player_y>>4)+3,16,28)){
 if(a->state==3)PassKill();else{left=player_x+304-a->x-32;right=a->x+432-player_x-64;if(left>0 && right>0)RapidPush(left<right?-left:right);}}
}

static void PassThrow(CombatObject* a)
{
 u8 i;CombatObject* f;for(i=0;i<3;++i){f=pass_fry+i;if(f->active)continue;
 f->x=a->x+(a->dir?384:0);f->y=a->y+128;pass_random=pass_random*33+17;
 f->vx=(a->dir?40:-40)-(pass_random>>4);f->vy=-20;f->active=1;f->tick=f->state=0;f->timer=1;f->frame=164;f->ground=0;break;}
}
static void PassFrench(CombatObject* a,u8 i)
{
 u8 base=PassBase(i,a->dir);i16 dx,dy;
 if(a->state==1){if(!--a->timer){a->state=0;a->frame=base;a->tick=0;a->vy=0;++flower_returns;return;}a->frame=26+((a->timer/20)&3);++a->tick;PassFall(a);return;}
 if(a->timer)--a->timer;
 if(!a->ground){++a->tick;PassFall(a);if(a->ground)a->tick=0;return;}
 if(a->state==2){++a->tick;a->frame=base+(a->tick<10?4:5);
 if(a->tick==10){PassThrow(a);a->timer=2;}
 if(a->tick>=21){PassMove(a,a->dir?-128:128);a->state=0;a->tick=0;a->frame=PassBase(i,a->dir);}return;}
 if(a->state==3){if(++a->tick<5)return;a->tick=0;PassWalk(a);a->state=0;a->frame=PassBase(i,a->dir);return;}
 if(++a->tick<10)return;a->tick=0;
 if(!a->timer){dx=player_x-a->x;dy=player_y-a->y;
 if(dy>768 || dy<-768){pass_random=pass_random*33+17;if(pass_random<8)a->dir^=1;}
 else if(dx<-1024){a->dir=0;a->state=2;a->frame=PassBase(i,0)+4;return;}
 else if(dx<0){a->dir=1;a->state=3;a->timer=8;}
 else if(dx<1024){a->dir=0;a->state=3;a->timer=8;}
 else if(dx<2048){a->dir=1;a->state=2;a->frame=PassBase(i,1)+4;return;}
 else a->dir=1;
 }
 base=PassBase(i,a->dir);a->frame=base+((a->frame-base+1)&3);PassWalk(a);
 if(base!=PassBase(i,a->dir))a->frame=PassBase(i,a->dir);
 // Frenchy's body does not kill Keen in original KeenContact; its fries do.
}
static void PassFry(CombatObject* a)
{
 u16 ox=a->x,oy=a->y,bottom=a->y+112,mid,surface;u8 wall,mx;
 a->x+=a->vx;a->y+=a->vy;if(++a->tick&1){a->vy+=3;if(a->vy>80)a->vy=80;}a->frame=164+((a->tick>>2)&1);
 if((i16)((a->x>>4)-camera_px)<-64 || (i16)((a->x>>4)-camera_px)>320 || (i16)((a->y>>4)-camera_py)<-64 || (i16)((a->y>>4)-camera_py)>256){a->active=0;return;}
 mx=(a->x+(a->vx>0?112:0))>>8;if(a->vx && (PassWall(mx,(a->y+48)>>8,a->vx>0?3:1))){a->x=ox;a->vx=-a->vx/2;}
 mid=a->x+56;mx=mid>>8;if(a->vy<0 && PassWall(mx,a->y>>8,2)){a->y=oy;a->vy=-a->vy/2;}
 wall=PassWall(mx,(a->y+112)>>8,0);if(wall && a->vy>=0){surface=((a->y+112)&0xff00)+PassHeight(wall,(mid>>4)&15);if(a->y+112>=surface && bottom<=surface+80){a->y=surface-113;BridgeBounce(a,wall);}}
 if(a->active && PassTouch(a,(player_x>>4)+4,(player_y>>4)+3,16,28))PassKill();
}
void PassTick(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=5)return;
 for(i=ACTOR_FIRST(pass_actors,10);i<10;i=ACTOR_AFTER(pass_actors,10,i)){a=pass_actors+i;combat_dirty=1;
 if(i<5)EnemyStep(a,i==4);else if(i<7)PassSquasher(a,i);else if(i==7){if(a->frame>=136)a->frame-=28;EnemyBrocco(a);if(a->frame>=108)a->frame+=28;}else PassFrench(a,i);
 if(death_ticks)return;}
 for(i=ACTOR_FIRST(pass_fry,3);i<3;i=ACTOR_AFTER(pass_fry,3,i)){a=pass_fry+i;combat_dirty=1;PassFry(a);if(death_ticks)return;}
}
static u8 PassObjectTouch(u16 x,u16 y,u16 w,u16 h){ca_wx=w;ca_wy=h;return ObjTouch(x,y);}
void PassObjectsTick(void) __banked
{
 u16 x,y;if(active_level!=5)return;if(Tick20Zero())combat_dirty=1;
 x=4*256;y=32*256;if(pass_rise){--pass_rise;combat_dirty=1;}
 if(!pass_keys && PassObjectTouch(x,y,240,112)){pass_keys=1;++key_owned;pass_rise=40;combat_dirty=1;}
 if(pass_door==2)return;if(pass_door==1){++pass_door_tick;combat_dirty=1;if(pass_door_tick>=24){pass_door=2;return;}}
 x=33*256;y=5*256-(u16)pass_door_tick*32;
 if(PassObjectTouch(x,y,240,1008)){if(!pass_door && key_owned){--key_owned;pass_door=1;combat_dirty=1;}player_x=player_x+184<x+120?x-305:x+241-64;player_vx=0;}
}
static void PassPut(DrawObject* out,u8* count,u16 x,u16 y,u8 frame)
{
 const SpriteDef* sp=sprites[4]+frame;
 if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))return;
 if(*count>=11){pass_overflow=1;return;}out+=(*count)++;out->x=x;out->y=y;out->frame=frame;out->visible=1;
}
void PassCollect(DrawObject* out) __banked
{
 u8 i,n=0;CombatObject* a;for(i=0;i<11;++i)out[i].visible=0;pass_overflow=0;
 if(!(enemy_mode&2)){
 for(i=ACTOR_FIRST(pass_actors,10);i<10;i=ACTOR_AFTER(pass_actors,10,i)){a=pass_actors+i;PassPut(out,&n,(a->x>>4)-(a->state==1?0:24),(a->y>>4)-(a->state==1?0:4),a->frame);}
 for(i=ACTOR_FIRST(pass_fry,3);i<3;i=ACTOR_AFTER(pass_fry,3,i)){a=pass_fry+i;PassPut(out,&n,(a->x>>4)-24,(a->y>>4)-4,a->frame);}
 }
 if(!pass_keys || pass_rise)PassPut(out,&n,64,512-(pass_rise?(40-pass_rise)/2:0),pass_rise?105:86+((simulation_ticks/20)&1));
 if(pass_door!=2)PassPut(out,&n,528,80-(u16)pass_door_tick*2,88);
}

void CityFrenchTick(CombatObject* a) __banked {PassFrench(a,8);}
void CityFriesTick(void) __banked {u8 i;for(i=0;i<3;++i)if(pass_fry[i].active){combat_dirty=1;PassFry(pass_fry+i);if(death_ticks)return;}}
