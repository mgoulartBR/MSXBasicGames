// Unified input: keyboard, joystick (ports A/B) and MSX mouse. The UI only sees this.
#pragma once
#include "gtypes.h"

enum
{
	IN_UP = 1 << 0, IN_DOWN = 1 << 1, IN_LEFT = 1 << 2, IN_RIGHT = 1 << 3,
	IN_OK = 1 << 4,       // Space / Enter / joystick button A / left mouse button
	IN_BACK = 1 << 5,     // Esc / BS / joystick button B / right mouse button
	IN_PLAY = 1 << 6,     // P
	IN_DISCARD = 1 << 7,  // D
	IN_SORT = 1 << 8,     // S  (toggle rank/suit)
	IN_INFO = 1 << 9,     // I  (run info)
	IN_NEXT = 1 << 10,    // N  (next / skip)
	IN_USE = 1 << 11,     // U  (use consumable / sell)
};

typedef struct
{
	u16  pressed;         // edge: went down this frame (with key repeat for directions)
	u16  held;
	u8   mx, my;          // pointer
	bool mouse;           // a mouse is plugged
	bool moved;           // pointer moved this frame
	bool click;           // left button went down
	bool rclick;          // right button went down
} InputState;
extern InputState in;

void Input_Init(void);
void Input_Update(void);
