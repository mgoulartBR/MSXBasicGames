#include "gfx.h"
#include "gen/blob.h"
#include "blob_io.h"
#include "text.h"

#define STRIP_Y      (PAGE1 + 216)      // transparent 12x(8x8) ball cells, page 1 (for LMMM)
#define SOLID_EVEN_Y (PAGE1 + 224)      // same cells with the cloth colour in the corners (HMMM, even x)
#define SOLID_ODD_Y  (PAGE1 + 232)      // 10-wide cells [cloth|ball|cloth] (HMMM, odd x)
#define CELL_DOT     10
#define AIM_DOTS     6

static u8 q_n;
static u16 g_Drawn;                 // bit n = ball n is currently drawn at (g_BX[n], g_BY[n])
static u8  g_DotN;

void Gfx_Init(void)
{
	q_n = 0; g_Drawn = 0; g_DotN = 0;            // RAM is not cleared by the mapper-ROM startup code
	VDP_SetMode(VDP_MODE_SCREEN5);
	VDP_SetLineCount(VDP_LINE_212);
	VDP_EnableDisplay(FALSE);
	VDP_EnableSprite(FALSE);                     // no hardware sprites: VDP commands run faster
	VDP_SetColor(0);
}

void Gfx_Show(bool on) { VDP_EnableDisplay(on); }
void Gfx_Wait(void)    { VDP_CommandWait(); }

void Gfx_Copy(u16 sx, u16 sy, u16 dx, u16 dy, u16 w, u16 h)
{
	VDP_CommandWait();
	VDP_CommandHMMM(sx, sy, dx, dy, w, h);
}

void Gfx_Fill(u16 x, u16 y, u16 w, u16 h, u8 col)
{
	VDP_CommandWait();
	VDP_CommandLMMV(x, y, w, h, col, VDP_OP_IMP);
}

void Gfx_Outline(u16 x, u16 y, u16 w, u16 h, u8 col)
{
	Gfx_Fill(x, y, w, 1, col);
	Gfx_Fill(x, y + h - 1, w, 1, col);
	Gfx_Fill(x, y, 1, h, col);
	Gfx_Fill(x + w - 1, y, 1, h, col);
}

//-----------------------------------------------------------------------------
// Assets (from the ROM blob)
static void LoadStrip(void)
{
	u8 y, k, len;
	static const u8 lens[3] = { 48, 48, 60 };
	static const u16 ys[3] = { STRIP_Y, SOLID_EVEN_Y, SOLID_ODD_Y };
	static u8 buf[60];
	Blob_Open(BLOB_BALLS_SEG, BLOB_BALLS_OFF);
	for (k = 0; k < 3; k++)
		for (y = 0; y < 8; y++)
		{
			for (len = 0; len < lens[k]; len++) buf[len] = Blob_Byte();
			Vram_Write(buf, 0x8000UL + (u32)(ys[k] - PAGE1 + y) * 128, lens[k]);
		}
}

//-----------------------------------------------------------------------------
// Drawing. Balls are redrawn only when they (or a neighbour they touch) changed.
static u8  g_BX[SP_NUM_BALLS], g_BY[SP_NUM_BALLS];
static u8  g_DotX[AIM_DOTS], g_DotY[AIM_DOTS];

static void DoCell(u8 cell, u8 x, u8 y)
{
	VDP_CommandWait();
	VDP_CommandLMMM((u16)cell * 8, STRIP_Y, x, y, 8, 8, VDP_OP_TIMP);
}

// A cell over perfectly flat cloth can be copied with the (much faster) byte-wise HMMM, since
// its transparent pixels are pre-filled with the cloth colour.  LMMM+TIMP costs 3-6 ms per ball.
static void DoSolid(u8 cell, u8 x, u8 y)
{
	VDP_CommandWait();
	if (x & 1) VDP_CommandHMMM((u16)cell * 10, SOLID_ODD_Y,  x - 1, y, 10, 8);
	else       VDP_CommandHMMM((u16)cell * 8,  SOLID_EVEN_Y, x,     y, 8,  8);
}

static bool Flat(u8 x, u8 y)
{
	return x > FLAT_X0 && x <= FLAT_X1 - 9 && y >= TBL_Y + FLAT_Y0 && y <= TBL_Y + FLAT_Y1 - 7;
}

static void DoErase(u8 x, u8 y)
{
	// byte-aligned fast copy from the clean page; 10 wide covers an odd x
	x &= 0xFE;
	VDP_CommandWait();
	VDP_CommandHMMM(x, PAGE1 + y, x, y, 10, 8);
}

