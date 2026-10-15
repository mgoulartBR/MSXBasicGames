// ROM segment 4 (ASCII8 bank 3 window, 0xA000): news items + all their strings.
// Everything referenced by g_News[] lives in this compilation unit, so pointers stay valid
// while segment 4 is mapped. Generated table: news_data.c (tools/gen_assets.py).
// Mapper self-test marker: must be the first object of the segment (address A000h)
const unsigned char g_SegMagic4[2] = { 0x52, 0x34 }; // "R4"
#include "news_data.c"
