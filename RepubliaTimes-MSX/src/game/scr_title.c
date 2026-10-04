#include "ui.h"
#include "../platform/msx_seg.h"
#include "logic.h"
#include "../data/gfx_data.h"

// Control hints and start prompt follow the active device (keyboard/joystick or mouse)
static void title_controls(void)
{
	Gfx_FillPattern(15, 15, 17, 3, 0, 0);
	if (g_MouseOn)
	{
		Gfx_Text(15, 15, 17, "Mouse: point to select", CT_INK);
		Gfx_Text(15, 16, 17, "Left click: confirm", CT_INK);
		Gfx_Text(15, 17, 17, "Right click: back", CT_INK);
	}
	else
	{
		Gfx_Text(15, 15, 17, "Arrows / joystick: move", CT_INK);
		Gfx_Text(15, 16, 17, "SPACE / button A: confirm", CT_INK);
		Gfx_Text(15, 17, 17, "ESC / TAB / button B: back", CT_INK);
	}
}

static const char* start_text(void)
{
	return g_MouseOn ? "Click to start" : "Press SPACE to start";
}

u8 Scr_Title(void)
{
	Seg_Set(SEG_GFX);
	Gfx_Display(FALSE);
	Gfx_Clear(CT_INK);
	Gfx_Blit(0, 1, g_Gfx_logo, GFX_LOGO_W, GFX_LOGO_H);
	Gfx_Blit(1, 9, g_Gfx_ministry, GFX_MINISTRY_W, GFX_MINISTRY_H);
	Gfx_Text(15, 9, 17, "Original game by", CT_INK);
	Gfx_Text(15, 10, 17, "Lucas Pope (@dukope)", CT_INK);
	Gfx_Text(15, 12, 17, "MSX port by", CT_INK);
	Gfx_TextCenter(15, 13, 17, "BIGFIVE STUDIOS", CT_INV);
	title_controls();
	Gfx_TextCenter(0, 22, 32, "Unofficial fan port. Non-commercial.", CT_INK);
	Gfx_Text(0, 23, 32, "MSX1  64K ROM (ASCII8)  v" GAME_VERSION, CT_INK);
	Gfx_Display(TRUE);
	Music_Play(MUSIC_MORNING);

	u16 t = 0;
	u8 lastMouse = g_MouseOn;
	Gfx_TextCenter(0, 20, 32, start_text(), CT_INK);
	Ui_WaitRelease();
	for (;;)
	{
		Ui_Frame();
		++t;
		if (g_MouseOn != lastMouse)
		{
			lastMouse = g_MouseOn;
			title_controls();
			t |= 31; // force a prompt redraw this frame
			t &= ~32;
		}
		if (!(t & 31))
			Gfx_TextCenter(0, 20, 32, (t & 32) ? "" : start_text(), CT_INK);
		if (g_Push & IN_A)
			break;
	}
	Sfx_Play(SFX_CLICK);
	Rand_Seed(t * 7919u + 0x1234);
	Game_Reset();
	g_Game.stateInControl = TRUE;
	g_Game.wonOnce = FALSE;
	return ST_MORNING;
}
