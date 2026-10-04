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

// Mouse (MSX mouse on joystick port 1 or 2). The pointer is shown/used only after the mouse moved;
// any keyboard / joystick press switches back to the focus (pad) model.
#define MB_LEFT  0x01
#define MB_RIGHT 0x02
extern volatile u8 g_MouseOn;  // 1 = mouse mode active
extern volatile u8 g_MouseX;   // pointer position in pixels (0..255)
extern volatile u8 g_MouseY;   // 0..191
extern volatile u8 g_MouseBtn; // MB_* bits currently down
extern u8 g_MouseProbe;        // TRUE on the title screen: look for a mouse (device at rest reads 0,0)

void Input_Init(void);
void Input_Sample(void); // ISR side
void Input_Update(void); // main side
