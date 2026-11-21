// Original Cantaloupe Cart: 32 units/tick; reverse at walls or missing track.
#include "cart.h"
extern volatile u16 camera_px,camera_py;
Cart carts[3];
u8 cart_riding,cart_cache_valid;
// One result per cart, keyed by both tile coordinates and direction.
// MapSet, level loading and CartReset invalidate all entries.
typedef struct {u8 lead,row; i8 dir; u8 blocked;} CartWallCache;
#define cart_wall_cache ((CartWallCache*)(world_collision+1152))
void CartReset(void)
{
    u8 i;
    static const u8 xs[3]={40,27,74},ys[3]={32,77,77};
    for(i=0;i<(active_level==7?1:3);++i){carts[i].x=(u16)xs[i]*256;carts[i].y=(u16)ys[i]*256-48;carts[i].dir=1;carts[i].delta=0;}
    if(active_level==7){carts[0].x=15U*256;carts[0].y=42U*256-48;}
    cart_riding=255;cart_cache_valid=0;
}
// Fast path for a valid wall-cache entry. HL = cart, DE = cache.
// Misses leave the cart unchanged and use the original C collision path.
static u8 CartCachedStep(Cart* c,CartWallCache* cache) __sdcccall(1) __naked
{
 c;cache;
 __asm
    push ix
    push iy
    push hl
    pop ix
    push de
    pop iy
    ld l,0(ix)
    ld h,1(ix)
    ld a,4(ix)
    cp #1
    jr nz,cart955_left
    ld bc,#32
    add hl,bc
    ld d,h
    ld e,l
    ld bc,#752
    add hl,bc
    jr cart955_lead
cart955_left:
    ld bc,#0xffe0
    add hl,bc
    ld d,h
    ld e,l
cart955_lead:
    ld a,h
    cp 0(iy)
    jr nz,cart955_miss
    ld l,2(ix)
    ld h,3(ix)
    ld bc,#288
    add hl,bc
    ld a,h
    cp 1(iy)
    jr nz,cart955_miss
    ld a,4(ix)
    cp 2(iy)
    jr nz,cart955_miss
    ld a,3(iy)
    or a,a
    jr z,cart955_move
    ld a,4(ix)
    neg
    ld 4(ix),a
    xor a,a
    ld 5(ix),a
    jr cart955_hit
cart955_move:
    ld 0(ix),e
    ld 1(ix),d
    ld a,4(ix)
    cp #1
    ld a,#32
    jr z,cart955_delta
    ld a,#0xe0
cart955_delta:
    ld 5(ix),a
cart955_hit:
    ld a,#1
    jr cart955_done
cart955_miss:
    xor a,a
cart955_done:
    pop iy
    pop ix
    ret
 __endasm;
}
void CartStep(void)
{
    u8 remaining,lead,row,bit;u16 nx;i8 step;Cart* c;CartWallCache* cache;
    if(active_level!=2 && active_level!=7)return;
    // Reset/reversal maintain dir in {-1,+1}; avoid repeated signed shifts.
    c=carts;cache=cart_wall_cache;bit=1;remaining=active_level==7?1:3;
    do {
        if((cart_cache_valid&bit) && CartCachedStep(c,cache)){++c;++cache;bit<<=1;continue;}
        if(c->dir>0){step=32;nx=c->x+32;lead=(nx+752)>>8;}
        else{step=-32;nx=c->x-32;lead=nx>>8;}
        row=(c->y+288)>>8;
        if(!(cart_cache_valid&bit) || cache->lead!=lead || cache->row!=row || cache->dir!=c->dir){
            cache->lead=lead;cache->row=row;cache->dir=c->dir;
            cache->blocked=Wall(lead,row,c->dir>0?3:1) || !Wall(lead,row+1,0);
            cart_cache_valid|=bit;
        }
        if(cache->blocked){
            c->dir=-c->dir;c->delta=0;
        }else{c->delta=step;c->x=nx;}
        ++c;++cache;bit<<=1;
    }while(--remaining);
    if(cart_riding<3 && !climbing){player_x+=carts[cart_riding].delta;}
    if(climbing)cart_riding=255;
    c=carts;remaining=active_level==7?1:3;
    do {
        // Same open intervals, one unsigned range check per axis.
        if((u16)((c->x>>4)-camera_px+63)<335 &&
           (u16)((c->y>>4)-camera_py+31)<223){combat_dirty=1;break;}
        ++c;
    }while(--remaining);
}
// Cache refill for cart i (A): the lead/row/dir/blocked computation of CartStep's miss path, nothing else.
static void CartFill(u8 i) __sdcccall(1)
{
    Cart* c=carts+i;CartWallCache* cache=cart_wall_cache+i;u8 lead,row;
    if(c->dir>0)lead=((u16)(c->x+32)+752)>>8;else lead=(u16)(c->x-32)>>8;
    row=(c->y+288)>>8;
    cache->lead=lead;cache->row=row;cache->dir=c->dir;
    cache->blocked=Wall(lead,row,c->dir>0?3:1) || !Wall(lead,row+1,0);
    cart_cache_valid|=(u8)(1<<i);
}
extern u8 ca_w,ca_h,ca_y,ca_tx,ca_ty;
#ifndef KEEN_CORE15
// Whole-tick fast path in Z80 (no IX/IY). Pass 1 checks, without touching anything, that every cart's wall-cache entry
// is valid and hits (same lead column, row and direction); on any miss it falls back to CartStep (the original C
// body, which refills the cache). Pass 2 reverses blocked carts or advances them 32 units; then the riding player
// delta and the on-screen test (combat_dirty) of the original. Cart = {u16 x,y; i8 dir,delta} (6 bytes); cache entry
// = {lead,row,dir,blocked} (4 bytes) at world_collision+1152; loop state in ca_w (count) / ca_h (bit).
void CartStepFast(void) __naked
{
 __asm
 ld a,(_active_level)
 cp #7
 ld b,#1
 jr z,cs_n
 cp #2
 ret nz
 ld b,#3
cs_n:
 ld a,b
 ld (_ca_w),a
 ld a,#1
 ld (_ca_h),a
 ld hl,#_carts
 ld de,#(_world_collision+1152)
cs_p1:
 ld a,(_ca_h)
 ld b,a
 ld a,(_cart_cache_valid)
 and b
 jp z,cs_miss
 ld a,(hl)
 ld (_ca_tx),a
 inc hl
 ld c,(hl)
 inc hl
 ld a,(hl)
 ld (_ca_ty),a
 inc hl
 ld b,(hl)
 inc hl
 ld a,(hl)
 ld (_ca_y),a
 ld a,(_ca_ty)
 add a,#0x20
 ld a,b
 adc a,#1
 ld b,a
 ld a,(_ca_y)
 cp #1
 jr nz,cs_left
 ld a,(_ca_tx)
 add a,#0x10
 ld a,c
 adc a,#3
 jr cs_cmp
cs_left:
 ld a,(_ca_tx)
 sub a,#0x20
 ld a,c
 sbc a,#0
cs_cmp:
 ld c,a
 ld a,(de)
 cp c
 jp nz,cs_miss
 inc de
 ld a,(de)
 cp b
 jp nz,cs_miss
 inc de
 ld a,(_ca_y)
 ld b,a
 ld a,(de)
 cp b
 jp nz,cs_miss
 inc de
 inc de
 inc hl
 inc hl
 ld a,(_ca_h)
 add a,a
 ld (_ca_h),a
 ld a,(_ca_w)
 dec a
 ld (_ca_w),a
 jr nz,cs_p1
 ld a,(_active_level)
 cp #7
 ld a,#3
 jr nz,cs_c
 ld a,#1
cs_c:
 ld (_ca_w),a
 ld bc,#_carts
 ld de,#(_world_collision+1152)
cs_p2:
 ld l,c
 ld h,b
 inc hl
 inc hl
 inc hl
 inc hl
 inc de
 inc de
 inc de
 ld a,(de)
 inc de
 or a
 jr nz,cs_blk
 ld a,(hl)
 inc hl
 cp #1
 jr nz,cs_ml
 ld (hl),#32
 ld l,c
 ld h,b
 ld a,(hl)
 add a,#32
 ld (hl),a
 inc hl
 ld a,(hl)
 adc a,#0
 ld (hl),a
 jr cs_nx
cs_ml:
 ld (hl),#0xe0
 ld l,c
 ld h,b
 ld a,(hl)
 sub a,#32
 ld (hl),a
 inc hl
 ld a,(hl)
 sbc a,#0
 ld (hl),a
 jr cs_nx
cs_blk:
 ld a,(hl)
 neg
 ld (hl),a
 inc hl
 ld (hl),#0
cs_nx:
 ld hl,#6
 add hl,bc
 ld c,l
 ld b,h
 ld a,(_ca_w)
 dec a
 ld (_ca_w),a
 jr nz,cs_p2
 ld a,(_cart_riding)
 cp #3
 jr nc,cs_t1
 ld b,a
 ld a,(_climbing)
 or a
 jr nz,cs_t1
 ld a,b
 add a,a
 ld c,a
 add a,a
 add a,c
 ld c,a
 ld b,#0
 ld hl,#(_carts+5)
 add hl,bc
 ld a,(hl)
 ld e,a
 rlca
 sbc a,a
 ld d,a
 ld hl,(_player_x)
 add hl,de
 ld (_player_x),hl
cs_t1:
 ld a,(_climbing)
 or a
 jr z,cs_t2
 ld a,#255
 ld (_cart_riding),a
cs_t2:
 ld a,(_active_level)
 cp #7
 ld b,#3
 jr nz,cs_v0
 ld b,#1
cs_v0:
 ld hl,#_carts
cs_v1:
 push bc
 push hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 ld hl,(_camera_px)
 ex de,hl
 or a
 sbc hl,de
 ld de,#63
 add hl,de
 ld de,#335
 or a
 sbc hl,de
 jr nc,cs_vn
 pop hl
 push hl
 inc hl
 inc hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 ld hl,(_camera_py)
 ex de,hl
 or a
 sbc hl,de
 ld de,#31
 add hl,de
 ld de,#223
 or a
 sbc hl,de
 jr nc,cs_vn
 pop hl
 pop bc
 ld a,#1
 ld (_combat_dirty),a
 ret
cs_vn:
 pop hl
 ld bc,#6
 add hl,bc
 pop bc
 djnz cs_v1
 ret
cs_miss:
 ld a,(_ca_h)
 srl a
 call _CartFill
 jp _CartStepFast
 __endasm;
}
#else
void CartStepFast(void){CartStep();}   // standalone core payload has no Z80 fast path
#endif
#ifndef KEEN_CORE15
// Z80 version of the C CartContact below (all compares are 16-bit modular, as in the C). oldbottom in HL.
// Scratch: ca_x = oldbottom, ca_r = prior riding index, ca_wx = player_x+64, ca_wy = player_x+304.
extern u16 ca_x,ca_r,ca_wx,ca_wy;
void CartContact(u16 oldbottom) __naked
{
 oldbottom;
 __asm
 ld (_ca_x),hl
 ld a,(_cart_riding)
 ld (_ca_r),a
 ld a,#255
 ld (_cart_riding),a
 ld a,(_active_level)
 ld b,#3
 cp #2
 jr z,cc_ok
 cp #7
 ret nz
 ld b,#1
cc_ok:
 ld a,(_climbing)
 or a
 ret nz
 ld a,(_player_vy+1)
 bit 7,a
 ret nz
 ld hl,(_player_x)
 ld de,#64
 add hl,de
 ld (_ca_wx),hl
 ld de,#240
 add hl,de
 ld (_ca_wy),hl
 push ix
 ld ix,#_carts
 ld c,#0
cc_loop:
 ld l,(ix+0)
 ld h,(ix+1)
 ld de,(_ca_wy)
 or a
 sbc hl,de
 jr nc,cc_next
 ld l,(ix+0)
 ld h,(ix+1)
 ld de,#752
 add hl,de
 ex de,hl
 ld hl,(_ca_wx)
 or a
 sbc hl,de
 jr nc,cc_next
 ld l,(ix+2)
 ld h,(ix+3)
 ld de,#80
 add hl,de
 ex de,hl
 ld hl,#48
 add hl,de
 push de
 ex de,hl
 ld hl,(_ca_x)
 or a
 sbc hl,de
 jr c,cc_c1
 jr z,cc_c1
 ld a,(_ca_r)
 cp c
 jr z,cc_c1
 pop de
 jr cc_next
cc_c1:
 pop de
 ld hl,(_player_y)
 push de
 ld de,#496
 add hl,de
 pop de
 or a
 sbc hl,de
 jr c,cc_next
 ex de,hl
 ld de,#-480
 add hl,de
 ld (_player_y),hl
 ld hl,#0
 ld (_player_vy),hl
 ld a,#1
 ld (_grounded),a
 ld a,#25
 ld (_floor_type),a
 xor a
 ld (_jump_hold),a
 ld a,c
 ld (_cart_riding),a
 pop ix
 ret
cc_next:
 ld de,#6
 add ix,de
 inc c
 djnz cc_loop
 pop ix
 ret
 __endasm;
}
#else
void CartContact(u16 oldbottom)
{
    u8 i,prior=cart_riding;u16 top,left,right;Cart* c;
    cart_riding=255;
    if((active_level!=2 && active_level!=7) || climbing || player_vy<0)return;
    for(i=0;i<(active_level==7?1:3);++i){
        c=carts+i;top=c->y+80;left=player_x+64;right=player_x+304;
        if(right<=c->x || left>=c->x+752)continue;
        if((oldbottom<=top+48 || prior==i) && player_y+496>=top){
            player_y=top-480;player_vy=0;grounded=1;floor_type=25;jump_hold=0;cart_riding=i;return;
        }
    }
}
#endif
