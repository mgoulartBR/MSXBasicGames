#include "grove.h"
#include "grove_bonus.h"
#include "combat_assets.h"
#include "campaign.h"
#include "game.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static void GroveStaticPut(DrawObject* out,u8* n,u16 x,u16 y,u8 f)
{
 DrawObject* d;const SpriteDef* sp=sprites[7]+f;if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))return;
 if(*n>=12){grove_overflow=2;return;}d=out+(*n<10?4+*n:*n==10?1:28);++*n;d->x=x;d->y=y;d->frame=f;d->visible=1;
}
// Z80 pre-filter: indices (ascending) of the not-yet-taken bonuses that can be on screen (generous superset
// of GroveStaticPut's exact test: -382 < dx < 256, -382 < dy < 192). Returns the count; `idx` needs 34 bytes.
u8 GroveBonusList(u8* idx) __naked
{
 idx;
 __asm
 push ix
 push iy
 push hl
 pop iy
 push iy
 ld ix,#_grove_bonus
 ld b,#0
gb_loop:
 ld a,b
 and #7
 ld c,a
 ld a,b
 rrca
 rrca
 rrca
 and #31
 cp #4
 jr nc,gb_hi
 ld hl,#_bonus_taken
 add a,l
 ld l,a
 jr nc,gb_ld
 inc h
gb_ld:
 ld a,(hl)
 jr gb_t
gb_hi:
 ld a,(_world_collision+1151)
gb_t:
 inc c
gb_sh:
 dec c
 jr z,gb_tz
 rrca
 jr gb_sh
gb_tz:
 and #1
 jr nz,gb_next
 ld l,(ix+0)
 ld h,(ix+1)
 ld de,(_camera_px)
 or a
 sbc hl,de
 ld de,#382
 add hl,de
 ld de,#638
 or a
 sbc hl,de
 jr nc,gb_next
 ld l,(ix+2)
 ld h,(ix+3)
 ld de,(_camera_py)
 or a
 sbc hl,de
 ld de,#382
 add hl,de
 ld de,#574
 or a
 sbc hl,de
 jr nc,gb_next
 ld (iy+0),b
 inc iy
gb_next:
 ld de,#5
 add ix,de
 inc b
 ld a,b
 cp #34
 jr c,gb_loop
 pop de
 push iy
 pop hl
 or a
 sbc hl,de
 ld a,l
 pop iy
 pop ix
 ret
 __endasm;
}
void GroveStatic(DrawObject* out) __banked
{
 u8 i,n=0;u32 bit=1;const Pickup* p=pickups;
 for(i=4;i<14;++i)out[i].visible=0;out[1].visible=out[28].visible=0;
 for(i=0;i<LEVEL_PICKUPS;++i,++p,bit<<=1)if(!(pickup_taken&bit))GroveStaticPut(out,&n,p->x,p->y,p->frame);
 {u8 list[34],k,cnt=GroveBonusList(list);for(k=0;k<cnt;++k){i=list[k];GroveStaticPut(out,&n,grove_bonus[i].x,grove_bonus[i].y,94+grove_bonus[i].kind);}}
}
