// SPDX-License-Identifier: GPL-2.0-or-later
// Original Castle Tuberia (map 16), with shared accepted actor physics.
#include "objtouch.inc"
#include "ticks.h"
#include "castle.h"
#include "castle_spawns.h"
#include "castle_boxes.h"
#include "castle_bonus.h"
#include "campaign.h"
#include "combat_assets.h"
#include "game.h"
#include "grove.h"
#include "pass.h"
#include "city.h"
#include "tomato.h"
#include "keydoor.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static u8 CastleBase(u8 i){return i<7?124:i<10?112:i<13?152:i<15?116:136;}
static const i8* CastleBox(u8 f){return castle_boxes[f>=152?30+f-152:f>=136?28+f-136:f>=124?16+f-124:f>=116?8+f-116:f-108];}
void CastleLoad(void) __banked {
 u8 i;for(i=92;i<96;++i)CastleAtlas(i);
 GroveCopy(96,(const u8*)0x8000,world_map,7100);GroveCopy(96,(const u8*)0xa000,world_collision,1168);
}
void CastleReset(void) __banked {
 u8 i;CombatObject*a;if(active_level!=12)return;
 for(i=0;i<17;++i){a=castle_actors+i;a->x=(u16)castle_spawn[i][0]*256;a->y=(u16)castle_spawn[i][1]*256-(i<7?497:i>=10 && i<15?512:0);
 a->vx=a->vy=a->timer=0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=CastleBase(i);castle_resume[i]=0;}
 castle_keys=castle_overflow=0;pass_random=world_map[8635]=73;
 for(i=0;i<2;++i)castle_rise[i]=castle_doors[i]=castle_door_ticks[i]=0;
 for(i=0;i<3;++i)pass_fry[i].active=0;
}
// Z80 version of the per-actor window test (camera-relative, 1/16 px record units >> 4).
void CastleSelectAsm(void) __naked
{
 __asm
 push ix
 ld ix,#(_world_map+7200)
 ld b,#17
csl_l:
 push bc
 call csl_one
 pop bc
 ld de,#16
 add ix,de
 djnz csl_l
 pop ix
 ret
csl_one:
 ld a,(ix+13)
 cp #1
 jr nz,csl_n
 ld (ix+10),#1
 ret
csl_n:
 ld l,(ix+0)
 ld h,(ix+1)
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 ld de,(_camera_px)
 or a
 sbc hl,de
 ld a,(ix+10)
 or a
 jr z,csl_off
 ld de,#96
 add hl,de
 ld de,#448
 ex de,hl
 or a
 sbc hl,de
 jr c,csl_kill
 call csl_y
 ld de,#96
 add hl,de
 ld de,#384
 ex de,hl
 or a
 sbc hl,de
 ret nc
csl_kill:
 ld (ix+10),#0
 ret
csl_off:
 ld de,#48
 add hl,de
 ld de,#352
 ex de,hl
 or a
 sbc hl,de
 ret c
 call csl_y
 ld de,#48
 add hl,de
 ld de,#288
 ex de,hl
 or a
 sbc hl,de
 ret c
 ld (ix+10),#1
 ret
csl_y:
 ld l,(ix+2)
 ld h,(ix+3)
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 ld de,(_camera_py)
 or a
 sbc hl,de
 ret
 __endasm;
}
// First active Castle actor with index >= i (17 if none): the 17 records (16 bytes, `active` at +10) at world_map+7200.
u8 CastleNext(u8 i) __naked
{
 i;
 __asm
 cp #17
 jr nc,cn_none
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,#(_world_map+7200+10)
 add hl,de
 ld c,a
 ld a,#17
 sub c
 ld b,a
 ld de,#16
cn_l:
 ld a,(hl)
 or a
 jr nz,cn_found
 add hl,de
 inc c
 djnz cn_l
cn_none:
 ld a,#17
 ret
cn_found:
 ld a,c
 ret
 __endasm;
}
void CastleSelect(void) __banked {
 if(active_level!=12)return;
 CastleSelectAsm();
 EnemyCache();
}
static u8 CastleWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static void CastleGrape(CombatObject*a,u8 i){
 u8 row,end;u16 old;
 if(a->state==1){if(--a->timer){a->frame=26+((a->timer/20)&1);if(!a->ground)GroveFall(a,31);return;}
 a->y+=128;a->state=castle_resume[i];a->vy=0;a->ground=0;a->frame=a->state==2?137:136;if(a->state==4)a->timer=30;++flower_returns;return;}
 if(a->state==0){if(player_x+64>a->x+368 || player_x+304<a->x || player_y<a->y)return;
 row=(a->y+368)>>8;end=(player_y+48)>>8;for(;row<end;++row)if(CastleWall((a->x+184)>>8,row,0))return;
 a->state=2;a->frame=137;a->ground=0;a->vy=0;
 }else if(a->state==2){GroveFall(a,23);if(a->ground){a->vy=-(a->vy*2)/3;if(a->vy>-32){a->state=4;a->timer=30;a->vy=0;a->frame=136;}else a->ground=0;}}
 else if(a->state==4){if(!--a->timer)a->state=5;}
 else if(a->state==5){old=a->y;a->y-=16;if(CastleWall((a->x+184)>>8,a->y>>8,2)){a->y=old;a->state=0;}}
 if(a->state==2 && (i16)(player_x+64-a->x)<384 && (i16)(a->x-player_x-64)<256 && (i16)(player_y+48-a->y)<384 && (i16)(a->y-player_y-48)<448 && !death_ticks){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}
}
void CastleTick(void) __banked {
 u8 i;CombatObject*a;if(active_level!=12)return;
 for(i=CastleNext(0);i<17;i=CastleNext(i+1)){a=castle_actors+i;combat_dirty=1;
 if(i<10)EnemyStep(a,i>=7);else if(i<13)CityFrenchTick(a);else if(i<15)GroveSquasher(a);else CastleGrape(a,i);
 if(death_ticks)return;}CityFriesTick();
}
u8 CastleHit(CombatObject*shot) __banked {
 u8 i;CombatObject*a;const i8*b;if(active_level!=12)return 0;
 for(i=0;i<17;++i){a=castle_actors+i;if(!a->active || a->state==1)continue;b=CastleBox(a->frame);
 if((i16)(shot->x>>4)>(i16)(a->x>>4)+b[2] || (i16)((shot->x>>4)+8)<=(i16)(a->x>>4)+b[0] || (i16)(shot->y>>4)>(i16)(a->y>>4)+b[3] || (i16)((shot->y>>4)+8)<=(i16)(a->y>>4)+b[1])continue;
 castle_resume[i]=a->state;if(i>=7 && i<10)a->y-=256;else if(i>=15)a->y-=128;
 a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}return 0;
}
extern u16 ca_x,ca_r,ca_wx,ca_wy;
u8 BonusList(u8* idx);
static u8 CastleVisible(u16 x,u16 y,u8 f){const SpriteDef*sp=sprites[11]+f;return (i16)(x+SPR_OX(sp)-camera_px)>-(i16)sp->w && (i16)(x+SPR_OX(sp)-camera_px)<256 && (i16)(y+SPR_OY(sp)-camera_py)>-(i16)sp->h && (i16)(y+SPR_OY(sp)-camera_py)<(hardware_scroll?192:176);}
static void CastlePut(DrawObject*out,u8*n,u16 x,u16 y,u8 f){if(!CastleVisible(x,y,f))return;if(*n>=13){castle_overflow=1;return;}out+=(*n)++;out->x=x;out->y=y;out->frame=f;out->visible=1;}
void CastleCollect(DrawObject*out) __banked {
 u8 i,n=0;CombatObject*a;Tomato*t;
 for(i=0;i<13;++i)out[i].visible=0;castle_overflow=0;
 if(!(enemy_mode&2)){
 for(i=0;i<17;++i){a=castle_actors+i;if(a->active)CastlePut(out,&n,(a->x>>4)-(i>=15 || a->state==1?0:24),(a->y>>4)-(i>=15 || a->state==1?0:4),a->frame);}
 for(i=0;i<5;++i){t=tomatoes+i;if(t->active)CastlePut(out,&n,t->x>>4,t->y>>4,t->frame);}
 for(i=0;i<3;++i){a=pass_fry+i;if(a->active)CastlePut(out,&n,(a->x>>4)-24,(a->y>>4)-4,a->frame);}}
 for(i=0;i<2;++i){if(!(castle_keys&(1<<i)) || castle_rise[i])CastlePut(out,&n,(u16)castle_spawn[17+i][0]*16,(u16)castle_spawn[17+i][1]*16-(castle_rise[i]?(40-castle_rise[i])/2:0),castle_rise[i]?105:86+((simulation_ticks/20)&1));
 if(castle_doors[i]!=2)CastlePut(out,&n,(u16)castle_spawn[19+i][0]*16,((u16)castle_spawn[19+i][1]-2)*16-(u16)castle_door_ticks[i]*2,88);}
}
static void CastleStaticPut(DrawObject*out,u8*n,u16 x,u16 y,u8 f){DrawObject*d;if(!CastleVisible(x,y,f))return;if(*n>=12){castle_overflow=2;return;}d=out+(*n<10?4+*n:*n==10?1:28);++*n;d->x=x;d->y=y;d->frame=f;d->visible=1;}
void CastleStatic(DrawObject*out) __banked {
 u8 i,n=0;u32 bit=1;const Pickup*p=pickups;for(i=4;i<14;++i)out[i].visible=0;out[1].visible=out[28].visible=0;
 {u8 pl[32],k,cnt,m=LEVEL_PICKUPS>32?32:LEVEL_PICKUPS;ca_x=(u16)pickups;ca_r=m;ca_wx=sizeof(Pickup);ca_wy=(u16)&pickup_taken;cnt=BonusList(pl);for(k=0;k<cnt;++k){p=pickups+pl[k];CastleStaticPut(out,&n,p->x,p->y,p->frame);}for(i=32,p=pickups+32;i<LEVEL_PICKUPS;++i,++p)CastleStaticPut(out,&n,p->x,p->y,p->frame);}
 {u8 list[29],k,cnt;ca_x=(u16)castle_bonus;ca_r=29;ca_wx=5;ca_wy=(u16)&bonus_taken;cnt=BonusList(list);for(k=0;k<cnt;++k){i=list[k];CastleStaticPut(out,&n,castle_bonus[i].x,castle_bonus[i].y,94+castle_bonus[i].kind);}}
}
static u8 CastleObjectTouch(u16 x,u16 y,u16 w,u16 h){ca_wx=w;ca_wy=h;return ObjTouch(x,y);}
void CastleObjectsTick(void) __banked {
 u8 i,bit;u16 x,y;if(active_level!=12)return;if(Tick20Zero())combat_dirty=1;
 for(i=0,bit=1;i<2;++i,bit<<=1){x=(u16)castle_spawn[17+i][0]*256;y=(u16)castle_spawn[17+i][1]*256;
 if(castle_rise[i]){--castle_rise[i];combat_dirty=1;}
 if(!(castle_keys&bit) && CastleObjectTouch(x,y,240,112)){castle_keys|=bit;++key_owned;castle_rise[i]=40;combat_dirty=1;}}
 for(i=0;i<2;++i){if(castle_doors[i]==2)continue;
 if(castle_doors[i]==1){++castle_door_ticks[i];combat_dirty=1;if(castle_door_ticks[i]>=24){castle_doors[i]=2;continue;}}
 x=(u16)castle_spawn[19+i][0]*256;y=((u16)castle_spawn[19+i][1]-2)*256-(u16)castle_door_ticks[i]*32;
 if(CastleObjectTouch(x,y,240,1008)){if(!castle_doors[i] && key_owned){--key_owned;castle_doors[i]=1;combat_dirty=1;}player_x=player_x+184<x+120?x-305:x+241-64;player_vx=0;}}
}
