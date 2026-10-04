#include "msx_isr.h"
#include "msx_input.h"
#include "msx_audio.h"
#include "bios_hook.h"

volatile u8 g_Frames;

static void VBlankHook(void)
{
	Input_Sample();
	Audio_Update();
	++g_Frames;
}

void Isr_Init(void)
{
	g_Frames = 0;
	BIOS_SetHookCallback(H_TIMI, VBlankHook);
}
