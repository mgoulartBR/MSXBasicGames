#include "ui.h"
#include "../platform/msx_seg.h"
#include "logic.h"
#include "widgets.h"
#include "../data/gfx_data.h"

static char s_Msg[1200];

u8 Scr_Morning(void)
{
	MorningResult res;
	char buf[12];

	Seg_Set(SEG_GFX);
	{
		u16 old = Seg_Set(SEG_TEXT);
		Text_Morning(s_Msg, &res);
		Seg_Restore(old);
	}
	Gfx_Display(FALSE);
	Gfx_Clear(CT_INK);
	if (g_Game.stateInControl)
		Gfx_Blit(0, 0, g_Gfx_logo, GFX_LOGO_W, GFX_LOGO_H);
	else
		Gfx_Blit(0, 0, g_Gfx_logo2, GFX_LOGO2_W, GFX_LOGO2_H);

	buf[0] = 'D'; buf[1] = 'a'; buf[2] = 'y'; buf[3] = ' ';
	{
		u8 d = g_Game.day, n = 4;
		if (d >= 10) buf[n++] = '0' + d / 10;
		buf[n++] = '0' + d % 10;
		buf[n] = 0;
	}
	Gfx_TextCenter(0, 6, 32, buf, CT_INK);

	u8 w = 30;
	if (g_Game.day > 1)
	{
		w = 25;
		Meter_Draw(27, 8, g_Game.loyalty);
		Readers_Draw(27, 14, g_Game.readers, 0, FALSE, TRUE);
	}
	Ui_Credits(23, CT_INK);
	Gfx_Display(TRUE);
	Music_Play(MUSIC_MORNING);

	Ui_ShowPages(s_Msg, 1, 7, 15, w, res.rebelsWon ? CT_RED : CT_INK, (res.rebelsWon ? "Let's Go!" : (res.gameOver ? "Accept Fate" : "Start Work")), 22, CT_INV);

	if (res.gameOver)
	{
		g_Game.wonOnce = TRUE; // same as the original (set on any game over)
		bool gov = g_Game.stateInControl;
		Game_Reset();
		g_Game.stateInControl = gov;
		return ST_MORNING;
	}
	return ST_PLAY;
}
