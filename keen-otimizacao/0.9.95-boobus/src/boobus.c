// SPDX-License-Identifier: GPL-2.0-or-later
// kd_act2.c BoobusThink/GroundReact/AirReact and kd_keen.c PowerContact.
#include "boobus.h"
#include "campaign.h"
#include "game.h"
#include "grove.h"
#include "combat_assets.h"
#include "actornext.inc"
#ifndef KEEN_BOOBUS_HP
#define KEEN_BOOBUS_HP 12
#endif
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static inline u8 BoobusWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static u16 BoobusHeight(u8 s,u8 x){switch(s&7){case 0:return 256;case 1:return 0;case 2:return (u16)x*8;case 3:return 128+(u16)x*8;case 4:return (u16)x*16;case 5:return 120-(u16)x*8;case 6:return 248-(u16)x*8;default:return 240-(u16)x*16;}}
static u8 BoobusRandom(void){boobus_random=boobus_random*5+1;return boobus_random;}
void BoobusLoad(void) __banked
{
 u8 i;for(i=76;i<80;++i)BoobusAtlas(i);
 GroveCopy(80,(const u8*)0x8000,world_map,6830);GroveCopy(80,(const u8*)0xa000,world_collision,1168);
}
void BoobusReset(void) __banked
{
 u8 i;CombatObject* a=boobus_actor;if(active_level!=11)return;
 a->x=21*256;a->y=15*256;a->vx=a->vy=a->timer=0;a->active=1;a->frame=108;a->ground=a->state=a->dir=a->tick=0;
 boobus_hp=KEEN_BOOBUS_HP;boobus_start_bombs=boobus_bombs;boobus_death_time=0;boobus_random=73;
 for(i=0;i<12;++i)boobus_fx[i].active=0;
}
void BoobusRestoreBombs(void) __banked{boobus_bombs=boobus_start_bombs;}
void BoobusSelect(void) __banked{if(active_level==11)boobus_actor->active=1;}
static u8 BoobusTouch(u16 x,u16 y,u8 w,u8 h)
{
 CombatObject* a=boobus_actor;
 return (i16)(x-a->x-256)<512 && (i16)(a->x+256-x)<(i16)w*16 &&
 (i16)(y-a->y-128)<1536 && (i16)(a->y+128-y)<(i16)h*16;
}
static void BoobusEffect(u16 x,u16 y)
{
 u8 i,old=0;CombatObject* a;
 for(i=0;i<12;++i){if(!boobus_fx[i].active){old=i;break;}if(boobus_fx[i].timer>boobus_fx[old].timer)old=i;}
 a=boobus_fx+old;a->x=x;a->y=y;a->vx=a->vy=0;a->timer=0;a->frame=117;a->active=1;
}
void BoobusTick(void) __banked
{
 CombatObject* a=boobus_actor;CombatObject* f;u8 i,wall,mx;u16 ox,oy,oldbottom,newbottom,mid,surface; i16 dx=0,dy,limit,move; i8 y;
 if(active_level!=11)return;combat_dirty=1;
 for(i=ACTOR_FIRST(boobus_fx,12);i<12;i=ACTOR_AFTER(boobus_fx,12,i)){f=boobus_fx+i;
  ++f->timer;f->frame=f->timer<20?117:f->timer<40?118:f->timer<80?119:f->timer<110?120:f->timer<140?121:122;
  if(simulation_ticks&1){f->vy+=3;if(f->vy>80)f->vy=80;}f->y+=f->vy;if(f->timer>=170)f->active=0;
 }
 if(!boobus_hp){
  ++boobus_death_time;
  // Original FragThink increments temp1 twice per four-tick call: disappears
  // at the 30th call (120 ticks), then FinishThink waits another 250 ticks.
  if(boobus_death_time<=120 && !(boobus_death_time&3))BoobusEffect(a->x-256+(u16)BoobusRandom()*5,a->y-256+(u16)BoobusRandom()*5);
  if(boobus_death_time>=370){level_done=1;player_vx=player_vy=0;}return;
 }
 ox=a->x;oy=a->y;oldbottom=a->y+1648;mid=a->x+504;
 if(!a->state){
  if(++a->tick>=10){a->tick=0;
   if(a->timer)--a->timer;
   else{
    if(a->x+256>player_x+304)a->dir=0;else if(a->x+752<player_x+64)a->dir=1;
    else if(player_y+48>=a->y+1648 || player_y+496<=a->y+128){a->state=1;a->frame=116;a->vx=0;
     if(a->y<player_y){a->y+=128;a->vy=0;}else a->vy=-60;
    }
   }
   if(!a->state){dx=a->dir?128:-128;a->frame=(a->dir?112:108)+((a->frame+1)&3);}
  }
  dy=a->state?a->vy:(dx<0?-dx:dx)+16;
 }else{dx=a->vx;if(simulation_ticks&1){a->vy+=3;if(a->vy>80)a->vy=80;}dy=a->vy;}
 // Standing still on the ground (state 0, ground, dx 0): the ground pass below would add dy, find the same surface and snap
 // a->y straight back with ground=1 and vy=0, so skip it (exact: the snap result does not depend on the previous y).
 if(!dx && !a->state && a->ground)goto touch;
 a->x+=dx;
 if(dx){mx=(a->x+(dx>0?752:256))>>8;
  for(y=(a->y+128)>>8;y<=(i8)((a->y+1647)>>8);++y)if(BoobusWall(mx,y,dx>0?3:1)){a->x=ox;a->dir=!a->dir;a->timer=BoobusRandom()>>5;a->vx=0;break;}
 }
 mid=a->x+504;mx=mid>>8;a->y+=dy;newbottom=a->y+1648;a->ground=0;limit=-(dx<0?-dx:dx)-dy-16;
 if(dy>=0)for(y=(oldbottom>>8)-1;y<=(i8)(newbottom>>8);++y){wall=BoobusWall(mx,y,0);if(!wall)continue;
  surface=(u16)y*256+BoobusHeight(wall,(mid>>4)&15)-1;move=(i16)(surface-newbottom);
  if(move<0 && move>=limit){a->y+=move;a->ground=1;a->vy=0;break;}
 }
 if(dy<0 && BoobusWall(mx,(a->y+128)>>8,2)){a->y=oy;a->vy=0;}
 if(a->ground){if(a->state){a->state=0;a->tick=0;a->frame=a->dir?112:108;}}
 else if(!a->state){a->state=1;a->frame=116;
  if(a->y+1648>=player_y+496){a->x=ox;a->y=oy;a->vy=-60;a->vx=a->dir?24:-24;}else a->vx=a->vy=0;
 }
touch:
 if(BoobusTouch(player_x+64,player_y+48,16,28)){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);}
}
u8 BoobusHit(CombatObject* s) __banked
{
 if(active_level!=11 || !boobus_hp || !BoobusTouch(s->x,s->y,16,16))return 0;
 --boobus_hp;s->state=3;s->timer=30;s->frame=117;s->vx=s->vy=0;combat_dirty=1;
 if(!boobus_hp){boobus_actor->state=2;boobus_actor->frame=116;boobus_death_time=0;GivePoints(50000);GivePoints(50000);}
 return 1;
}
void BoobusBombTick(CombatObject* s) __banked
{
 if(s->state==3){if(!--s->timer)s->active=0;else s->frame=117+((30-s->timer)/5&1);return;}
 s->frame=s->state==2?102:102+((simulation_ticks/5&1)?2:0);
}
void BoobusStatic(DrawObject* out) __banked{u8 i;for(i=4;i<14;++i)out[i].visible=0;out[1].visible=out[28].visible=0;}
void BoobusCollect(DrawObject* out) __banked
{
 u8 i,n=0;CombatObject* a=boobus_actor;for(i=0;i<13;++i)out[i].visible=0;if(enemy_mode&2)return;
 if(boobus_hp || boobus_death_time<120){out->x=(a->x>>4)-24;out->y=(a->y>>4)-4;out->frame=a->frame;out->visible=1;++n;}
 for(i=ACTOR_FIRST(boobus_fx,12);i<12;i=ACTOR_AFTER(boobus_fx,12,i)){a=boobus_fx+i;out[n].x=a->x>>4;out[n].y=a->y>>4;out[n].frame=a->frame;out[n].visible=1;++n;}
}
