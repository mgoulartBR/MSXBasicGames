// Platform layer: Screen 2 tile canvas, proportional text, colour helpers.
#include "msx_gfx.h"
#include "../game_data.h"

#define PT_BASE 0x0000
#define NT_BASE 0x1800
#define CT_BASE 0x2000

#define TILE_OFS(col, row) ((u16)((row) >> 3) * 0x800 + (u16)((row) & 7) * 0x100 + (u16)(col) * 8)

static u8 s_StripBuf[SCR_COLS * 8];
Canvas g_Strip = { s_StripBuf, SCR_COLS, 1 };

void Gfx_Init(void)
{
	VDP_SetMode(VDP_MODE_GRAPHIC2);
	VDP_EnableVBlank(TRUE);
	VDP_EnableDisplay(FALSE);
	VDP_SetColor(COLOR_BLACK);
	VDP_ClearVRAM();
	// Identity name table: every screen cell owns a unique tile (bitmap-like mode)
	for (u16 i = 0; i < 256; ++i)
		s_StripBuf[i] = (u8)i;
	VDP_WriteVRAM_16K(s_StripBuf, NT_BASE, 256);
	VDP_WriteVRAM_16K(s_StripBuf, NT_BASE + 256, 256);
	VDP_WriteVRAM_16K(s_StripBuf, NT_BASE + 512, 256);
	VDP_FillVRAM_16K(0xD0, 0x1B00, 1); // sprite list terminator: no sprites
	VDP_EnableDisplay(TRUE);
}

void Gfx_Display(bool on)
{
	VDP_EnableDisplay(on);
}

void Gfx_Clear(u8 color)
{
	VDP_FillVRAM_16K(0x00, PT_BASE, 0x1800);
	VDP_FillVRAM_16K(color, CT_BASE, 0x1800);
}

void Gfx_FillPattern(u8 col, u8 row, u8 w, u8 h, u8 even, u8 odd)
{
	u8 y;
	if (even == odd)
	{
		for (y = 0; y < h; ++y)
			VDP_FillVRAM_16K(even, PT_BASE + TILE_OFS(col, row + y), (u16)w * 8);
		return;
	}
	for (y = 0; y < h; ++y)
	{
		for (u8 x = 0; x < w; ++x)
		{
			u8 r;
			for (r = 0; r < 8; ++r)
				s_StripBuf[r] = (r & 1) ? odd : even;
			VDP_WriteVRAM_16K(s_StripBuf, PT_BASE + TILE_OFS(col + x, row + y), 8);
		}
	}
}

void Gfx_FillColor(u8 col, u8 row, u8 w, u8 h, u8 color)
{
	for (u8 y = 0; y < h; ++y)
		VDP_FillVRAM_16K(color, CT_BASE + TILE_OFS(col, row + y), (u16)w * 8);
}

void Gfx_Blit(u8 col, u8 row, const u8* tiles, u8 w, u8 h)
{
	u16 n = (u16)w * 8;
	for (u8 y = 0; y < h; ++y)
	{
		VDP_WriteVRAM_16K(tiles, PT_BASE + TILE_OFS(col, row + y), n);
		tiles += n;
	}
}

void Gfx_BlitCanvas(u8 col, u8 row, const Canvas* cv)
{
	Gfx_Blit(col, row, cv->buf, cv->w, cv->h);
}

//-----------------------------------------------------------------------------
// Canvas primitives

static u16 cv_size(const Canvas* cv) { return (u16)cv->w * cv->h * 8; }

void Cv_Clear(Canvas* cv, u8 v)
{
	Mem_Set(v, cv->buf, cv_size(cv));
}

void Cv_Copy(Canvas* cv, const u8* tiles)
{
	Mem_Copy(tiles, cv->buf, cv_size(cv));
}

static u8* cv_byte(Canvas* cv, u8 x, u8 y)
{
	return cv->buf + ((u16)(y >> 3) * cv->w + (x >> 3)) * 8 + (y & 7);
}

static const u8 s_Mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

void Cv_Pixel(Canvas* cv, u8 x, u8 y)
{
	if (x >= ((u16)cv->w << 3) || y >= ((u16)cv->h << 3))
		return;
	*cv_byte(cv, x, y) |= s_Mask[x & 7];
}

