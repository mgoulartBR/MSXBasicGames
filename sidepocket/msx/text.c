#include "text.h"
#include "font/font_mgl_sample6.h"

#define MAX_CHARS 40

static u8 s_Row[MAX_CHARS * 6];
static u8 s_Pair[4];

void Vram_Write(const u8* src, u32 dest, u16 count)
{
	VDP_CommandWait();
	DisableInterrupt();
	VDP_WriteVRAM(src, (u16)dest, (u8)(dest >> 16), count);
	EnableInterrupt();
}

static void Pairs(u8 fg, u8 bg)
{
	// two pixels -> one byte; index = the two font bits (left pixel = high bit)
	s_Pair[0] = (bg << 4) | bg;
	s_Pair[1] = (bg << 4) | fg;
	s_Pair[2] = (fg << 4) | bg;
	s_Pair[3] = (fg << 4) | fg;
}

u8 Text_Width(const c8* str)
{
	u8 n = 0;
	while (str[n] && n < MAX_CHARS) n++;
	return n * 6;
}

void Text_Draw(u8 x, u8 y, const c8* str, u8 fg, u8 bg)
{
	const c8* s;
	u8* o;
	u8 n = Text_Width(str) / 6, r, b;

	Pairs(fg, bg);
	for (r = 0; r < 8; r++)
	{
		o = s_Row;
		for (s = str; s < str + n; s++)
		{
			b = g_Font_MGL_Sample6[4 + ((u16)(u8)*s << 3) + r];
			*o++ = s_Pair[(b >> 6) & 3];
			*o++ = s_Pair[(b >> 4) & 3];
			*o++ = s_Pair[(b >> 2) & 3];
		}
		Vram_Write(s_Row, (u32)(y + r) * 128 + (x >> 1), (u16)n * 3);
	}
}

void Text_DrawBig(u8 x, u8 y, const c8* str, u8 fg, u8 bg)
{
	const c8* s;
	u8* o;
	u8 n = Text_Width(str) / 6, r, b, k, p;
	if (n > MAX_CHARS) n = MAX_CHARS;

	for (r = 0; r < 16; r++)
	{
		o = s_Row;
		for (s = str; s < str + n; s++)
		{
			b = g_Font_MGL_Sample6[4 + ((u16)(u8)*s << 3) + (r >> 1)];
			for (k = 0; k < 6; k++)                     // each font pixel becomes 2 pixels = 1 byte
			{
				p = (b & (0x80 >> k)) ? fg : bg;
				*o++ = (p << 4) | p;
			}
		}
		Vram_Write(s_Row, (u32)(y + r) * 128 + (x >> 1), (u16)n * 6);
	}
}

void Text_DrawNum(u8 x, u8 y, u16 value, u8 width, u8 fg, u8 bg)
{
	c8 buf[7];
	u8 i = 6;
	buf[6] = 0;
	do { buf[--i] = '0' + (value % 10); value /= 10; } while (value && i);
	while (6 - i < width && i) buf[--i] = ' ';
	Text_Draw(x, y, buf + i, fg, bg);
}
