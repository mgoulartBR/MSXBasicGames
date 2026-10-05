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

#ifdef DEBUG_KEYS
// frame-time profiling (debug build): worst number of video frames one main-loop iteration needed, per screen
volatile u8 g_perf[12];          // [screen] = max frames per iteration (1 = on time), [10] = iterations over 1 frame
#define JIFFY (*(volatile u16*)0xFC9E)
#endif

#ifdef PERFTEST
volatile u16 g_pt[16];
#define JIF (*(volatile u16*)0xFC9E)
static u16 timeit(u8 which)
{
	u16 t = JIF;
	switch (which)
	{
		case 0: for (u8 i = 0; i < 50; i++) VDP_CommandHMMM(0, 256 + 0, 70, 120, 24, 32); break;               // HMMM card
		case 1: for (u8 i = 0; i < 50; i++) VDP_CommandLMMM(0, 256 + 0, 70, 120, 24, 32, VDP_OP_TIMP); break;  // LMMM TIMP card
		case 2: for (u8 i = 0; i < 50; i++) VDP_CommandHMMV(66, 0, 190, 38, 0x11); break;                       // HMMV 190x38
		case 3: for (u8 i = 0; i < 50; i++) VDP_CommandLMMV(65, 0, 191, 38, 1, VDP_OP_IMP); break;              // LMMV
		case 4: for (u8 i = 0; i < 100; i++) VDP_CommandLMMM(0, FONT_Y0, 70, 100, 5, 9, VDP_OP_TIMP); break;    // glyph LMMM
		case 5: for (u8 i = 0; i < 100; i++) VDP_CommandHMMM(0, FONT_Y0, 70, 100, 6, 9); break;                 // glyph HMMM (even)
	}
	VDP_CommandWait();
	return JIF - t;
}
static void perftest(void)
{
	Vid_Init();
	Vid_Sync();
	for (u8 w = 0; w < 6; w++) g_pt[w] = timeit(w);                       // display + sprites on
	VDP_EnableSprite(FALSE);
	for (u8 w = 0; w < 6; w++) g_pt[6 + w] = timeit(w);                   // sprites off
	VDP_EnableDisplay(FALSE);
	for (u8 w = 0; w < 4; w++) g_pt[12 + w] = timeit(w);                  // display off
	g_pt[15] = 1;
	while (1) {}
}
#endif

void main()
{
#ifdef PERFTEST
	perftest();
#endif
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
#ifdef DEBUG_KEYS
	u16 lastJiffy = JIFFY;
	for (u8 i = 0; i < 12; i++) g_perf[i] = 0;
#endif
	while (1)
	{
#ifdef DEBUG_KEYS
		{ u16 now = JIFFY; u8 d = (u8)(now - lastJiffy); lastJiffy = now; if (ui.screen < 10 && d > g_perf[ui.screen] && d < 200) g_perf[ui.screen] = d; if (d > 1 && d < 200) g_perf[10]++; }
#endif
		Vid_Sync();
		Snd_Update();
		Input_Update();
		if (in.pressed & IN_MUTE) Snd_Music(!Snd_MusicOn());
		ui_update();
	}
}
