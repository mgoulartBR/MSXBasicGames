// Side Pocket for MSX2 (MSXGL, Screen 5): title, main menu, game modes.  Screens come from the ROM blob.
#include "msxgl.h"
#include "gen/blob.h"
#include "blob_io.h"
#include "text.h"
#include "gfx.h"
#include "game.h"
#include "../src/tricks.h"

#define TRICK_X(c)   (LAY_TRICK_GX + (c) * LAY_TRICK_GW / 5)
#define TRICK_Y(r)   (LAY_TRICK_GY + (r) * LAY_TRICK_GH / 4)

static u8 CenterX(const c8* s, u8 charW) { return (u8)((256 - Text_Width(s) / 6 * charW) / 2) & 0xFE; }

//-----------------------------------------------------------------------------
// Title: the original picture, with PRESS START BUTTON blinking
static void Title(void)
{
	u8 t = 0, on = 1;
	Gfx_Show(FALSE);
	Blob_LoadImage(BLOB_TITLE_SEG, BLOB_TITLE_OFF, 0, 0);
	Gfx_Show(TRUE);
	do
	{
		Frame();
		if (++t == 40)
		{
			t = 0; on ^= 1;
			if (on) Blob_LoadImage(BLOB_TITLE_ON_SEG, BLOB_TITLE_ON_OFF, 0, 0);
			else    Blob_LoadImage(BLOB_TITLE_OFF_SEG, BLOB_TITLE_OFF_OFF, 0, 0);
		}
	} while (!g_Press.fire);
}

//-----------------------------------------------------------------------------
// Main menu: the portrait with the original panel; the five highlight states are preloaded off screen
static void Menu_Patch(u8 k)
{
	Gfx_Copy(IMG_MENU_S0_XB * 2, 256 + k * IMG_MENU_S0_H, IMG_MENU_S0_XB * 2, IMG_MENU_S0_Y, IMG_MENU_S0_W * 2, IMG_MENU_S0_H);
}

static u8 s_MenuSel, s_TrickSel;          // (initialised in main: the RAM is not cleared at startup)

static u8 Menu(void)
{
	u8 sel = s_MenuSel;
	static const u16 seg[5] = { BLOB_MENU_S0_SEG, BLOB_MENU_S1_SEG, BLOB_MENU_S2_SEG, BLOB_MENU_S3_SEG, BLOB_MENU_S4_SEG };
	static const u16 off[5] = { BLOB_MENU_S0_OFF, BLOB_MENU_S1_OFF, BLOB_MENU_S2_OFF, BLOB_MENU_S3_OFF, BLOB_MENU_S4_OFF };
	u8 k;

	Gfx_Show(FALSE);
	Blob_LoadImage(BLOB_MENU_SEG, BLOB_MENU_OFF, 0, 0);
	for (k = 0; k < 5; k++)
		Blob_LoadImage((u8)seg[k], off[k], 256 + k * IMG_MENU_S0_H - IMG_MENU_S0_Y, 0);
	Menu_Patch(sel);
	Gfx_Show(TRUE);
	for (;;)
	{
		Frame();
		if (g_Press.down) { sel = sel == 4 ? 0 : sel + 1; Menu_Patch(sel); }
		if (g_Press.up)   { sel = sel == 0 ? 4 : sel - 1; Menu_Patch(sel); }
		if (g_Press.fire) { s_MenuSel = sel; return sel; }
	}
}

