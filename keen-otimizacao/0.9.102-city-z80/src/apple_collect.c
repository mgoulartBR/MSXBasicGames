#include "ticks.h"
#include "apple.h"
#include "apple_spawns.h"
#include "combat_assets.h"
#include "campaign.h"
#include "cart.h"
#include "game.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
extern u16 ca_x,ca_r,ca_wx,ca_wy,ca_map,ca_tile;
// Z80 version of the C original (collect the Apple actors, cart, bombs into the 13 draw records). `ca_x` = out, `ca_r` = cart frame | bomb frame<<8.
// Scratch: ca_map = records written, ca_wx/ca_wy = x/y handed to `put`. Same clip rule as the C ApplePut (verified by BENCH_PCORC).
static void AppleCollectAsm(void) __naked
{
 __asm
 push ix
 ld hl,(_ca_x)
 ld de,#5
 add hl,de
 ld b,#13
 ld de,#6
 xor a
ac_clr:
 ld (hl),a
 add hl,de
 djnz ac_clr
 ld (_world_map+9189),a
 ld (_ca_map),a
 ld (_ca_map+1),a
 ld a,(_enemy_mode)
 and #2
 jr nz,ac_cart
 ld ix,#(_world_map+9000)
 ld b,#5
 call ac_acts
 ld ix,#(_world_map+9088)
 ld b,#4
 call ac_acts
ac_cart:
 ld hl,(_carts)
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 ld (_ca_wx),hl
 ld hl,(_carts+2)
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 ld (_ca_wy),hl
 ld a,(_ca_r)
 call ac_put
 ld hl,#(_apple_spawn+18)
 ld de,#(_world_map+9162)
 ld bc,#0x0601
ac_bomb:
 ld a,(_world_map+9161)
 and c
 jr z,ac_show
 ld a,(de)
 or a
 jr z,ac_bnext
ac_show:
 ld a,(de)
 ld (_ca_tile),a
 push hl
 push de
 push bc
 ld e,(hl)
 ld d,#0
 inc hl
 ld c,(hl)
 ld l,e
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld (_ca_wx),hl
 ld l,c
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld a,(_ca_tile)
 or a
 jr z,ac_norise
 ld c,a
 ld a,#40
 sub c
 srl a
 ld e,a
 ld d,#0
 or a
 sbc hl,de
 ld (_ca_wy),hl
 ld a,#89
 jr ac_bput
ac_norise:
 ld (_ca_wy),hl
 ld a,(_ca_r+1)
ac_bput:
 call ac_put
 pop bc
 pop de
 pop hl
ac_bnext:
 inc de
 inc hl
 inc hl
 inc hl
 sla c
 dec b
 jp nz,ac_bomb
 pop ix
 ret
ac_acts:
 ld a,(ix+10)
 or a
 jr z,ac_anext
 ld e,(ix+0)
 ld d,(ix+1)
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
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
 ld a,(ix+13)
 cp #1
 jr z,ac_nooff
 push bc
 push hl
 ex de,hl
 ld de,#-24
 add hl,de
 ex de,hl
 pop hl
 ld bc,#-4
 add hl,bc
 pop bc
ac_nooff:
 ld (_ca_wx),de
 ld (_ca_wy),hl
 ld a,(ix+11)
 push bc
 call ac_put
 pop bc
ac_anext:
 ld de,#16
 add ix,de
 djnz ac_acts
 ret
ac_put:
 push af
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,(_sprites+12)
 add hl,de
 call ac_clip
 or a
 jr z,ac_pend
 ld a,(_ca_map)
 cp #13
 jr nc,ac_ovf
 ld l,a
 ld h,#0
 ld d,h
 ld e,l
 add hl,hl
 add hl,de
 add hl,hl
 ld de,(_ca_x)
 add hl,de
 ld de,(_ca_wx)
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 ld de,(_ca_wy)
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 pop af
 ld (hl),a
 inc hl
 ld (hl),#1
 ld hl,#_ca_map
 inc (hl)
 ret
ac_ovf:
 ld a,#1
 ld (_world_map+9189),a
ac_pend:
 pop af
 ret
ac_clip:
 inc hl
 inc hl
 inc hl
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 inc hl
 ld a,(hl)
 and #127
 ld e,a
 inc hl
 ld a,(hl)
 and #127
 ld d,a
 ld a,e
 ld hl,(_ca_wx)
 add a,l
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 push de
 ld de,(_camera_px)
 or a
 sbc hl,de
 pop de
 ld a,h
 dec a
 cp #0x7F
 jr c,ac_hide
 ld a,l
 add a,c
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 bit 7,h
 jr nz,ac_hide
 or h
 or l
 jr z,ac_hide
 ld hl,(_ca_wy)
 ld a,d
 add a,l
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 ld de,(_camera_py)
 or a
 sbc hl,de
 ld a,h
 or a
 jr z,ac_ylow
 bit 7,a
 jr z,ac_hide
 jr ac_ytop
ac_ylow:
 ld a,(_hardware_scroll)
 or a
 ld a,#176
 jr z,ac_lim
 ld a,#192
ac_lim:
 cp l
 jr c,ac_hide
 jr z,ac_hide
ac_ytop:
 ld a,l
 add a,b
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 bit 7,h
 jr nz,ac_hide
 or l
 jr z,ac_hide
 ld a,#1
 ret
ac_hide:
 xor a
 ret
 __endasm;
}
void AppleCollect(DrawObject* out) __banked
{
 ca_x=(u16)out;ca_r=(84+TickTenthOdd())|((u16)(102+(simulation_ticks/20)%3)<<8);
 AppleCollectAsm();
}
