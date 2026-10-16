#include "msx_isr.h"
#include "msx_input.h"
#include "msx_audio.h"
#include "bios_hook.h"

volatile u8 g_Frames;

void Isr_Hook(void)
{
	Input_Sample();
	Audio_Update();
	++g_Frames;
}

// H.TIMI is patched with a plain JP to this trampoline (no BIOS inter-slot call, whose behaviour and
// register usage differ between BIOS versions). It saves EVERY register, IX/IY and the alternate set
// included, so neither the C hook nor the MSXgl port routines can disturb the interrupted program.
// The ISR runs with interrupts disabled (INPUT_USE_ISR_PROTECTION is off: no EI inside the hook).
static void Isr_Tramp(void) __naked
{
	__asm
		push	af
		push	bc
		push	de
		push	hl
		push	ix
		push	iy
		ex	af, af'
		push	af
		exx
		push	bc
		push	de
		push	hl
		call	_Isr_Hook
		pop	hl
		pop	de
		pop	bc
		exx
		pop	af
		ex	af, af'
		pop	iy
		pop	ix
		pop	hl
		pop	de
		pop	bc
		pop	af
		ret
	__endasm;
}

void Isr_Init(void)
{
	g_Frames = 0;
	BIOS_SetHookDirectCallback(H_TIMI, Isr_Tramp);
}
