#ifndef KEEN_ACTORNEXT_INC
#define KEEN_ACTORNEXT_INC
#include "msxgl.h"
// First active actor with index >= i (count if none) among `ca_r` 16-byte CombatObject records at `ca_x`
// (`active` at +10). Per-module copy (the fixed bank is full); callers set ca_x/ca_r before each call.
extern u16 ca_x,ca_r,ca_wx,ca_wy;
static u8 ActorNext(u8 i) __naked
{
 i;
 __asm
 ld c,a
 ld a,(_ca_r)
 sub c
 jr c,an_none
 jr z,an_none
 ld b,a
 ld l,c
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,(_ca_x)
 add hl,de
 ld de,#10
 add hl,de
 ld de,#16
an_l:
 ld a,(hl)
 or a
 jr nz,an_found
 add hl,de
 inc c
 djnz an_l
an_none:
 ld a,(_ca_r)
 ret
an_found:
 ld a,c
 ret
 __endasm;
}
#define ACTOR_FIRST(base,n) (ca_x=(u16)(base),ca_r=(n),ActorNext(0))
#define ACTOR_AFTER(base,n,i) (ca_x=(u16)(base),ca_r=(n),ActorNext((i)+1))
#endif
