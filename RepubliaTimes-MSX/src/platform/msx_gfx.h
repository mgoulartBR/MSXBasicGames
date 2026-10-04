// Platform layer: Screen 2 tile canvas, proportional text, colour helpers.
// Everything the game draws goes through here (no VDP access elsewhere).
#pragma once
#include "msxgl.h"

#define SCR_COLS 32
#define SCR_ROWS 24

// Colours (colour-table byte = ink<<4 | paper)
#define CT_INK       0x1F  // black on white
#define CT_INV       0xF1  // white on black
#define CT_RED       0x6F  // dark red on white
#define CT_RED_INV   0xF6
#define CT_GRAY      0x1E  // black on gray
#define CT_OK        0x13  // black on light green
#define CT_BAD       0x19  // black on light red

// A canvas is a RAM copy of a rectangle of tiles, in VRAM pattern layout:
// buf[(trow * w + tcol) * 8 + y]
typedef struct
{
	u8* buf;
	u8 w; // tiles
	u8 h; // tile rows
} Canvas;

// A wrapped line of text
typedef struct
{
	u16 start;
	u8 len;
} TextLine;

void Gfx_Init(void);
void Gfx_Clear(u8 color);
void Gfx_Display(bool on);

void Gfx_FillPattern(u8 col, u8 row, u8 w, u8 h, u8 even, u8 odd);
void Gfx_FillColor(u8 col, u8 row, u8 w, u8 h, u8 color);
void Gfx_Blit(u8 col, u8 row, const u8* tiles, u8 w, u8 h);
void Gfx_BlitCanvas(u8 col, u8 row, const Canvas* cv);

void Cv_Clear(Canvas* cv, u8 v);
void Cv_Copy(Canvas* cv, const u8* tiles);
void Cv_Pixel(Canvas* cv, u8 x, u8 y);
void Cv_Line(Canvas* cv, i8 x0, i8 y0, i8 x1, i8 y1);
void Cv_FillRect(Canvas* cv, u8 x, u8 y, u8 w, u8 h);
void Cv_HLine(Canvas* cv, u8 x, u8 y, u8 w, u8 dotted);
void Cv_VLine(Canvas* cv, u8 x, u8 y, u8 h, u8 dotted);
void Cv_Invert(Canvas* cv);
u8 Cv_Text(Canvas* cv, u8 x, u8 trow, const char* s, u8 len); // returns pixel width drawn
u8 Cv_TextKnock(Canvas* cv, u8 x, u8 trow, const char* s, u8 len);

u8 Text_Len(const char* s);
u8 Text_Width(const char* s, u8 len);
u8 Text_Wrap(const char* s, u8 maxw, TextLine* out, u8 maxLines);

// Draw 'len' chars as one text line directly on screen (tile aligned), then colour it
void Gfx_TextLine(u8 col, u8 row, u8 wtiles, u8 xpix, const char* s, u8 len, u8 color);
void Gfx_Text(u8 col, u8 row, u8 wtiles, const char* s, u8 color);
void Gfx_TextRight(u8 col, u8 row, u8 wtiles, const char* s, u8 color);
// Centered variant
void Gfx_TextCenter(u8 col, u8 row, u8 wtiles, const char* s, u8 color);

// Mouse pointer (2 hardware sprites: white hand + black outline). Hotspot = finger tip.
void Pointer_Update(bool show, u8 x, u8 y);

// Shared scratch strip (one tile row, full screen width)
extern Canvas g_Strip;
