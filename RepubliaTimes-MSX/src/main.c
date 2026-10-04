// The Republia Times - MSX port by BigFive Studios
// Original game by Lucas Pope. See README.md / LICENSES.md.
#include "msxgl.h"
#include "platform/msx_gfx.h"
#include "platform/msx_input.h"
#include "platform/msx_audio.h"
#include "platform/msx_isr.h"
#include "game/ui.h"

// The MSXgl crt0 copies initialised variables but does NOT clear the rest of RAM: uninitialised
// globals/statics contain garbage at power-up. Clear [s__DATA, s__INITIALIZED) (linker area symbols)
// before anything else; initialised variables live after it and are untouched.
u8* clr_start;
u8* clr_end;

static void clear_ram_vars(void)
{
	u8* start;
	u8* end;
	__asm
		ld	hl, #s__DATA
		ld	(_clr_start), hl
		ld	hl, #s__INITIALIZED
		ld	(_clr_end), hl
	__endasm;
	start = clr_start;
	end = clr_end;
	Mem_Set(0, start, (u16)(end - start));
}

void main()
{
	clear_ram_vars();
	Gfx_Init();
	Input_Init();
	Audio_Init();
	Isr_Init();

	u8 state = ST_TITLE;
	for (;;)
	{
		switch (state)
		{
		case ST_TITLE:   state = Scr_Title();   break;
		case ST_MORNING: state = Scr_Morning(); break;
		case ST_PLAY:    state = Scr_Play();    break;
		case ST_NIGHT:   state = Scr_Night();   break;
		}
	}
}