//-----------------------------------------------------------------------------
// Stage introduction (Pocket Game): portrait, stage number and target score
void Screen_StageIntro(u8 stage, u16 target)
{
	c8 buf[3];
	Gfx_Show(FALSE);
	Blob_LoadImage(BLOB_STAGE1_SEG, BLOB_STAGE1_OFF, 0, 0);
	Gfx_Fill(106, 0, 150, 212, LAY_ST_BLACK);
	buf[0] = stage >= 10 ? '0' + stage / 10 : ' '; buf[1] = '0' + stage % 10; buf[2] = 0;
	Text_DrawBig(134, 20, "STAGE", LAY_ST_WHITE, LAY_ST_BLACK);
	Text_DrawBig(210, 20, buf, LAY_ST_RED, LAY_ST_BLACK);
	if (stage == 1) { Text_DrawBig(134, 56, "LOS", LAY_ST_WHITE, LAY_ST_BLACK); Text_DrawBig(134, 76, "ANGELES", LAY_ST_WHITE, LAY_ST_BLACK); }
	Text_DrawBig(134, 112, "TARGET", LAY_ST_WHITE, LAY_ST_BLACK);
	Text_DrawBig(134, 130, "SCORE", LAY_ST_WHITE, LAY_ST_BLACK);
	Text_DrawNum(150, 156, target, 5, LAY_ST_RED, LAY_ST_BLACK);
	Text_Draw(184, 156, "PTS", LAY_ST_WHITE, LAY_ST_BLACK);
	Text_Draw(150, 196, "PRESS FIRE", LAY_ST_WHITE, LAY_ST_BLACK);
	Gfx_Show(TRUE);
	do Frame(); while (!g_Press.fire);
}

//-----------------------------------------------------------------------------
// 2P match screens.  Nine ball: choose the race (3 / 7 / 15 racks).  Pocket: 5 set match.
static u8 Screen_Vs(u8 pocket)
{
	static const u16 seg[3] = { BLOB_VS_R3_SEG, BLOB_VS_R7_SEG, BLOB_VS_R15_SEG };
	static const u16 off[3] = { BLOB_VS_R3_OFF, BLOB_VS_R7_OFF, BLOB_VS_R15_OFF };
	static const u8 race[3] = { 3, 7, 15 };
	u8 sel = 0, k;

	Gfx_Show(FALSE);
	Blob_LoadImage(BLOB_VS_SEG, BLOB_VS_OFF, 0, 0);
	if (pocket) Blob_LoadImage(BLOB_VS_POCKET_SEG, BLOB_VS_POCKET_OFF, 0, 0);
	else for (k = 0; k < 3; k++)
		Blob_LoadImage((u8)seg[k], off[k], 256 + k * 24 - IMG_VS_R3_Y, 0);
	Gfx_Show(TRUE);
	for (;;)
	{
		Frame();
		if (g_Press.back) return 0;
		if (g_Press.fire) return pocket ? 1 : race[sel];
		if (!pocket && (g_Press.right || g_Press.left))
		{
			if (g_Press.right) sel = sel == 2 ? 0 : sel + 1; else sel = sel == 0 ? 2 : sel - 1;
			Gfx_Copy(IMG_VS_R3_XB * 2, 256 + sel * 24, IMG_VS_R3_XB * 2, IMG_VS_R3_Y, IMG_VS_R3_W * 2, IMG_VS_R3_H);
		}
	}
}

//-----------------------------------------------------------------------------
// Trick game: pick one of the 19 tricks (the last cell is EXIT)
static void TrickCell(u8 idx, u8 col)
{
	u8 c = idx % 5, r = idx / 5;
	u16 x = TRICK_X(c) & ~1u, y = TRICK_Y(r), w = (LAY_TRICK_GW / 5 + 2) & ~1u, h = LAY_TRICK_GH / 4;
	Gfx_Outline(x, y, w, h, col);
}

static void TrickRestore(u8 idx)
{
	u8 c = idx % 5, r = idx / 5;
	u16 x = (TRICK_X(c) - 2) & ~1u, y = TRICK_Y(r) - 1, w = (LAY_TRICK_GW / 5 + 6) & ~1u, h = LAY_TRICK_GH / 4 + 2;
	Gfx_Copy(x, 256 + y, x, y, w, h);
}

static void TrickMarks(void)
{
	u8 i;
	for (i = 0; i < TRICK_COUNT; i++)
		if (g_TrickDone[i >> 3] & (1 << (i & 7)))
			Gfx_Fill(TRICK_X(i % 5) + LAY_TRICK_GW / 10 - 2, TRICK_Y(i / 5) + LAY_TRICK_GH / 8 - 2, 5, 5, LAY_TK_YELLOW);
}

