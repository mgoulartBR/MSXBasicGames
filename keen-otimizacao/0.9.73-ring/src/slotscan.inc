// Z80: indices of slots the first ActorsPrepare pass must visit: anything visible now (the HUD
// slot always is) or visible on either page. First array pointer is pre-biased to the visible
// byte (+5); db/dc are the byte distances to the other two arrays.
typedef struct {const u8* a;u16 db,dc;u8* out;u8 n,count,n0;} SlotArgs;
static u8 SlotScan(SlotArgs* args) __sdcccall(1) __naked
{
 args;
 __asm
  push iy
  push hl
  pop iy
  ld l,0 (iy)
  ld h,1 (iy)
  ld c,2 (iy)
  ld b,3 (iy)
  ld e,4 (iy)
  ld d,5 (iy)
ss_loop:
  ld a,(hl)
  or a
  jr nz,ss_take
  push hl
  add hl,bc
  ld a,(hl)
  pop hl
  push hl
  add hl,de
  or (hl)
  pop hl
  and #1
  jr z,ss_next
ss_take:
  ld a,10 (iy)
  sub 8 (iy)
  push hl
  ld l,6 (iy)
  ld h,7 (iy)
  ld (hl),a
  inc hl
  ld 6 (iy),l
  ld 7 (iy),h
  pop hl
  inc 9 (iy)
ss_next:
  ld a,l
  add a,#6
  ld l,a
  jr nc,ss_nc
  inc h
ss_nc:
  dec 8 (iy)
  jr nz,ss_loop
  ld a,9 (iy)
  pop iy
  ret
 __endasm;
}
