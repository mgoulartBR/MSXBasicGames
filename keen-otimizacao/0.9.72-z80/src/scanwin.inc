// Z80 window scan: indices of records whose u16 x,y (offsets 0,2) satisfy
// (u16)(x-ox)<w && (u16)(y-oy)<h, in ascending order. Args travel on the caller's stack.
// Record pointer lives in DE, remaining count/index in H/L; s1/s3 are filled by the routine.
typedef struct {const u8* items;u8* out;u16 ox,oy,w,h;u8 stride,n,count,s1,s3;} ScanArgs;
static u8 ScanWin(ScanArgs* args) __sdcccall(1) __naked
{
 args;
 __asm
  push iy
  push hl
  pop iy
  ld a,13 (iy)
  or a
  jr z,sw_done
  ld h,a
  ld l,#0
  ld a,12 (iy)
  dec a
  ld 15 (iy),a
  dec a
  dec a
  ld 16 (iy),a
  ld e,0 (iy)
  ld d,1 (iy)
sw_loop:
  ld a,(de)
  sub 4 (iy)
  ld c,a
  inc de
  ld a,(de)
  sbc a,5 (iy)
  ld b,a
  ld a,c
  sub 8 (iy)
  ld a,b
  sbc a,9 (iy)
  jr nc,sw_nx
  inc de
  ld a,(de)
  sub 6 (iy)
  ld c,a
  inc de
  ld a,(de)
  sbc a,7 (iy)
  ld b,a
  ld a,c
  sub 10 (iy)
  ld a,b
  sbc a,11 (iy)
  jr nc,sw_ny
  ld a,l
  push hl
  ld l,2 (iy)
  ld h,3 (iy)
  ld (hl),a
  inc hl
  ld 2 (iy),l
  ld 3 (iy),h
  pop hl
  inc 14 (iy)
sw_ny:
  ld a,e
  add a,16 (iy)
  jr sw_adv
sw_nx:
  ld a,e
  add a,15 (iy)
sw_adv:
  ld e,a
  jr nc,sw_nc
  inc d
sw_nc:
  inc l
  dec h
  jr nz,sw_loop
sw_done:
  ld a,14 (iy)
  pop iy
  ret
 __endasm;
}
