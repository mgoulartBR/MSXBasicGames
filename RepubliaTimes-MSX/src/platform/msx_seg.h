// ASCII8 ROM mapper helpers. Code and small tables stay in the fixed segments 0-2
// (0x4000-0x9FFF). Bank 3 (0xA000-0xBFFF) is a data window:
//   SEG_NEWS (4): news table + strings (g_News)
//   SEG_GFX  (5): converted bitmaps (g_Gfx_*)
#pragma once
#include "msxgl.h"

#define SEG_NEWS 4
#define SEG_GFX  5
//   SEG_TEXT (6): Text_Morning / Text_Night code + strings
#define SEG_TEXT 6
#define SEG_WINDOW_BANK 3

// Switch the data window; returns the previously mapped segment (for restoring)
inline u16 Seg_Set(u8 seg)
{
	u16 old = GET_BANK_SEGMENT(SEG_WINDOW_BANK);
	SET_BANK_SEGMENT(SEG_WINDOW_BANK, seg);
	return old;
}
inline void Seg_Restore(u16 old) { SET_BANK_SEGMENT(SEG_WINDOW_BANK, old); }
