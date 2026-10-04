#include "ui.h"
#include "../platform/msx_seg.h"
#include "logic.h"
#include "widgets.h"
#include "../data/gfx_data.h"

static char s_Msg[900];

u8 Scr_Night(void)
{
	{
		u16 old = Seg_Set(SEG_TEXT);
		Text_Night(s_Msg);
		Seg_Restore(old);
	}
	Seg_Set(SEG_GFX);
	Gfx_Display(FALSE);
	Gfx_Clear(CT_INV);
	Gfx_Blit(0, 1, g_Gfx_presses, GFX_PRESSES_W, GFX_PRESSES_H);
	Gfx_FillColor(0, 1, GFX_PRESSES_W, GFX_PRESSES_H, CT_INK);
	Meter_Draw(27, 11, g_Game.loyalty);
	Readers_Draw(27, 17, g_Game.readers, Game_ReadersDelta(), TRUE, FALSE);
	Gfx_Display(TRUE);
	Music_Play(MUSIC_NIGHT);

	Ui_ShowPages(s_Msg, 1, 10, 11, 25, CT_INV, "Go to Sleep", 22, CT_INK);
	g_Game.day++;
	return ST_MORNING;
}