//-----------------------------------------------------------------------------
// Command queue.  The game logic only *plans* what to draw; Flush() runs the VDP commands back to back,
// right after the vertical-blank interrupt, so the beam is still above the table while balls are erased and
// redrawn (an erase followed by milliseconds of CPU work before the redraw shows up as blinking).
enum { OP_ERASE, OP_SOLID, OP_CELL };
#define Q_MAX 80
static u8 q_op[Q_MAX], q_cell[Q_MAX], q_x[Q_MAX], q_y[Q_MAX];

static void Push(u8 op, u8 cell, u8 x, u8 y)
{
	if (q_n >= Q_MAX) return;
	q_op[q_n] = op; q_cell[q_n] = cell; q_x[q_n] = x; q_y[q_n] = y;
	q_n++;
}
static void EraseSpot(u8 x, u8 y)          { Push(OP_ERASE, 0, x, y); }
static void DrawSolid(u8 c, u8 x, u8 y)    { Push(OP_SOLID, c, x, y); }
static void DrawCell(u8 c, u8 x, u8 y)     { Push(OP_CELL, c, x, y); }

void Gfx_Flush(void)
{
	u8 i;
	for (i = 0; i < q_n; i++)
	{
		if (q_op[i] == OP_ERASE)      DoErase(q_x[i], q_y[i]);
		else if (q_op[i] == OP_SOLID) DoSolid(q_cell[i], q_x[i], q_y[i]);
		else                          DoCell(q_cell[i], q_x[i], q_y[i]);
	}
	q_n = 0;
}

void Gfx_LoadGame(void)
{
	q_n = 0; g_Drawn = 0; g_DotN = 0;            // RAM is not cleared at startup: initialise what we rely on
	Blob_SetPalette(BLOB_PALETTE_SEG, BLOB_PALETTE_OFF);
	Blob_LoadImage(BLOB_TABLE_SEG, BLOB_TABLE_OFF, 0, 1);     // page 0 (visible) and page 1 (clean copy)
	LoadStrip();
}

void Gfx_RestoreTable(void)
{
	Gfx_Flush();                              // pending plan first, or its draws would land on the fresh table
	Gfx_Copy(0, PAGE1 + TBL_Y, 0, TBL_Y, 256, 166);
	g_Drawn = 0; g_DotN = 0;
}

// 8-bit neighbourhood test on top-left pixel coordinates (a ball is 8x8, an erase covers 10x8)
#define NEAR8(ax, ay, bx, by)  ((u8)((ax) - (bx) + 11) < 23 && (u8)((ay) - (by) + 9) < 19)

static u8 g_NX[SP_NUM_BALLS], g_NY[SP_NUM_BALLS];
static u8 g_P[2 * SP_NUM_BALLS];            // final top-left position of every ball: x[10] then y[10], 0xFF = none

// Index of the first ball (>= start) whose rectangle touches the one at (x, y), or 0xFF.
static u8 NextNear(u8 start, u8 x, u8 y)
{
	sp_scan_px = g_P;
	sp_scan_xl = (u8)(x - 11); sp_scan_wx = 23;
	sp_scan_yl = (u8)(y - 9);  sp_scan_wy = 19;
	return SP_ScanWindow(start);
}

void Gfx_Balls(SP_Table* t)
{
	u16 m = t->moved, bit, mv = 0, dr = 0, uns = 0;
	u8 i, j, x, y;

	t->moved = 0;
	if (!m) return;                                    // the physics touched nothing

	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
	{
		if (m & bit)
		{
			if (t->active & bit)
			{
				x = (u8)(SP_PX(t->ball[i].x) - 4);
				y = (u8)(SP_PX(t->ball[i].y) - 4 + TBL_Y);
				g_NX[i] = x; g_NY[i] = y;
				if (!(g_Drawn & bit) || x != g_BX[i] || y != g_BY[i]) mv |= bit;
			}
			else if (g_Drawn & bit) mv |= bit;          // just pocketed: erase only
		}
	}
	if (!mv) return;

	// where every ball will be once this refresh ends
	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
	{
		if (!(t->active & bit)) { g_P[i] = g_P[i + SP_NUM_BALLS] = 0xFF; }
		else if (mv & bit)      { g_P[i] = g_NX[i]; g_P[i + SP_NUM_BALLS] = g_NY[i]; }
		else                    { g_P[i] = g_BX[i]; g_P[i + SP_NUM_BALLS] = g_BY[i]; }
	}

	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
		if ((mv & bit) && (g_Drawn & bit)) EraseSpot(g_BX[i], g_BY[i]);

	// redraw the moved balls and every ball whose pixels the erase / the new draw may have touched
	dr = mv & t->active;
	for (j = 0, bit = 1; j < SP_NUM_BALLS; j++, bit <<= 1)
	{
		if (!(mv & bit)) continue;
		if (g_Drawn & bit)
			for (i = NextNear(0, g_BX[j], g_BY[j]); i != 0xFF; i = NextNear(i + 1, g_BX[j], g_BY[j]))
				dr |= (u16)(1u << i);
		if (t->active & bit)
			for (i = NextNear(0, g_NX[j], g_NY[j]); i != 0xFF; i = NextNear(i + 1, g_NX[j], g_NY[j]))
				dr |= (u16)(1u << i);
	}

	// solid (fast) draws first, then the transparent ones on top of them
	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
	{
		if (!(dr & bit)) continue;
		x = g_P[i]; y = g_P[i + SP_NUM_BALLS];
		if (!Flat(x, y)) { uns |= bit; continue; }
		for (j = NextNear(0, x, y); j != 0xFF; j = NextNear(j + 1, x, y))
			if (j != i) { uns |= bit; break; }
		if (!(uns & bit)) DrawSolid(i, x, y);
	}
	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
		if (uns & bit) DrawCell(i, g_P[i], g_P[i + SP_NUM_BALLS]);

	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
		if (mv & bit)
		{
			if (t->active & bit) { g_BX[i] = g_NX[i]; g_BY[i] = g_NY[i]; g_Drawn |= bit; }
			else g_Drawn &= ~bit;
		}
}

