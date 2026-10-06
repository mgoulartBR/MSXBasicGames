// MadTV-MSX - milestone 0.1: prova do pipeline (boot, video, input, texto)
#include "msxgl.h"
#include "font/font_mgl_sample6.h"

void main()
{
	VDP_SetMode(VDP_MODE_SCREEN5);
	VDP_SetColor(COLOR_BLACK);
	VDP_EnableVBlank(TRUE);
	VDP_ClearVRAM();

	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Print_SetColor(COLOR_WHITE, COLOR_BLACK);
	Print_SetPosition(8, 8);
	Print_DrawText("MadTV-MSX  v0.1");
	Print_SetPosition(8, 24);
	Print_DrawText("Pipeline OK: MSXgl + SDCC");

	while (!Keyboard_IsKeyPressed(KEY_ESC))
		Halt();

	BIOS_Exit(0);
}
