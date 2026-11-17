// SPDX-License-Identifier: GPL-2.0-or-later
// kd_act1.c grape cycle: unobstructed sight below, falling, bounce, 30 ticks, rise.
#include "grove.h"
#include "game.h"
#include "rapids.h"
void EnemyBrocco(CombatObject*) __banked;
static u8 GroveWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static void GroveGrape(CombatObject* a,u8 i)
{
 u8 row,end;u16 old;
 if(a->state==1){if(--a->timer){a->frame=26+((a->timer/20)&1);if(!a->ground)GroveFall(a,31);return;}
 a->y+=128;a->state=grove_resume[i];a->vy=0;a->ground=0;a->frame=a->state==2?153:152;if(a->state==4)a->timer=30;++flower_returns;return;}
 if(a->state==0){
  if(player_x+64>a->x+368 || player_x+304<a->x || player_y<a->y)return;
  row=(a->y+368)>>8;end=(player_y+48)>>8;
  for(;row<end;++row)if(GroveWall((a->x+184)>>8,row,0))return;
  a->state=2;a->frame=153;a->ground=0;a->vy=0;
 }else if(a->state==2){GroveFall(a,23);
  if(a->ground){a->vy=-(a->vy*2)/3;if(a->vy>-32){a->state=4;a->timer=30;a->vy=0;a->frame=152;}else a->ground=0;}
 }else if(a->state==4){if(!--a->timer)a->state=5;}
 else if(a->state==5){old=a->y;a->y-=16;if(GroveWall((a->x+184)>>8,a->y>>8,2)){a->y=old;a->state=0;}}
 if(a->state==2 && (i16)(player_x+64-a->x)<384 && (i16)(a->x-player_x-64)<256 && (i16)(player_y+48-a->y)<384 && (i16)(a->y-player_y-48)<448){
 if(!death_ticks){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}}
}
// First active Grove actor with index >= i (39 if none): the 39 records (16 bytes, `active` at +10) at world_map+7960.
u8 GroveNext(u8 i) __naked
{
 i;
 __asm
 cp #39
 jr nc,gn_none
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,#(_world_map+7960+10)
 add hl,de
 ld c,a
 ld a,#39
 sub c
 ld b,a
 ld de,#16
gn_l:
 ld a,(hl)
 or a
 jr nz,gn_found
 add hl,de
 inc c
 djnz gn_l
gn_none:
 ld a,#39
 ret
gn_found:
 ld a,c
 ret
 __endasm;
}
void GroveTick(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=8)return;
 for(i=GroveNext(0);i<39;i=GroveNext(i+1)){a=grove_actors+i;combat_dirty=1;
 if(i<3)EnemyStep(a,i==2);else if(i==3){if(a->frame>=136)a->frame-=28;EnemyBrocco(a);if(a->frame>=108)a->frame+=28;}else if(i==4)GroveSquasher(a);else GroveGrape(a,i);
 if(death_ticks)return;}
}
