// ROM segment 4 (ASCII8 bank 3 window, 0xA000): news items + all their strings.
// Everything referenced by g_News[] lives in this compilation unit, so pointers stay valid
// while segment 4 is mapped. Generated table: news_data.c (tools/gen_assets.py).
#include "news_data.c"
