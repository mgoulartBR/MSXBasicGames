// The Republia Times - MSX port by BigFive Studios
// Original game by Lucas Pope. See README.md / LICENSES.md.
#include "msxgl.h"
#include "platform/msx_gfx.h"
#include "platform/msx_input.h"
#include "platform/msx_audio.h"
#include "platform/msx_isr.h"
#include "game/ui.h"

void main()
{
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
