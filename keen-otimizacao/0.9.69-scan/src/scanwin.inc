// Z80 window scan: indices of records whose u16 x,y (offsets 0,2) satisfy
// (u16)(x-ox)<w && (u16)(y-oy)<h, in ascending order. Args travel on the caller's stack.
typedef struct {const u8* items;u8* out;u16 ox,oy,w,h;u8 stride,n,count;} ScanArgs;
static u8 ScanWin(ScanArgs* args) __sdcccall(1) __naked
{
 args;
 __asm
  push ix
  push iy
  push hl
  pop iy
  ld a,13 (iy)
  or a
  jr z,sw_done
  ld l,0 (iy)
  ld h,1 (iy)
  push hl
  pop ix
  ld b,a
  ld c,#0
sw_loop:
  ld l,0 (ix)
  ld h,1 (ix)
  ld e,4 (iy)
  ld d,5 (iy)
  or a
  sbc hl,de
  ld e,8 (iy)
  ld d,9 (iy)
  or a
  sbc hl,de
  jr nc,sw_next
  ld l,2 (ix)
  ld h,3 (ix)
  ld e,6 (iy)
  ld d,7 (iy)
  or a
  sbc hl,de
  ld e,10 (iy)
  ld d,11 (iy)
  or a
  sbc hl,de
  jr nc,sw_next
  ld l,2 (iy)
  ld h,3 (iy)
  ld (hl),c
  inc hl
  ld 2 (iy),l
  ld 3 (iy),h
  inc 14 (iy)
sw_next:
  inc c
  ld e,12 (iy)
  ld d,#0
  add ix,de
  djnz sw_loop
sw_done:
  ld a,14 (iy)
  pop iy
  pop ix
  ret
 __endasm;
}