// Aim guide: erase the old dots (redrawing any ball they covered), then draw the new ones.
void Gfx_Aim(SP_Table* t, u8 angle, bool show)
{
	u8 i, j;
	for (i = 0; i < g_DotN; i++) EraseSpot(g_DotX[i], g_DotY[i]);
	for (j = 0; j < SP_NUM_BALLS; j++)
		if ((g_Drawn >> j) & 1)
			for (i = 0; i < g_DotN; i++)
				if (NEAR8(g_BX[j], g_BY[j], g_DotX[i], g_DotY[i])) { DrawCell(j, g_BX[j], g_BY[j]); break; }
	g_DotN = 0;
	if (show && (t->active & 1))
	{
		i16 cx = SP_PX(t->ball[0].x), cy = SP_PX(t->ball[0].y);
		i16 c = SP_Cos(angle), s = SP_Sin(angle);
		for (i = 1; i <= AIM_DOTS; i++)
		{
			i16 d = (i16)i * 14 + 6;
			i16 x = cx + (i16)(((i32)c * d) >> 8), y = cy + (i16)(((i32)s * d) >> 8);
			if (x < 18 || x > 238 || y < 28 || y > 140) break;
			g_DotX[g_DotN] = (u8)(x - 4); g_DotY[g_DotN] = (u8)(y - 4 + TBL_Y);
			{
				bool solid = Flat(g_DotX[g_DotN], g_DotY[g_DotN]);
				for (j = 0; solid && j < SP_NUM_BALLS; j++)
					if (((g_Drawn >> j) & 1) && NEAR8(g_DotX[g_DotN], g_DotY[g_DotN], g_BX[j], g_BY[j])) solid = FALSE;
				if (solid) DrawSolid(CELL_DOT, g_DotX[g_DotN], g_DotY[g_DotN]);
				else       DrawCell(CELL_DOT, g_DotX[g_DotN], g_DotY[g_DotN]);
			}
			g_DotN++;
		}
	}
}

//-----------------------------------------------------------------------------
// HUD (the original panel scaled to 256x45; only the dynamic parts are drawn here)
void Hud_Score(u16 score)
{
	Text_DrawNum(36, 13, score, 5, COL_WHITE, COL_BLACK);
}

void Hud_Shots(u8 n)
{
	u8 i;
	for (i = 0; i < 8; i++) Gfx_Fill(20 + i * 6, 26, 4, 4, i < n ? COL_CREAM : COL_BLACK);
}

void Hud_Next(u8 ball)
{
	Gfx_Fill(148, 3, 32, 11, COL_BLACK);
	if (ball) { Gfx_Wait(); VDP_CommandLMMM((u16)ball * 8, STRIP_Y, 160, 4, 8, 8, VDP_OP_TIMP); }
	else Text_Draw(162, 5, "-", COL_WHITE, COL_BLACK);
}

void Hud_Power(u8 power)
{
	u8 i, lit = (u8)(((u16)power * 7) >> 8) + (power ? 1 : 0);
	Gfx_Copy(136, PAGE1 + 16, 136, 16, 32, 10);                  // original gauge art back from the clean page...
	for (i = 0; i < lit; i++)                             // ...then the lit bars on top
		Gfx_Fill(137 + i * 4, 24 - (i + 1), 3, i + 1, COL_GOLD);
}

void Hud_Label(const c8* line1, const c8* line2)
{
	Gfx_Fill(78, 3, 38, 30, COL_BLACK);
	if (line1) Text_Draw(80, 6, line1, COL_WHITE, COL_BLACK);
	if (line2) Text_Draw(80, 18, line2, COL_CREAM, COL_BLACK);
}