void Cv_Line(Canvas* cv, i8 x0, i8 y0, i8 x1, i8 y1)
{
	i8 dx = x1 > x0 ? x1 - x0 : x0 - x1;
	i8 dy = y1 > y0 ? y1 - y0 : y0 - y1;
	i8 sx = x0 < x1 ? 1 : -1;
	i8 sy = y0 < y1 ? 1 : -1;
	i16 err = dx - dy;
	for (;;)
	{
		if (x0 >= 0 && y0 >= 0)
			Cv_Pixel(cv, (u8)x0, (u8)y0);
		if (x0 == x1 && y0 == y1)
			break;
		i16 e2 = err * 2;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}
}

void Cv_HLine(Canvas* cv, u8 x, u8 y, u8 w, u8 dotted)
{
	for (u8 i = 0; i < w; ++i)
		if (!dotted || !(i & 1))
			Cv_Pixel(cv, x + i, y);
}

void Cv_VLine(Canvas* cv, u8 x, u8 y, u8 h, u8 dotted)
{
	for (u8 i = 0; i < h; ++i)
		if (!dotted || !(i & 1))
			Cv_Pixel(cv, x, y + i);
}

void Cv_FillRect(Canvas* cv, u8 x, u8 y, u8 w, u8 h)
{
	u8 x1 = x + w; // exclusive
	for (u8 yy = y; yy < y + h; ++yy)
	{
		u8 xx = x;
		while (xx < x1)
		{
			u8 bit = xx & 7;
			u8 span = 8 - bit;
			if (span > x1 - xx)
				span = x1 - xx;
			u8 mask = (u8)(0xFF >> bit);
			if (span < 8 - bit)
				mask &= (u8)(0xFF << (8 - bit - span));
			*cv_byte(cv, xx, yy) |= mask;
			xx += span;
		}
	}
}

void Cv_Invert(Canvas* cv)
{
	u16 n = cv_size(cv);
	u8* p = cv->buf;
	while (n--)
	{
		*p = ~*p;
		++p;
	}
}

//-----------------------------------------------------------------------------
// Text

static const u8* glyph(char c)
{
	u8 u = (u8)c;
	if (u < 32 || u > 126)
		u = '?';
	return g_Font_Silk + (u16)(u - 32) * 9;
}

u8 Text_Width(const char* s, u8 len)
{
	u8 w = 0;
	while (len--)
		w += glyph(*s++)[0];
	return w;
}

// --- Glyph blitter (assembly hotspot, see docs/PORTING.md "Profiling") ---------------------
// Profiling (openMSX PC sampling) showed the C version spent ~9000 cycles per character on the
// variable shifts. This routine ORs (or clears) one 8-row glyph shifted right by 'off' bits into
// two horizontally adjacent tiles of a tile-major canvas (left tile bytes dst[0..7], right tile
// bytes dst[8..15]).
//   gl_src : glyph rows (8 bytes, left aligned)        gl_dst : left tile, row 0
//   gl_off : 0..7                                      gl_mode: bit0 = clear instead of OR,
//                                                              bit1 = do not touch the right tile
u8 const* gl_src;
u8* gl_dst;
u8 gl_off;
u8 gl_mode;

static void glyph_blit(void)
{
	// No alternate registers and no stack tricks: safe against the VBlank hook (ISR) at any point
	__asm
		push	ix
		push	iy
		ld	iy, (_gl_src)
		ld	ix, (_gl_dst)
		ld	a, (_gl_off)
		ld	c, a
		ld	a, (_gl_mode)
		ld	l, a			; l = mode flags
		ld	b, #8			; 8 glyph rows
	1$:
		ld	a, 0(iy)
		inc	iy
		ld	e, #0			; e = bits shifted out to the right tile
		ld	d, c
		inc	d
		dec	d
		jr	z, 3$
	2$:
		srl	a
		rr	e
		dec	d
		jr	nz, 2$
	3$:
		bit	0, l
		jr	nz, 5$
		or	0(ix)			; OR mode: left tile
		ld	0(ix), a
		jr	6$
	5$:
		cpl				; clear mode: left tile
		and	0(ix)
		ld	0(ix), a
		ld	a, e
		cpl
		ld	e, a
	6$:
		bit	1, l			; right tile skipped when mode bit1 set
		jr	nz, 8$
		ld	a, e
		bit	0, l
		jr	nz, 7$
		or	8(ix)
		ld	8(ix), a
		jr	8$
	7$:
		and	8(ix)
		ld	8(ix), a
	8$:
		inc	ix
		djnz	1$
		pop	iy
		pop	ix
	__endasm;
}

