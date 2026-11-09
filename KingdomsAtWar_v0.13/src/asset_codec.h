/* Executed below A000h: bank 3 is overlaid atomically and restored
 * before returning to game code. BIOS page 0 is never unmapped. */
const u8* packed_src;
/* The 256 byte LZ window is only needed while loading; the sprite attribute buffer reuses the same RAM afterwards. */
u8 p1_shared[500];
#define packed_window p1_shared
u8 packed_write,packed_flags,packed_bits,packed_copy,packed_left;
static void asset_begin(const Asset* a) {
 asset_bank=a->bank;packed_src=(const u8*)a->ptr;packed_write=packed_bits=packed_left=0;
}
/* Z80 LZ decoder (same format and state as the C loop below): the load screen is dominated by this. */
u16 lz_dst;u8 lz_cnt;
static void lz_decode(void) __naked {
 __asm
 push ix
 ld ix,(_packed_src)
 ld de,(_lz_dst)
 ld a,(_packed_write)
 ld b,a
 ld a,(_packed_copy)
 ld c,a
lz_loop:
 ld a,(_lz_cnt)
 or a,a
 jp z,lz_done
 ld a,(_packed_left)
 or a,a
 jp z,lz_flag
 ld h,a
 ld a,(_lz_cnt)
 cp a,h
 jr c,lz_run_ok
 ld a,h
lz_run_ok:
 ld l,a
 ld a,(_lz_cnt)
 sub a,l
 ld (_lz_cnt),a
 ld a,(_packed_left)
 sub a,l
 ld (_packed_left),a
lz_run:
 push hl
 ld hl,#_p1_shared
 ld a,c
 add a,l
 ld l,a
 jr nc,lz_r1
 inc h
lz_r1:
 ld a,(hl)
 inc c
 push af
 ld hl,#_p1_shared
 ld a,b
 add a,l
 ld l,a
 jr nc,lz_r2
 inc h
lz_r2:
 pop af
 ld (hl),a
 inc b
 ld (de),a
 inc de
 pop hl
 dec l
 jr nz,lz_run
 jp lz_loop
lz_flag:
 ld a,(_packed_bits)
 or a,a
 jr nz,lz_have
 ld a,0(ix)
 inc ix
 ld (_packed_flags),a
 ld a,#8
 ld (_packed_bits),a
lz_have:
 ld a,(_packed_flags)
 bit 0,a
 jr z,lz_match
 ld a,0(ix)
 inc ix
 push af
 ld hl,#_p1_shared
 ld a,b
 add a,l
 ld l,a
 jr nc,lz_l1
 inc h
lz_l1:
 pop af
 ld (hl),a
 inc b
 ld (de),a
 inc de
 ld a,(_lz_cnt)
 dec a
 ld (_lz_cnt),a
 jr lz_shift
lz_match:
 ld a,b
 sub a,0(ix)
 dec a
 ld c,a
 inc ix
 ld a,0(ix)
 add a,#3
 ld (_packed_left),a
 inc ix
lz_shift:
 ld a,(_packed_flags)
 srl a
 ld (_packed_flags),a
 ld a,(_packed_bits)
 dec a
 ld (_packed_bits),a
 jp lz_loop
lz_done:
 ld (_packed_src),ix
 ld a,b
 ld (_packed_write),a
 ld a,c
 ld (_packed_copy),a
 pop ix
 ret
 __endasm;
}
void asset_read(u8* dst,u8 count) {
 poll_input();
 /* The decoder and raster hook both reside in fixed banks below A000h.
  * Keep raster interrupts alive while bank 3 holds read-only asset data. */
 __asm di __endasm;SET_BANK_SEGMENT(3,asset_bank);__asm ei __endasm;
 /* Consume an entire available back-reference run before reading flags.
  * Forward byte copies preserve overlap and the 256-byte ring wrap. */
 lz_dst=(u16)dst;lz_cnt=count;lz_decode();

 __asm di __endasm;SET_BANK_SEGMENT(3,3);__asm ei __endasm;poll_input();
}
