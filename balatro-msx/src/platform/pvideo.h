// MSX2 Screen 5 video layer: palette, VRAM atlas, text, panels, mouse sprite.
#pragma once
#include "gtypes.h"

// palette indices (see tools/gen_assets.py ANCHORS)
enum { COL_FELT = 0, COL_INK, COL_CREAM, COL_SLATE, COL_RED, COL_BLUE, COL_GOLD, COL_ORANGE, COL_GREEN };
// text colours = font strips in VRAM
enum { TC_WHITE, TC_INK, TC_GOLD, TC_RED, TC_BLUE, TC_GREEN, TC_SLATE };

#define SCREEN_W 256
#define SCREEN_H 212
#define SEG_TEXT 20                 // mapper segment holding the text/data tables (default bank-3 mapping)

void Vid_Init(void);
void Vid_Sync(void);
u8   Vid_Hz(void);                  // 50 or 60                // wait for the next frame
void Vid_Fill(u8 x, u8 y, u8 w, u8 h, u8 col);
void Vid_Frame(u8 x, u8 y, u8 w, u8 h, u8 col);              // 1px outline
void Vid_Panel(u8 x, u8 y, u8 w, u8 h, u8 fill, u8 border);  // rounded panel
void Vid_Card(u8 cell, u8 x, u8 y);                           // 24x32 atlas cell
void Vid_PlayCard(u16 card, bool faceDown, u8 x, u8 y);       // a playing card with its enhancement / edition / seal marks
void Vid_EdStripe(u8 ed, u8 x, u8 y);                         // edition stripe on a card (jokers too)
void Vid_BlindIcon(u8 row, u8 x, u8 y);                       // 16x16
void Vid_TagIcon(u8 tag, u8 x, u8 y);                         // 16x16 (icons live below the visible screen, page 0 line 212+)
void Vid_Text(u8 x, u8 y, const char* s, u8 tc);
void Vid_TextN(u8 x, u8 y, const char* s, u8 n, u8 tc);       // at most n chars
u8   Vid_TextW(const char* s);
void Vid_TextC(u8 cx, u8 y, const char* s, u8 tc);            // centred on cx
void Vid_Num(u8 x, u8 y, i32 v, u8 tc);
void Vid_NumR(u8 xr, u8 y, i32 v, u8 tc);                     // right aligned at xr
u8   Vid_NumW(i32 v);
u8   Vid_Wrap(u8 x, u8 y, const char* s, u8 maxw, u8 tc, u8 maxLines);  // word wrap, returns lines used
u8   Vid_WrapDesc(u8 desc, u8 x, u8 y, u8 maxw, u8 tc, u8 maxLines);   // description #desc (segment 21), word wrapped
void Vid_Logo(u8 x, u8 y);
void Vid_Cursor(u8 x, u8 y, bool show);
void Vid_Clear(u8 col);
void Vid_Display(bool on);          // blank the screen while a whole screen is redrawn (also speeds the VDP up a little)
