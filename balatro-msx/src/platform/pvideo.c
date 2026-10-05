// MSX2 / V9938 Screen 5 (256x212, 16 colours). VRAM map:
//   page 0  y 0..211   display (drawn directly; panels are repainted per region)
//   page 1..3          atlas: playing cards, planets, jokers, tarots, blind icons, pre-coloured font strips
//   page 0  y 232..255 sprite tables (BIOS defaults)
#include "msxgl.h"
#include "pvideo.h"
#include "assets_gen.h"

extern const u8 g_FontW[95], g_FontX[95], g_FontL[95];
static const u8 s_palette[] = GFX_PALETTE_INIT;

// 8x8 -> 16x16 mouse arrow: fill sprite + 1px dilated outline sprite
static const u8 s_arrow[12] = { 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF, 0xF8, 0xD8, 0x8C, 0x0C };

static void build_cursor(void)
{
	u8 fill[32], out[32];
	u16 f[16], o[16];
	for (u8 r = 0; r < 16; r++) f[r] = 0, o[r] = 0;
	for (u8 r = 0; r < 12; r++) f[r + 1] = (u16)s_arrow[r] << 7;           // fill drawn at (1,1) of the 16x16 grid
	for (u8 r = 0; r < 16; r++)
	{
		u16 m = f[r] | (f[r] << 1) | (f[r] >> 1);                           // dilate horizontally
		if (r > 0) m |= f[r - 1] | (f[r - 1] << 1) | (f[r - 1] >> 1);
		if (r < 15) m |= f[r + 1] | (f[r + 1] << 1) | (f[r + 1] >> 1);
		o[r] = m;
	}
	for (u8 r = 0; r < 16; r++)
	{
		fill[r] = (u8)(f[r] >> 8);      fill[16 + r] = (u8)(f[r] & 0xFF);
		out[r]  = (u8)(o[r] >> 8);      out[16 + r]  = (u8)(o[r] & 0xFF);
	}
	VDP_LoadSpritePattern(fill, 0, 4);  // 16x16 sprite = 4 consecutive 8x8 patterns
	VDP_LoadSpritePattern(out, 4, 4);
}

void Vid_Sync(void) { Halt(); }

void Vid_Init(void)
{
	VDP_SetMode(VDP_MODE_SCREEN5);
	VDP_SetLineCount(VDP_LINE_212);
	VDP_EnableDisplay(FALSE);
	VDP_SetPalette(s_palette);
	VDP_SetColor(COL_FELT);
	VDP_SetPage(0);
	VDP_CommandHMMV(0, 0, 256, 256, 0);                    // clear page 0
	// atlas -> VRAM pages 1..3 (graphics live in mapper segments, visible through bank 3)
	u32 left = (u32)GFX_ATLAS_LINES * 128UL;
	u32 dst = (u32)GFX_ATLAS_Y0 * 128UL;
	for (u8 s = 0; s < GFX_ATLAS_SEGS; s++)
	{
		u16 n = left > 8192 ? 8192 : (u16)left;
		SET_BANK_SEGMENT(3, GFX_ATLAS_FIRST_SEG + s);
		VDP_WriteVRAM((const u8*)0xA000, (u16)(dst & 0xFFFF), (u8)(dst >> 16), n);
		dst += n; left -= n;
	}
	SET_BANK_SEGMENT(3, SEG_TEXT);
	VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);
	VDP_EnableSprite(TRUE);
	build_cursor();
	VDP_SetSpriteExUniColor(0, 0, 216, 0, COL_CREAM);      // y=216 hides it (and every lower-priority sprite)
	VDP_SetSpriteExUniColor(1, 0, 216, 4, COL_INK);
	VDP_EnableDisplay(TRUE);
}

void Vid_Clear(u8 col) { VDP_CommandHMMV(0, 0, 256, 212, col | (col << 4)); }

void Vid_Fill(u8 x, u8 y, u8 w, u8 h, u8 col)
{
	if (!w || !h) return;
	if (((x | w) & 1) == 0) VDP_CommandHMMV(x, y, w, h, col | (col << 4));
	else VDP_CommandLMMV(x, y, w, h, col, VDP_OP_IMP);
}

void Vid_Frame(u8 x, u8 y, u8 w, u8 h, u8 col)
{
	Vid_Fill(x, y, w, 1, col); Vid_Fill(x, y + h - 1, w, 1, col);
	Vid_Fill(x, y, 1, h, col); Vid_Fill(x + w - 1, y, 1, h, col);
}

void Vid_Panel(u8 x, u8 y, u8 w, u8 h, u8 fill, u8 border)
{
	Vid_Fill(x + 1, y, w - 2, h, border);        // rounded corners: border fill minus the corner pixels
	Vid_Fill(x, y + 1, w, h - 2, border);
	Vid_Fill(x + 1, y + 1, w - 2, h - 2, fill);
}

