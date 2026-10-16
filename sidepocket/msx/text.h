// Minimal text output in Screen 5 using the MSXGL 6x8 bitmap font (replaces the 8 KB Print module).
#pragma once
#include "msxgl.h"

// VRAM block write.  A running VDP command and CPU writes to VRAM collide (bytes get lost), so wait for the
// command engine first; interrupts are off during the write because the BIOS handler touches the VDP.
void Vram_Write(const u8* src, u32 dest, u16 count);

// x must be even (glyphs are 6 px wide = 3 bytes); opaque: unset pixels use bg.  Pages 0 and 1 only.
void Text_Draw(u8 x, u8 y, const c8* str, u8 fg, u8 bg);
void Text_DrawBig(u8 x, u8 y, const c8* str, u8 fg, u8 bg);     // 2x size (12x16 glyphs)
void Text_DrawNum(u8 x, u8 y, u16 value, u8 width, u8 fg, u8 bg);  // right aligned, space padded to `width`
u8   Text_Width(const c8* str);                                // in pixels (6 per character)
