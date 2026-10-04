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

volatile u8 g_MouseOn;
volatile u8 g_MouseX = 128;
volatile u8 g_MouseY = 96;
volatile u8 g_MouseBtn;
u8 g_MouseProbe = 1;
static u8 s_MousePort;   // 0 = none, 1 / 2 = port with a mouse
static u8 s_ProbeCnt[2];

#ifndef DBG_MOUSE_TEST
// Poll one port with the MSX mouse protocol (timing-critical, done inside the ISR)
static void mouse_poll(u8 port)
{
	Mouse_State ms;
	ms.Buttons = 0xFF;
	Mouse_Read(port == 1 ? MOUSE_PORT_1 : MOUSE_PORT_2, &ms);
	i16 dx = Mouse_GetOffsetX(&ms);
	i16 dy = Mouse_GetOffsetY(&ms);
	// a device that is not a mouse (joystick idle / nothing) reads 0xFF,0xFF => ignore
	if ((u8)ms.dX == 0xFF && (u8)ms.dY == 0xFF)
		dx = dy = 0;
	// light acceleration for fast moves
	if (dx > 6 || dx < -6) dx += dx / 2;
	if (dy > 6 || dy < -6) dy += dy / 2;
	i16 x = (i16)g_MouseX + dx;
	i16 y = (i16)g_MouseY + dy;
	if (x < 0) x = 0;
	if (x > 255) x = 255;
	if (y < 0) y = 0;
	if (y > 191) y = 191;
	g_MouseX = (u8)x;
	g_MouseY = (u8)y;
	u8 b = 0;
	if (Mouse_IsButtonPress(&ms, MOUSE_BOUTON_LEFT)) b |= MB_LEFT;
	if (Mouse_IsButtonPress(&ms, MOUSE_BOUTON_RIGHT)) b |= MB_RIGHT;
	if (dx || dy || b)
		g_MouseOn = 1;
	g_MouseBtn = b;
}

// Title-screen probe: a mouse at rest reads (0,0); joystick idle / empty port read (0xFF,0xFF)
static void mouse_probe(u8 port)
{
	Mouse_State ms;
	ms.Buttons = 0xFF;
	Mouse_Read(port == 1 ? MOUSE_PORT_1 : MOUSE_PORT_2, &ms);
	u8 i = port - 1;
	if (ms.dX == 0 && ms.dY == 0)
	{
		if (++s_ProbeCnt[i] >= 5)
			s_MousePort = port;
	}
	else
		s_ProbeCnt[i] = 0;
}
#endif

// Called from the VBlank hook (50/60 Hz)
void Input_Sample(void)
{
	u8 h = 0;
#ifndef DBG_MOUSE_TEST
	if (g_MouseProbe && !s_MousePort)
	{
		mouse_probe(1);
		mouse_probe(2);
	}
	if (s_MousePort != 1)
		h |= ~Joystick_Read(JOY_PORT_1) & 0x3F; // bits 0-3 dirs, 4 trigger A, 5 trigger B (same layout as IN_*)
	if (s_MousePort != 2)
		h |= ~Joystick_Read(JOY_PORT_2) & 0x3F;
	if (s_MousePort)
		mouse_poll(s_MousePort);
#endif
	u8 kb = 0;
	if (Keyboard_IsKeyPressed(KEY_UP)) kb |= IN_UP;
	if (Keyboard_IsKeyPressed(KEY_DOWN)) kb |= IN_DOWN;
	if (Keyboard_IsKeyPressed(KEY_LEFT)) kb |= IN_LEFT;
	if (Keyboard_IsKeyPressed(KEY_RIGHT)) kb |= IN_RIGHT;
	if (Keyboard_IsKeyPressed(KEY_SPACE) || Keyboard_IsKeyPressed(KEY_RETURN)) kb |= IN_A;
	if (Keyboard_IsKeyPressed(KEY_ESC) || Keyboard_IsKeyPressed(KEY_TAB)) kb |= IN_B;
	h |= kb;
	if (h & 0x3F)
		g_MouseOn = 0; // keyboard / joystick takes over: pointer hidden, focus model
	if (g_MouseOn)
	{
		if (g_MouseBtn & MB_LEFT) h |= IN_A;
		if (g_MouseBtn & MB_RIGHT) h |= IN_B;
	}
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
