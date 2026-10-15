// Side Pocket-style pool for MSX2 (MSXGL, Screen 5).
// Page 0: visible screen.  Page 1: clean copy of the table (to erase) + ball cell strip.
#include "msxgl.h"
#include "font/font_mgl_sample6.h"
#include "gen/assets.h"
#include "../src/sp_logic.h"

#define TBL_Y        46                 // screen line where the 166-line table image starts
#define PAGE1        256                // first line of page 1
#define STRIP_Y      (PAGE1 + 216)      // transparent 12x(8x8) ball cells, page 1 (for LMMM)
#define SOLID_EVEN_Y (PAGE1 + 224)      // same cells with the cloth colour in the corners (HMMM, even x)
#define SOLID_ODD_Y  (PAGE1 + 232)      // 10-wide cells [cloth|ball|cloth] (HMMM, odd x)
#define CELL_DOT     10
#define CELL_RING    11
#define AIM_DOTS     6
#define JIFFY        (*(volatile u16*)0xFC9E)   // BIOS 60 Hz (50 Hz on PAL) tick counter

enum { ST_AIM, ST_ROLL };

static SP_Table g_Table;
static u8  g_State;
static u8  g_Angle;
static u8  g_Power;
static u8  g_Charging;
static u16 g_Score;
static u8  g_Row[128];
u16 g_Tick;   // debug: main-loop iterations (read from the emulator)
static u8  g_Col, g_RowIdx;

//-----------------------------------------------------------------------------
// Table loading (RLE -> both pages)
static void Emit(u8 v)
{
	g_Row[g_Col++] = v;
	if (g_Col == 128)
	{
		u16 addr = (u16)(TBL_Y + g_RowIdx) * 128;
		VDP_WriteVRAM(g_Row, addr, 0, 128);
		VDP_WriteVRAM(g_Row, 0x8000 + addr, 0, 128);
		g_Col = 0;
		g_RowIdx++;
	}
}

static void LoadTable()
{
	const u8* p = g_TableRLE;
	const u8* end = g_TableRLE + TABLE_RLE_SIZE;
	g_Col = 0; g_RowIdx = 0;
	while (p < end)
	{
		u8 c = *p++;
		u8 n;
		if (c & 0x80)
		{
			u8 v = *p++;
			for (n = (c & 0x7F) + 1; n; n--) Emit(v);
		}
		else
			for (n = c + 1; n; n--) Emit(*p++);
	}
}

static void LoadStrip()
{
	u8 y;
	for (y = 0; y < 8; y++)
	{
		VDP_WriteVRAM(g_BallStrip + y * 48,        0x8000 + (u16)(STRIP_Y - PAGE1 + y) * 128, 0, 48);
		VDP_WriteVRAM(g_BallStrip + 384 + y * 48,  0x8000 + (u16)(SOLID_EVEN_Y - PAGE1 + y) * 128, 0, 48);
		VDP_WriteVRAM(g_BallStrip + 768 + y * 60,  0x8000 + (u16)(SOLID_ODD_Y - PAGE1 + y) * 128, 0, 60);
	}
}

//-----------------------------------------------------------------------------
// Drawing. Balls are redrawn only when they (or a neighbour they touch) changed.
static u8  g_BX[SP_NUM_BALLS], g_BY[SP_NUM_BALLS];
static u16 g_Drawn;                 // bit n = ball n is currently drawn at (g_BX[n], g_BY[n])
static u8 g_DotX[AIM_DOTS], g_DotY[AIM_DOTS], g_DotN;

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
static u8 q_op[Q_MAX], q_cell[Q_MAX], q_x[Q_MAX], q_y[Q_MAX], q_n;

static void Push(u8 op, u8 cell, u8 x, u8 y)
{
	if (q_n >= Q_MAX) return;
	q_op[q_n] = op; q_cell[q_n] = cell; q_x[q_n] = x; q_y[q_n] = y;
	q_n++;
}
static void EraseSpot(u8 x, u8 y)          { Push(OP_ERASE, 0, x, y); }
static void DrawSolid(u8 c, u8 x, u8 y)    { Push(OP_SOLID, c, x, y); }
static void DrawCell(u8 c, u8 x, u8 y)     { Push(OP_CELL, c, x, y); }

static void Flush()
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

