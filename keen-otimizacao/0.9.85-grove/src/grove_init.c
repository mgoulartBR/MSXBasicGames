// SPDX-License-Identifier: GPL-2.0-or-later
#include "grove.h"
#include "grove_spawns.h"
#include "grove_parts.h"
#include "game.h"
#include "rapids.h"
extern volatile u16 camera_px,camera_py;
void GroveLoad(void) __banked
{
 u8 i;for(i=0;i<sizeof(grove_parts)/sizeof(grove_parts[0]);++i)GroveDecode(grove_parts[i].bank,(const u8*)grove_parts[i].source,grove_parts[i].y,grove_parts[i].rows);
 GroveCopy(13,(const u8*)0x9900,world_map,7960);GroveCopy(13,(const u8*)0xb900,world_collision,1168);
}
void GroveReset(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=8)return;
 for(i=0;i<39;++i){a=grove_actors+i;a->x=(u16)grove_spawn[i][0]*256;a->y=(u16)grove_spawn[i][1]*256-(i>=5?0:i<2?497:i==2?0:512);
 a->vx=a->vy=a->timer=0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=i<2?124:i==2?112:i==3?144:i==4?116:152;grove_resume[i]=0;}
 grove_bombs=grove_overflow=0;for(i=0;i<3;++i)grove_rise[i]=0;world_map[8635]=73;
}
// Z80 version of the per-actor window test (camera-relative, 1/16 px record units >> 4).
void GroveSelectAsm(void) __naked
{
 __asm
 push ix
 ld ix,#(_world_map+7960)
 ld b,#39
gsl_l:
 push bc
 call gsl_one
 pop bc
 ld de,#16
 add ix,de
 djnz gsl_l
 pop ix
 ret
gsl_one:
 ld a,(ix+13)
 cp #1
 jr nz,gsl_n
 ld (ix+10),#1
 ret
gsl_n:
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
 jr z,gsl_off
 ld de,#96
 add hl,de
 ld de,#448
 ex de,hl
 or a
 sbc hl,de
 jr c,gsl_kill
 call gsl_y
 ld de,#96
 add hl,de
 ld de,#384
 ex de,hl
 or a
 sbc hl,de
 ret nc
gsl_kill:
 ld (ix+10),#0
 ret
gsl_off:
 ld de,#48
 add hl,de
 ld de,#352
 ex de,hl
 or a
 sbc hl,de
 ret c
 call gsl_y
 ld de,#48
 add hl,de
 ld de,#288
 ex de,hl
 or a
 sbc hl,de
 ret c
 ld (ix+10),#1
 ret
gsl_y:
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
void GroveSelect(void) __banked
{
 if(active_level!=8)return;
 GroveSelectAsm();
 EnemyCache();
}
