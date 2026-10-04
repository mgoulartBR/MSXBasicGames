// Platform layer: keyboard + joystick (both ports) merged into one logical pad.
// Sampling runs in the VBlank hook (Input_Sample) so short key presses are never lost while the
// main loop is busy redrawing; the main loop consumes the latched edges with Input_Update().
#pragma once
#include "msxgl.h"

#define IN_UP    0x01
#define IN_DOWN  0x02
#define IN_LEFT  0x04
#define IN_RIGHT 0x08
#define IN_A     0x10 // confirm: SPACE / RETURN / joystick trigger 1
#define IN_B     0x20 // cancel / switch: ESC / TAB / joystick trigger 2
#define IN_MUTE  0x40 // M key: toggle sound

extern u8 g_Held; // buttons currently down (as of the last Input_Update)
extern u8 g_Push; // presses since the last Input_Update (directions auto-repeat)

void Input_Init(void);
void Input_Sample(void); // ISR side
void Input_Update(void); // main side