void Vid_Card(u8 cell, u8 x, u8 y)
{
	u16 sx = (u16)(cell % GFX_CELLS_PER_ROW) * GFX_CELL_W;
	u16 sy = GFX_ATLAS_Y0 + (u16)(cell / GFX_CELLS_PER_ROW) * GFX_CELL_H;
	VDP_CommandLMMM(sx, sy, x, y, GFX_CELL_W, GFX_CELL_H, VDP_OP_TIMP);
}

void Vid_BlindIcon(u8 row, u8 x, u8 y)
{
	VDP_CommandLMMM(240, GFX_ATLAS_Y0 + (u16)row * 16, x, y, 16, 16, VDP_OP_TIMP);
}

static void glyph(u8 x, u8 y, u8 ch, u8 tc)
{
	if (ch < 32 || ch > 126) ch = '?';
	ch -= 32;
	u8 w = g_FontW[ch];
	if (ch == 0) return;
	VDP_CommandLMMM(g_FontX[ch], FONT_Y0 + (u16)tc * FONT_STRIP_H + (u16)g_FontL[ch] * FONT_ROWS, x, y, w, FONT_ROWS, VDP_OP_TIMP);
}

u8 Vid_TextW(const char* s)
{
	u8 w = 0;
	for (; *s; s++) { u8 c = (u8)*s; w += g_FontW[(c < 32 || c > 126) ? 31 : c - 32]; }
	return w;
}

void Vid_TextN(u8 x, u8 y, const char* s, u8 n, u8 tc)
{
	for (; *s && n; s++, n--)
	{
		u8 c = (u8)*s;
		glyph(x, y, c, tc);
		x += g_FontW[(c < 32 || c > 126) ? 31 : c - 32];
	}
}
void Vid_Text(u8 x, u8 y, const char* s, u8 tc) { Vid_TextN(x, y, s, 255, tc); }
void Vid_TextC(u8 cx, u8 y, const char* s, u8 tc) { u8 w = Vid_TextW(s); Vid_Text(cx > w / 2 ? cx - w / 2 : 0, y, s, tc); }

static u8 itoa32(i32 v, char* buf)           // returns length, buf not terminated
{
	char tmp[11]; u8 n = 0, len = 0;
	bool neg = v < 0;
	u32 u = neg ? (u32)(-v) : (u32)v;
	do { tmp[n++] = (char)('0' + (u % 10)); u /= 10; } while (u);
	if (neg) buf[len++] = '-';
	while (n) buf[len++] = tmp[--n];
	buf[len] = 0;
	return len;
}
void Vid_Num(u8 x, u8 y, i32 v, u8 tc) { char b[13]; itoa32(v, b); Vid_Text(x, y, b, tc); }
u8 Vid_NumW(i32 v) { char b[13]; itoa32(v, b); return Vid_TextW(b); }
void Vid_NumR(u8 xr, u8 y, i32 v, u8 tc) { u8 w = Vid_NumW(v); Vid_Num(xr - w, y, v, tc); }

// word wrap helper: draws s inside maxw pixels, returns lines used (10px line height)
u8 Vid_Wrap(u8 x, u8 y, const char* s, u8 maxw, u8 tc, u8 maxLines)
{
	u8 line = 0, cx = 0;
	while (*s && line < maxLines)
	{
		const char* e = s; u8 w = 0;
		while (*e && *e != ' ') { w += g_FontW[((u8)*e < 32 || (u8)*e > 126) ? 31 : (u8)*e - 32]; e++; }
		if (cx + w > maxw && cx > 0) { line++; cx = 0; if (line >= maxLines) break; }
		Vid_TextN(x + cx, y + line * 10, s, (u8)(e - s), tc);
		cx += w;
		if (*e == ' ') { cx += g_FontW[0]; e++; }
		s = e;
	}
	return (u8)(line + 1);
}

void Vid_Logo(u8 x, u8 y)
{
	SET_BANK_SEGMENT(3, GFX_LOGO_SEG);
	VDP_CommandHMMC((const u8*)0xA000, x, y, GFX_LOGO_W, GFX_LOGO_H);
	SET_BANK_SEGMENT(3, SEG_TEXT);
}

void Vid_Cursor(u8 x, u8 y, bool show)
{
	if (!show) { VDP_SetSpritePosition(0, 0, 216); return; }
	VDP_SetSpritePosition(0, x - 1, y - 2);               // the hot spot is the arrow tip; +1 sprite Y offset
	VDP_SetSpritePosition(1, x - 1, y - 2);
}
