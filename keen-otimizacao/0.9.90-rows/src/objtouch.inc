#ifndef KEEN_OBJTOUCH_INC
#define KEEN_OBJTOUCH_INC
#include "msxgl.h"
// Player/object overlap used by the key, door and bomb objects of the levels (x in HL, y in DE; w = ca_wx,
// h = ca_wy): (i16)(px+64-x) < w && (i16)(x-px-64) < 256 && (i16)(py+48-y) < h && (i16)(y-py-48) < 448,
// as two unsigned range tests. Per-module copy (the fixed bank is full).
extern u16 ca_x,ca_r,ca_wx,ca_wy;
static u8 ObjTouch(u16 x,u16 y) __naked
{
 x;y;
 __asm
 ex de,hl
 push hl
 ld hl,(_player_x)
 or a
 sbc hl,de
 ld de,#319
 add hl,de
 push hl
 ld hl,(_ca_wx)
 ld de,#255
 add hl,de
 ex de,hl
 pop hl
 or a
 sbc hl,de
 jr nc,ot_failp
 pop de
 ld hl,(_player_y)
 or a
 sbc hl,de
 ld de,#495
 add hl,de
 push hl
 ld hl,(_ca_wy)
 ld de,#447
 add hl,de
 ex de,hl
 pop hl
 or a
 sbc hl,de
 jr nc,ot_fail
 ld a,#1
 ret
ot_failp:
 pop de
ot_fail:
 xor a
 ret
 __endasm;
}
#endif