static u8 cv_text(Canvas* cv, u8 x, u8 trow, const char* s, u8 len, u8 knock)
{
	u8 x0 = x;
	u16 maxw = (u16)cv->w << 3;
	while (len--)
	{
		const u8* g = glyph(*s++);
		u8 tx = x >> 3;
		if (x >= maxw)
			break;
		gl_src = g + 1;
		gl_dst = cv->buf + ((u16)trow * cv->w + tx) * 8;
		gl_off = x & 7;
		gl_mode = knock | ((gl_off == 0 || (u8)(tx + 1) >= cv->w) ? 2 : 0);
		glyph_blit();
		x += g[0];
	}
	return x - x0;
}

u8 Cv_Text(Canvas* cv, u8 x, u8 trow, const char* s, u8 len)
{
	return cv_text(cv, x, trow, s, len, 0);
}

// Same as Cv_Text but clears the glyph pixels (white text on a filled area)
u8 Cv_TextKnock(Canvas* cv, u8 x, u8 trow, const char* s, u8 len)
{
	return cv_text(cv, x, trow, s, len, 1);
}

u8 Text_Wrap(const char* s, u8 maxw, TextLine* out, u8 maxLines)
{
	u8 n = 0;
	u16 i = 0;
	u16 start = 0;
	u16 lastSpace = 0xFFFF;
	u8 w = 0;
	for (;;)
	{
		char c = s[i];
		if (c == 0 || c == '\n')
		{
			if (n < maxLines)
			{
				out[n].start = start;
				out[n].len = (u8)(i - start);
				++n;
			}
			if (c == 0)
				return n;
			++i;
			start = i;
			lastSpace = 0xFFFF;
			w = 0;
			continue;
		}
		u8 cw = glyph(c)[0];
		if (w + cw > maxw && i > start)
		{
			if (lastSpace != 0xFFFF && lastSpace >= start)
			{
				// soft break at the last space; the remainder moves to the next line
				if (n < maxLines)
				{
					out[n].start = start;
					out[n].len = (u8)(lastSpace - start);
					++n;
				}
				start = lastSpace + 1;
				w = Text_Width(s + start, (u8)(i - start));
			}
			else
			{
				// hard break inside a long word
				if (n < maxLines)
				{
					out[n].start = start;
					out[n].len = (u8)(i - start);
					++n;
				}
				start = i;
				w = 0;
				if (c == ' ')
				{
					start = ++i;
					lastSpace = 0xFFFF;
					continue;
				}
			}
			lastSpace = 0xFFFF;
		}
		if (c == ' ')
			lastSpace = i;
		w += cw;
		++i;
	}
}

void Gfx_TextLine(u8 col, u8 row, u8 wtiles, u8 xpix, const char* s, u8 len, u8 color)
{
	g_Strip.w = wtiles;
	Cv_Clear(&g_Strip, 0);
	Cv_Text(&g_Strip, xpix, 0, s, len);
	Gfx_BlitCanvas(col, row, &g_Strip);
	Gfx_FillColor(col, row, wtiles, 1, color);
}

void Gfx_TextCenter(u8 col, u8 row, u8 wtiles, const char* s, u8 color)
{
	u8 len = 0;
	while (s[len])
		++len;
	u8 tw = Text_Width(s, len);
	u16 px = (u16)wtiles << 3;
	Gfx_TextLine(col, row, wtiles, tw < px ? (px - tw) >> 1 : 0, s, len, color);
}

u8 Text_Len(const char* s)
{
	u8 n = 0;
	while (s[n])
		++n;
	return n;
}

void Gfx_Text(u8 col, u8 row, u8 wtiles, const char* s, u8 color)
{
	Gfx_TextLine(col, row, wtiles, 1, s, Text_Len(s), color);
}

void Gfx_TextRight(u8 col, u8 row, u8 wtiles, const char* s, u8 color)
{
	u8 len = Text_Len(s);
	u8 tw = Text_Width(s, len);
	u16 px = (u16)wtiles << 3;
	Gfx_TextLine(col, row, wtiles, tw + 1 < px ? px - tw - 1 : 0, s, len, color);
}
