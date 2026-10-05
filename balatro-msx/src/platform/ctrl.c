#include "msxgl.h"
#include "ctrl.h"

InputState in;
static u16 s_prev;
static u8  s_mousePort;           // INPUT_PORT1 / INPUT_PORT2 when a mouse was found, 0 otherwise
static Mouse_State s_ms;
static u8  s_rep[4];              // key repeat counters for directions
static u8  s_detect;

static u8 detect_mouse(void)
{
	if (Input_Detect(INPUT_PORT1) == INPUT_TYPE_MOUSE) return INPUT_PORT1;
	if (Input_Detect(INPUT_PORT2) == INPUT_TYPE_MOUSE) return INPUT_PORT2;
	return 0;
}

void Input_Init(void)
{
	in.mx = 128; in.my = 106;
	s_mousePort = detect_mouse();
	in.mouse = s_mousePort != 0;
	if (in.mouse) Mouse_Read(s_mousePort, &s_ms);          // discard first (accumulated) motion
}

static u16 read_all(void)
{
	u16 k = 0;
	u8 r8 = Keyboard_Read(8), r7 = Keyboard_Read(7), r4 = Keyboard_Read(4), r3 = Keyboard_Read(3), r5 = Keyboard_Read(5);
	if (IS_KEY_PRESSED(r8, KEY_UP)) k |= IN_UP;
	if (IS_KEY_PRESSED(r8, KEY_DOWN)) k |= IN_DOWN;
	if (IS_KEY_PRESSED(r8, KEY_LEFT)) k |= IN_LEFT;
	if (IS_KEY_PRESSED(r8, KEY_RIGHT)) k |= IN_RIGHT;
	if (IS_KEY_PRESSED(r8, KEY_SPACE) || IS_KEY_PRESSED(r7, KEY_RETURN)) k |= IN_OK;
	if (IS_KEY_PRESSED(r7, KEY_ESC) || IS_KEY_PRESSED(r8, KEY_DEL) || IS_KEY_PRESSED(r7, KEY_BS)) k |= IN_BACK;
	if (IS_KEY_PRESSED(r4, KEY_P)) k |= IN_PLAY;
	if (IS_KEY_PRESSED(r3, KEY_D)) k |= IN_DISCARD;
	if (IS_KEY_PRESSED(r5, KEY_S)) k |= IN_SORT;
	if (IS_KEY_PRESSED(Keyboard_Read(4), KEY_N)) k |= IN_NEXT;
	if (IS_KEY_PRESSED(Keyboard_Read(3), KEY_I)) k |= IN_INFO;
	if (IS_KEY_PRESSED(Keyboard_Read(5), KEY_U)) k |= IN_USE;
	for (u8 p = 0; p < 2; p++)                                // joystick ports A and B (the mouse port is skipped)
	{
		if (s_mousePort == (p ? INPUT_PORT2 : INPUT_PORT1)) continue;
		u8 j = Joystick_Read(p ? JOY_PORT_2 : JOY_PORT_1);
		if (!(j & JOY_INPUT_DIR_UP)) k |= IN_UP;
		if (!(j & JOY_INPUT_DIR_DOWN)) k |= IN_DOWN;
		if (!(j & JOY_INPUT_DIR_LEFT)) k |= IN_LEFT;
		if (!(j & JOY_INPUT_DIR_RIGHT)) k |= IN_RIGHT;
		if (JOY_GET_TRIG1(j)) k |= IN_OK;
		if (JOY_GET_TRIG2(j)) k |= IN_BACK;
	}
	return k;
}

void Input_Update(void)
{
	u16 k = read_all();
	u16 pressed = k & ~s_prev;
	// key repeat for the directions
	for (u8 d = 0; d < 4; d++)
	{
		u16 bit = (u16)(1 << d);
		if (k & bit)
		{
			if (pressed & bit) s_rep[d] = 14;
			else if (--s_rep[d] == 0) { pressed |= bit; s_rep[d] = 4; }
		}
	}
	in.moved = FALSE; in.click = FALSE; in.rclick = FALSE;
	if (in.mouse)
	{
		Mouse_Read(s_mousePort, &s_ms);
		i8 dx = Mouse_GetOffsetX(&s_ms), dy = Mouse_GetOffsetY(&s_ms);
		if (dx || dy)
		{
			i16 x = (i16)in.mx + dx, y = (i16)in.my + dy;
			if (x < 1) x = 1; if (x > 254) x = 254;
			if (y < 1) y = 1; if (y > 210) y = 210;
			in.mx = (u8)x; in.my = (u8)y; in.moved = TRUE;
		}
		bool l = Mouse_IsButtonPress(&s_ms, MOUSE_BOUTON_LEFT), r = Mouse_IsButtonPress(&s_ms, MOUSE_BOUTON_RIGHT);
		static bool pl, pr;
		in.click = l && !pl; in.rclick = r && !pr; pl = l; pr = r;
		if (in.click) pressed |= IN_OK;
		if (in.rclick) pressed |= IN_BACK;
		if (l) k |= IN_OK;
	}
	else if ((++s_detect & 63) == 0)                           // hot-plug: look again once in a while
	{
		s_mousePort = detect_mouse();
		in.mouse = s_mousePort != 0;
		if (in.mouse) Mouse_Read(s_mousePort, &s_ms);
	}
	in.pressed = pressed;
	in.held = k;
	s_prev = k;
}