static void RefreshBalls()
{
	u16 m = g_Table.moved, bit, mv = 0, dr = 0, uns = 0;
	u8 i, j, x, y;

	g_Table.moved = 0;
	if (!m) return;                                    // the physics touched nothing

	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
	{
		if (m & bit)
		{
			if (g_Table.active & bit)
			{
				x = (u8)(SP_PX(g_Table.ball[i].x) - 4);
				y = (u8)(SP_PX(g_Table.ball[i].y) - 4 + TBL_Y);
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
		if (!(g_Table.active & bit)) { g_P[i] = g_P[i + SP_NUM_BALLS] = 0xFF; }
		else if (mv & bit)           { g_P[i] = g_NX[i]; g_P[i + SP_NUM_BALLS] = g_NY[i]; }
		else                         { g_P[i] = g_BX[i]; g_P[i + SP_NUM_BALLS] = g_BY[i]; }
	}

	for (i = 0, bit = 1; i < SP_NUM_BALLS; i++, bit <<= 1)
		if ((mv & bit) && (g_Drawn & bit)) EraseSpot(g_BX[i], g_BY[i]);

	// redraw the moved balls and every ball whose pixels the erase / the new draw may have touched
	dr = mv & g_Table.active;
	for (j = 0, bit = 1; j < SP_NUM_BALLS; j++, bit <<= 1)
	{
		if (!(mv & bit)) continue;
		if (g_Drawn & bit)
			for (i = NextNear(0, g_BX[j], g_BY[j]); i != 0xFF; i = NextNear(i + 1, g_BX[j], g_BY[j]))
				dr |= (u16)(1u << i);
		if (g_Table.active & bit)
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
			if (g_Table.active & bit) { g_BX[i] = g_NX[i]; g_BY[i] = g_NY[i]; g_Drawn |= bit; }
			else g_Drawn &= ~bit;
		}
}

// Aim guide: erase the old dots (redrawing any ball they covered), then draw the new ones.
static void RefreshAim(bool show)
{
	u8 i, j;
	for (i = 0; i < g_DotN; i++) EraseSpot(g_DotX[i], g_DotY[i]);
	for (j = 0; j < SP_NUM_BALLS; j++)
		if ((g_Drawn >> j) & 1)
			for (i = 0; i < g_DotN; i++)
				if (NEAR8(g_BX[j], g_BY[j], g_DotX[i], g_DotY[i])) { DrawCell(j, g_BX[j], g_BY[j]); break; }
	g_DotN = 0;
	if (show && (g_Table.active & 1))
	{
		i16 cx = SP_PX(g_Table.ball[0].x), cy = SP_PX(g_Table.ball[0].y);
		i16 c = SP_Cos(g_Angle), s = SP_Sin(g_Angle);
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

static void DrawScore()
{
	VDP_CommandWait();
	VDP_CommandHMMV(8, 8, 120, 8, 0);
	Print_SetPosition(8, 8);
	Print_DrawText("SCORE ");
	Print_DrawInt(g_Score);
}

static void DrawPowerFrame()
{
	VDP_CommandWait();
	VDP_CommandHMMV(8, 28, 130, 10, 0);
	VDP_CommandWait();
	VDP_CommandLMMV(8, 28, 130, 1, 15, VDP_OP_IMP);
	VDP_CommandWait();
	VDP_CommandLMMV(8, 37, 130, 1, 15, VDP_OP_IMP);
}

static void DrawPower()      // fill only up to the current power (the bar only grows while charging)
{
	u16 w = (u16)(g_Power >> 1) & ~1u;       // HMMV works on byte pairs: even width, colour = two pixels
	if (!w) return;
	VDP_CommandWait();
	VDP_CommandHMMV(10, 30, w, 6, 0xCC);
}

//-----------------------------------------------------------------------------
// Game
static void NewRack()
{
	SP_Init(&g_Table);
	g_Angle = 128; g_Power = 0; g_Charging = 0; g_State = ST_AIM;
}

static void ResetPlay()
{
	// repaint the clean table, then draw everything again
	Flush();                                // pending plan first, or its draws would land on the fresh table
	VDP_CommandWait();
	VDP_CommandHMMM(0, PAGE1 + TBL_Y, 0, TBL_Y, 256, 166);
	g_Drawn = 0;
	g_DotN = 0;
	NewRack();
	RefreshBalls();
	RefreshAim(TRUE);
}

void main()
{
	u8 held = 0, i;
	u16 lastJiffy = JIFFY;

	VDP_SetMode(VDP_MODE_SCREEN5);
	VDP_SetLineCount(VDP_LINE_212);
	VDP_EnableDisplay(FALSE);
	VDP_EnableSprite(FALSE);                     // no hardware sprites: VDP commands run faster
	VDP_SetColor(0);
	VDP_SetPalette(g_Palette + 2);              // MSXGL loads entries 1..15
	VDP_SetPaletteEntry(0, 0);                   // entry 0 = black (transparent colour of the ball blits)
	VDP_CommandHMMV(0, 0, 256, 512, 0);
	VDP_CommandWait();
	LoadTable();
	LoadStrip();

	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Print_SetColor(15, 0);
	Print_SetPosition(8, 0);
	Print_DrawText("SIDE POCKET  MSX2");
	Print_SetPosition(8, 18);
	Print_DrawText("ARROWS AIM   SPACE SHOOT");

	g_Score = 0;
	g_Drawn = 0;
	g_DotN = 0;
	NewRack();
	RefreshBalls();
	RefreshAim(TRUE);
	DrawScore();
	DrawPowerFrame();
	Flush();
	VDP_EnableDisplay(TRUE);

	for (;;)
	{
		u8 joy, key, left, right, fire;

		Halt();
		Flush();                                  // vblank has just started: run last frame's plan
		g_Tick++;

		joy = Joystick_Read(JOY_PORT_1);
		key = Keyboard_Read(8);
		left  = ((joy & JOY_INPUT_DIR_LEFT)  == 0) || ((key & (1 << 4)) == 0);
		right = ((joy & JOY_INPUT_DIR_RIGHT) == 0) || ((key & (1 << 7)) == 0);
		fire  = ((joy & JOY_INPUT_TRIGGER_A) == 0) || ((key & 1) == 0);

		if (g_State == ST_AIM)
		{
			u8 changed = 0;
			if (left)  { g_Angle -= (held > 20) ? 2 : 1; changed = 1; }
			if (right) { g_Angle += (held > 20) ? 2 : 1; changed = 1; }
			held = (left || right) ? (held < 255 ? held + 1 : held) : 0;

			if (fire)
			{
				if (g_Power < 252) { g_Power += 3; DrawPower(); }
				g_Charging = 1;
			}
			else if (g_Charging)
			{
				u8 p = (g_Power < 24) ? 24 : g_Power;
				SP_Shoot(&g_Table, g_Angle, p);
				g_Power = 0; g_Charging = 0;
				DrawPowerFrame();
				RefreshAim(FALSE);
				g_State = ST_ROLL;
				lastJiffy = JIFFY;
				changed = 0;
			}
			if (changed) RefreshAim(TRUE);
		}
		else // ST_ROLL
		{
			// Simulate one step per elapsed 1/60 s (BIOS JIFFY), so a slow frame never slows the game.
			u8 moving = 1, steps = (u8)(JIFFY - lastJiffy);
			lastJiffy = JIFFY;
			if (steps == 0) steps = 1;
			if (steps > 3) steps = 3;
			while (steps-- && moving) moving = SP_Step(&g_Table);
			RefreshBalls();
			if (!moving)
			{
				u8 cnt = 0, left_over = 0;
				u8 m = g_Table.pocketed_mask_lo;
				for (i = 1; i < 8; i++) if (m & (1 << i)) cnt++;
				cnt += (g_Table.pocketed_mask_hi & 1) + ((g_Table.pocketed_mask_hi >> 1) & 1);
				g_Score += (u16)cnt * 100;
				for (i = 1; i < SP_NUM_BALLS; i++) if (SP_ACTIVE(&g_Table, i)) left_over++;
				if (g_Table.scratch)
				{
					SP_PlaceCue(&g_Table);
					g_Score = (g_Score >= 50) ? g_Score - 50 : 0;
					RefreshBalls();
				}
				DrawScore();
				if (left_over == 0) ResetPlay();
				else { g_State = ST_AIM; RefreshAim(TRUE); }
			}
		}
	}
}
