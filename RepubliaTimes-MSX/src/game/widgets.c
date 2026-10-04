#include "widgets.h"
#include "logic.h"
#include "../platform/msx_gfx.h"
#include "../data/gfx_data.h"
#include "../platform/msx_seg.h"

static const i8 s_Sin64[64] = {
	0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126,
	127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
	0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126,
	-127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12 };

// angle index: 0..63 clockwise on screen starting at 3 o'clock (y grows downwards)
static i8 sin64(u8 a) { return s_Sin64[a & 63]; }
static i8 cos64(u8 a) { return s_Sin64[(a + 16) & 63]; }
static i8 scale(i8 v, u8 r) { return (i8)(((i16)v * r) / 127); }

#define CLOCK_CX 20
#define CLOCK_CY 16
#define CLOCK_R  15

static u8 s_FaceBuf[5 * 4 * 8];
static u8 s_WorkBuf[5 * 4 * 8];
static Canvas s_Face = { s_FaceBuf, 5, 4 };
static Canvas s_Work = { s_WorkBuf, 5, 4 };
static u8 s_LastKey = 0xFF;
static u8 s_LastBlink = 0xFF;

void Clock_Init(void)
{
	Cv_Clear(&s_Face, 0);
	for (u8 a = 0; a < 64; ++a)
	{
		Cv_Pixel(&s_Face, CLOCK_CX + scale(cos64(a), CLOCK_R), CLOCK_CY + scale(sin64(a), CLOCK_R));
		Cv_Pixel(&s_Face, CLOCK_CX + scale(cos64(a), CLOCK_R - 1), CLOCK_CY + scale(sin64(a), CLOCK_R - 1));
	}
	for (u8 h = 0; h < 12; ++h)
	{
		u8 a = h * 64 / 12;
		a = (u8)((h * 21 + (h + 1) / 2) >> 2); // ~ h*5.333
		Cv_Line(&s_Face, CLOCK_CX + scale(cos64(a), CLOCK_R - 4), CLOCK_CY + scale(sin64(a), CLOCK_R - 4),
		        CLOCK_CX + scale(cos64(a), CLOCK_R - 1), CLOCK_CY + scale(sin64(a), CLOCK_R - 1));
	}
	Cv_FillRect(&s_Face, CLOCK_CX - 1, CLOCK_CY - 1, 2, 2);
	s_LastKey = 0xFF;
	s_LastBlink = 0xFF;
}

static void thick_line(Canvas* cv, i8 x0, i8 y0, i8 x1, i8 y1)
{
	Cv_Line(cv, x0, y0, x1, y1);
	Cv_Line(cv, x0 + 1, y0, x1 + 1, y1);
}

void Clock_Draw(u8 col, u8 row, u16 time, bool alarmBlink)
{
	// hour hand: starts pointing down (6 o'clock) and makes one turn per day
	u8 hourA = (u8)(16 + (u8)(((u32)time * 64) / DAY_DURATION));
	// minute hand: starts up, 60 turns per day (one turn per second of play)
	u8 minA = (u8)(-16 + (u8)(((u32)time * 32) / 300));
	u8 key = hourA ^ (u8)(minA << 2) ^ (u8)(minA >> 3);
	u8 blink = alarmBlink ? (u8)(((u32)time / 120) & 1) : 0;
	if (key == s_LastKey && blink == s_LastBlink)
		return;
	s_LastKey = key;
	Cv_Copy(&s_Work, s_FaceBuf);
	thick_line(&s_Work, CLOCK_CX, CLOCK_CY, CLOCK_CX + scale(cos64(hourA), 11), CLOCK_CY + scale(sin64(hourA), 11));
	Cv_Line(&s_Work, CLOCK_CX, CLOCK_CY, CLOCK_CX + scale(cos64(minA), CLOCK_R - 2), CLOCK_CY + scale(sin64(minA), CLOCK_R - 2));
	Gfx_BlitCanvas(col, row, &s_Work);
	if (blink != s_LastBlink)
	{
		s_LastBlink = blink;
		Gfx_FillColor(col, row, 5, 4, blink ? CT_RED : CT_INK);
	}
}

static u8 s_MeterBuf[5 * 5 * 8];
static Canvas s_Meter = { s_MeterBuf, 5, 5 };

static char* itoa16(char* p, i16 v)
{
	char t[7];
	u8 n = 0;
	if (v < 0)
	{
		*p++ = '-';
		v = -v;
	}
	do
	{
		t[n++] = '0' + (v % 10);
		v /= 10;
	} while (v);
	while (n)
		*p++ = t[--n];
	*p = 0;
	return p;
}

void Meter_Draw(u8 col, u8 row, i8 value)
{
	char buf[8];
	u16 oldSeg = Seg_Set(SEG_GFX);
	Cv_Copy(&s_Meter, g_Gfx_meter);
	Seg_Restore(oldSeg);
	// needle: value -30..+30 sweeps from 9 o'clock over 12 to 3 o'clock
	i8 idx = -16 + (i8)(((i16)value * 8 + (value >= 0 ? 7 : -7)) / 15);
	u8 a = (u8)idx;
	i8 nx = scale(cos64(a), 12);
	i8 ny = scale(sin64(a), 12);
	thick_line(&s_Meter, 20, 18, 20 + nx, 18 + ny);
	itoa16(buf, value);
	u8 len = Text_Len(buf);
	u8 tw = Text_Width(buf, len);
	Cv_TextKnock(&s_Meter, (40 - tw) >> 1, 3, buf, len);
	Cv_TextKnock(&s_Meter, (40 - Text_Width("Loyalty", 7)) >> 1, 4, "Loyalty", 7);
	Gfx_BlitCanvas(col, row, &s_Meter);
	Gfx_FillColor(col, row, 5, 5, CT_INK);
}

void Readers_Draw(u8 col, u8 row, i16 count, i16 delta, bool showDelta, bool onWhite)
{
	char buf[20];
	char* p;
	u8 color = onWhite ? CT_INK : CT_INV;
	Gfx_TextCenter(col, row, 5, "Readers", color);
	p = itoa16(buf, count);
	if (showDelta && delta != 0)
	{
		*p++ = ' ';
		*p++ = '(';
		if (delta > 0)
			*p++ = '+';
		p = itoa16(p, delta);
		*p++ = ')';
		*p = 0;
	}
	Gfx_TextCenter(col, row + 1, 5, buf, color);
}
