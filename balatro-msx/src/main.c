// Balatro MSX - entry point. All the game logic and UI live in banked code segments (bank 2).
#include "msxgl.h"
#include "platform/pvideo.h"
#include "platform/ctrl.h"
#include "ui/ui.h"

void main()
{
	Vid_Init();
	Input_Init();
	ui_init();
	while (1)
	{
		Vid_Sync();
		Input_Update();
		ui_update();
	}
}