static u8 TrickSelect(void)
{
	u8 sel = s_TrickSel;
	c8 lab[12];
	u8 old;

	Gfx_Show(FALSE);
	Blob_LoadImage(BLOB_TRICK_SEG, BLOB_TRICK_OFF, 0, 1);          // page 1 keeps a clean copy for the cursor
	TrickMarks();
	Gfx_Show(TRUE);
	for (;;)
	{
		if (sel == 19) { lab[0] = 'E'; lab[1] = 'X'; lab[2] = 'I'; lab[3] = 'T'; lab[4] = 0; }
		else { lab[0] = 'T'; lab[1] = 'R'; lab[2] = 'I'; lab[3] = 'C'; lab[4] = 'K'; lab[5] = ' '; lab[6] = '0' + (sel + 1) / 10; lab[7] = '0' + (sel + 1) % 10; lab[8] = 0; }
		Text_Draw(CenterX(lab, 6), 200, lab, LAY_TK_WHITE, LAY_TK_BLACK);
		TrickCell(sel, LAY_TK_GREEN);
		old = sel;
		do
		{
			Frame();
			if (g_Press.back) return 0xFF;
			if (g_Press.fire) { s_TrickSel = sel; return sel == 19 ? 0xFF : sel; }
			if (g_Press.right) sel = (sel % 5 == 4) ? sel - 4 : sel + 1;
			if (g_Press.left)  sel = (sel % 5 == 0) ? sel + 4 : sel - 1;
			if (g_Press.down)  sel = sel >= 15 ? sel - 15 : sel + 5;
			if (g_Press.up)    sel = sel < 5 ? sel + 15 : sel - 5;
		} while (sel == old);
		TrickRestore(old);
		TrickMarks();
	}
}

//-----------------------------------------------------------------------------
// Jukebox: the real track list (21 songs).  There is no sound engine yet.
static const c8* const s_Songs[21] = {
	"Whispers", "The Lounge", "Groovin'", "Cue Ball Boogie", "Play'n It Cool!", "Las Vegas", "Come on Over",
	"Sunny Morning", "Midnight Kiss", "Lazy Afternoon", "Natural Shot!", "Slow Down, Dude", "Ace In The Hole",
	"Meteor-Man", "Round And Round", "Dinner for Two", "Stylin'", "Fight It Out", "Chillin'", "California Lemonade",
	"Don't Go, Baby"
};

static void SongName(const c8* s)
{
	Gfx_Fill(LAY_JK_X, LAY_JK_Y, LAY_JK_W, LAY_JK_H, LAY_JK_BG);
	Text_Draw((u8)(LAY_JK_X + (LAY_JK_W - Text_Width(s)) / 2) & 0xFE, LAY_JK_Y, s, LAY_JK_FG, LAY_JK_BG);
}

static void Jukebox(void)
{
	u8 sel = 0;
	Gfx_Show(FALSE);
	Blob_LoadImage(BLOB_JUKE_SEG, BLOB_JUKE_OFF, 0, 0);
	SongName(s_Songs[0]);
	Gfx_Show(TRUE);
	for (;;)
	{
		Frame();
		if (g_Press.back) return;
		if (g_Press.right) { sel = sel == 20 ? 0 : sel + 1; SongName(s_Songs[sel]); }
		if (g_Press.left)  { sel = sel == 0 ? 20 : sel - 1; SongName(s_Songs[sel]); }
		if (g_Press.fire)  { SongName("(NO SOUND ENGINE YET)"); WaitFrames(90); SongName(s_Songs[sel]); }
	}
}

//-----------------------------------------------------------------------------
void main()
{
	u8 sel, r, k;

	Gfx_Init();
	EnableInterrupt();
	g_TrickDone[0] = g_TrickDone[1] = g_TrickDone[2] = 0;
	s_MenuSel = s_TrickSel = 0;
	g_Held.up = g_Held.down = g_Held.left = g_Held.right = g_Held.fire = g_Held.back = 0;
	for (;;)
	{
		Title();
		for (;;)
		{
			sel = Menu();
			if (sel == 0) Game_Pocket(1);
			else if (sel == 1) { if (Screen_Vs(1)) Game_Pocket(2); }
			else if (sel == 2) { r = Screen_Vs(0); if (r) Game_Nine(r); }
			else if (sel == 3) { for (;;) { k = TrickSelect(); if (k == 0xFF) break; Game_Trick(k); } }
			else Jukebox();
		}
	}
}
