// ROM segment 6 (ASCII8 bank 3 window, 0xA000): story texts and the code that builds them.
// Called from fixed code with the window switched: Seg_Set(SEG_TEXT); ...; Seg_Restore(old).
// Never returns pointers into this segment (callers copy into RAM buffers).
const unsigned char g_SegMagic6[2] = { 0x52, 0x36 }; // "R6"
#include "../game/texts.c"
