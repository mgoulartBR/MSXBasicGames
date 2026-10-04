#include "msx_input.h"

#define REPEAT_DELAY 14
#define REPEAT_RATE  4

u8 g_Held;
u8 g_Push;
static volatile u8 s_Held;  // written by the ISR
static volatile u8 s_Latch; // presses accumulated by the ISR
static u8 s_Prev;
static u8 s_Timer[4];

void Input_Init(void)
{
	g_Held = g_Push = s_Held = s_Latch = s_Prev = 0;
}

// Called from the VBlank hook (50/60 Hz)
void Input_Sample(void)
{
	u8 j1 = ~Joystick_Read(JOY_PORT_1);
	u8 j2 = ~Joystick_Read(JOY_PORT_2);
	u8 h = (j1 | j2) & 0x3F; // bits 0-3 dirs, 4 trigger A, 5 trigger B (same layout as IN_*)
	if (Keyboard_IsKeyPressed(KEY_UP)) h |= IN_UP;
	if (Keyboard_IsKeyPressed(KEY_DOWN)) h |= IN_DOWN;
	if (Keyboard_IsKeyPressed(KEY_LEFT)) h |= IN_LEFT;
	if (Keyboard_IsKeyPressed(KEY_RIGHT)) h |= IN_RIGHT;
	if (Keyboard_IsKeyPressed(KEY_SPACE) || Keyboard_IsKeyPressed(KEY_RETURN)) h |= IN_A;
	if (Keyboard_IsKeyPressed(KEY_ESC) || Keyboard_IsKeyPressed(KEY_TAB)) h |= IN_B;
	if (Keyboard_IsKeyPressed(KEY_M)) h |= IN_MUTE;

	u8 push = h & ~s_Prev;
	for (u8 i = 0; i < 4; ++i)
	{
		u8 m = 1 << i;
		if (!(h & m))
		{
			s_Timer[i] = 0;
			continue;
		}
		if (push & m)
		{
			s_Timer[i] = REPEAT_DELAY;
			continue;
		}
		if (--s_Timer[i] == 0)
		{
			s_Timer[i] = REPEAT_RATE;
			push |= m;
		}
	}
	s_Prev = h;
	s_Held = h;
	s_Latch |= push;
}

void Input_Update(void)
{
	__asm__("di");
	g_Push = s_Latch;
	s_Latch = 0;
	g_Held = s_Held;
	__asm__("ei");
}
