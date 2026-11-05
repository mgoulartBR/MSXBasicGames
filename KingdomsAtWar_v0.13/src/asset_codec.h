/* Executed below A000h: bank 3 is overlaid atomically and restored
 * before returning to game code. BIOS page 0 is never unmapped. */
const u8* packed_src;
u8 packed_window[256],packed_write,packed_flags,packed_bits,packed_copy,packed_left;
static void asset_begin(const Asset* a) {
 asset_bank=a->bank;packed_src=(const u8*)a->ptr;packed_write=packed_bits=packed_left=0;
}
void asset_read(u8* dst,u8 count) {
 poll_input();
 /* The decoder and raster hook both reside in fixed banks below A000h.
  * Keep raster interrupts alive while bank 3 holds read-only asset data. */
 __asm di __endasm;SET_BANK_SEGMENT(3,asset_bank);__asm ei __endasm;
 /* Consume an entire available back-reference run before reading flags.
  * Forward byte copies preserve overlap and the 256-byte ring wrap. */
 while(count) {
  if(packed_left) {
   u8 run=packed_left<count?packed_left:count;
   count-=run;packed_left-=run;
   do {
    u8 value=packed_window[packed_copy++];
    packed_window[packed_write++]=value;*dst++=value;
   }while(--run);
  } else {
   if(!packed_bits) {packed_flags=*packed_src++;packed_bits=8;}
   if(packed_flags&1) {
    u8 value=*packed_src++;
    packed_window[packed_write++]=value;*dst++=value;--count;
   } else {packed_copy=packed_write-*packed_src++-1;packed_left=(*packed_src++)+3;}
   packed_flags>>=1;--packed_bits;
  }
 }

 __asm di __endasm;SET_BANK_SEGMENT(3,3);__asm ei __endasm;poll_input();
}
#ifndef KAW_V9990
/* v0.13: native poses are stored raw. A cache miss is now one block copy per
 * 16x16 piece instead of an LZ decode in C (~360 cycles/byte). */
void raw_read(u8* dst,u8 count) {
 poll_input();
 __asm di __endasm;SET_BANK_SEGMENT(3,asset_bank);__asm ei __endasm;
 __builtin_memcpy(dst,packed_src,count);packed_src+=count;
 __asm di __endasm;SET_BANK_SEGMENT(3,3);__asm ei __endasm;poll_input();
}
#endif
