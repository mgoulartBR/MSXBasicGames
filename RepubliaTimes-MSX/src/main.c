// The Republia Times - MSX port by BigFive Studios
// Original game by Lucas Pope. See README.md / LICENSES.md.
#include "msxgl.h"
#include "platform/msx_gfx.h"
#include "platform/msx_input.h"
#include "platform/msx_audio.h"
#include "platform/msx_isr.h"
#include "game/ui.h"
#include "platform/msx_seg.h"

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

// Real-hardware diagnostic: the data segments (4-6) are not visible through the mapper window.
static void mapper_error(u8 seg)
{
	char n[3] = { '0' + seg, 0, 0 };
	Gfx_Clear(CT_INK);
	Gfx_TextCenter(0, 3, 32, "MAPPER ERROR", CT_INV);
	Gfx_Text(1, 6, 30, "The cartridge does not switch ROM", CT_INK);
	Gfx_Text(1, 7, 30, "banks the way this ROM expects.", CT_INK);
	Gfx_Text(1, 9, 30, "Failed segment:", CT_INK);
	Gfx_Text(10, 9, 4, n, CT_INK);
	Gfx_Text(1, 11, 30, "Use republia-msx-0.2.rom (ASCII8,", CT_INK);
	Gfx_Text(1, 12, 30, "8KB banks) or the -konami.rom", CT_INK);
	Gfx_Text(1, 13, 30, "build (Konami mapper, no SCC).", CT_INK);
	Gfx_Text(1, 14, 30, "Pick that mapper in the cart menu.", CT_INK);
	Gfx_Text(1, 17, 30, "Select the mapper in your flash", CT_INK);
	Gfx_Text(1, 18, 30, "cartridge menu / loader.", CT_INK);
	Gfx_TextCenter(0, 22, 32, CREDIT_PORT, CT_INK);
	for (;;)
		Halt();
}

void main()
{
	clear_ram_vars();
	Gfx_Init();
	{
		u8 bad = Seg_SelfTest();
		if (bad)
			mapper_error(bad);
	}
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
