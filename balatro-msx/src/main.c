// Balatro MSX - entry point. All the game logic and UI live in banked code segments (bank 2).
#include "msxgl.h"
#include "platform/pvideo.h"
#include "platform/ctrl.h"
#include "platform/audio.h"
#include "ui/ui.h"

#ifdef SELFTEST
extern void run_all_cases(void) __banked;
extern volatile u8 g_selftest[10];
extern void selftest_clear(void) __banked;
#endif

void main()
{
#ifdef SELFTEST
	SET_BANK_SEGMENT(3, SEG_TEXT);        // data tables live in the bank-3 window
	selftest_clear();
	run_all_cases();
	g_selftest[0] = 1;
	while (1) { }
#endif
	Vid_Init();
	Input_Init();
	Snd_Init(Vid_Hz());
	ui_init();
	while (1)
	{
		Vid_Sync();
		Snd_Update();
		Input_Update();
		if (in.pressed & IN_MUTE) Snd_Music(!Snd_MusicOn());
		ui_update();
	}
}
