// SPDX-License-Identifier: GPL-2.0-or-later
// Included after bridge.c in bank 15, sharing the original Tater behavior.
#include "hill.h"
#include "hill_spawns.h"
void HillReset(void) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=1)return;
 for(i=0;i<11;++i){a=hill_actors+i;
  a->x=(u16)hill_spawn[i][0]*256;a->y=((u16)hill_spawn[i][1]-2)*256;
  if(i>=4)a->y+=15;
  a->vx=a->vy=a->timer=0;a->active=a->state=a->tick=0;
  a->ground=i<4;a->dir=1;a->frame=i<4?21:124;
 }
}
// Z80 (Hill actors at world_map+9600, 16-byte records, then `enemy`). Scratch: the ring column
// variables ca_x/ca_r/ca_wx/ca_wy, unused while actors are selected.
extern u16 ca_x,ca_r,ca_wx,ca_wy;
void HillSelectAsm(void) __naked
{
 __asm
 push ix
 ld hl,(_camera_px)
 ld de,#0xFFA0
 add hl,de
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld (_ca_x),hl
 ld hl,(_camera_py)
 add hl,de
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld (_ca_r),hl
 ld ix,#(_world_map+9600)
 ld b,#11
hs_loop:
 push bc
 call hs_one
 pop bc
 ld de,#16
 add ix,de
 djnz hs_loop
 ld ix,#_enemy
 call hs_one
 ld hl,#_wall_key
 ld (hl),#0xFF
 ld d,h
 ld e,l
 inc de
 ld bc,#15
 ldir
 pop ix
 ret
hs_one:
 ld e,(ix+0)
 ld d,(ix+1)
 ld hl,(_ca_x)
 ex de,hl
 or a
 sbc hl,de
 ld a,(ix+10)
 or a
 jr z,hs_off
 ld de,#7183
 ex de,hl
 or a
 sbc hl,de
 jr c,hs_kill
 ld e,(ix+2)
 ld d,(ix+3)
 ld hl,(_ca_r)
 ex de,hl
 or a
 sbc hl,de
 ld de,#6159
 ex de,hl
 or a
 sbc hl,de
 ret nc
hs_kill:
 ld (ix+10),#0
 ret
hs_off:
 ld de,#768
 or a
 sbc hl,de
 ex de,hl
 ld hl,#5647
 or a
 sbc hl,de
 ret c
 ld e,(ix+2)
 ld d,(ix+3)
 ld hl,(_ca_r)
 ex de,hl
 or a
 sbc hl,de
 ld de,#768
 or a
 sbc hl,de
 ex de,hl
 ld hl,#4623
 or a
 sbc hl,de
 ret c
 ld (ix+10),#1
 ret
 __endasm;
}
// Any Tater (actors 4..10) active? (A != 0)
u8 HillAnyTater(void) __naked
{
 __asm
 ld hl,#(_world_map+9600+4*16+10)
 ld de,#16
 ld b,#7
 xor a
hat_l:
 or (hl)
 add hl,de
 djnz hat_l
 ret
 __endasm;
}
void HillSelect(void) __banked
{
 if(active_level!=1)return;
 HillSelectAsm();
}
void HillTick(void) __banked
{
 u8 i;CombatObject* a=hill_actors+4;
 if(active_level!=1)return;
 if(!HillAnyTater())return;
 for(i=4;i<11;++i,++a){if(!a->active)continue;
  combat_dirty=1;BridgeWalker(a,0);if(death_ticks)return;
 }
}
u8 HillHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;
 if(active_level!=1)return 0;
 for(i=0;i<11;++i){a=hill_actors+i;
  if(!a->active || a->state==1)continue;
  if(i<4){
   if((i16)(shot->x-a->x)>=512 || (i16)(a->x-shot->x)>=128 ||
      (i16)(shot->y-a->y)>=512 || (i16)(a->y-shot->y)>=128)continue;
  }else if(!BridgeTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
  a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=0;
  shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;
 }
 return 0;
}
// Z80: fills the 11 Hill draw records (6 bytes each) from the 11 actors (16 bytes each).
void HillCollectAsm(DrawObject* out) __naked
{
 out;
 __asm
 push ix
 push iy
 push hl
 pop iy
 ld ix,#(_world_map+9600)
 ld b,#4
hc_l1:
 push bc
 ld c,#0
 call hc_one
 pop bc
 ld de,#16
 add ix,de
 ld de,#6
 add iy,de
 djnz hc_l1
 ld b,#7
hc_l2:
 push bc
 ld c,#1
 call hc_one
 pop bc
 ld de,#16
 add ix,de
 ld de,#6
 add iy,de
 djnz hc_l2
 pop iy
 pop ix
 ret
hc_one:
 ld (iy+5),#0
 ld a,(ix+10)
 or a
 ret z
 ld a,(_enemy_mode)
 and #2
 ret nz
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
 push hl
 ld de,(_camera_px)
 or a
 sbc hl,de
 ld de,#64
 add hl,de
 ld de,#344
 or a
 sbc hl,de
 jr nc,hc_rej1
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
 push hl
 ld de,(_camera_py)
 or a
 sbc hl,de
 ld de,#32
 add hl,de
 ld de,#224
 or a
 sbc hl,de
 jr nc,hc_rej2
 pop de
 pop hl
 ld a,c
 or a
 jr z,hc_set
 ld a,(ix+13)
 cp #1
 jr z,hc_set
 ld bc,#0xFFE8
 add hl,bc
 ex de,hl
 ld bc,#0xFFFC
 add hl,bc
 ex de,hl
hc_set:
 ld (iy+0),l
 ld (iy+1),h
 ld (iy+2),e
 ld (iy+3),d
 ld a,(ix+11)
 ld (iy+4),a
 ld (iy+5),#1
 ret
hc_rej2:
 pop hl
hc_rej1:
 pop hl
 ret
 __endasm;
}
void HillCollect(DrawObject* out) __banked
{
 HillCollectAsm(out);
}
