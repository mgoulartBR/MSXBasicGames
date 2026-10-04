#include "ui.h"

#define MAX_LINES 56
static TextLine s_Lines[MAX_LINES];

void Ui_Frame(void)
{
	Halt(); // wait for the next VBlank (the hook has sampled input / run audio)
	Input_Update();
	Pointer_Update(g_MouseOn, g_MouseX, g_MouseY);
	if (g_Push & IN_MUTE)
	{
		Audio_Mute(!Audio_IsMuted());
	}
}

void Ui_WaitRelease(void)
{
	do
		Ui_Frame();
	while (g_Held & (IN_A | IN_B));
}

static void prompt(u8 row, u8 color, const char* a, const char* b)
{
	char buf[48];
	char* p = buf;
	const char* s = g_MouseOn ? "[CLICK]  " : "[SPACE]  ";
	while (*s) *p++ = *s++;
	while (*a) *p++ = *a++;
	if (b)
		while (*b) *p++ = *b++;
	*p = 0;
	Gfx_TextCenter(1, row, 30, buf, color);
}

void Ui_ShowPages(const char* msg, u8 col, u8 row0, u8 rows, u8 wtiles, u8 color,
                  const char* finalLabel, u8 promptRow, u8 promptColor)
{
	u8 n = Text_Wrap(msg, (u8)(wtiles << 3) - 2, s_Lines, MAX_LINES);
	// drop trailing blank lines
	while (n > 1 && s_Lines[n - 1].len == 0)
		--n;
	u8 pages = (n + rows - 1) / rows;
	for (u8 page = 0; page < pages; ++page)
	{
		Gfx_FillPattern(col, row0, wtiles, rows, 0, 0);
		Gfx_FillColor(col, row0, wtiles, rows, color);
		for (u8 i = 0; i < rows; ++i)
		{
			u8 li = page * rows + i;
			if (li >= n)
				break;
			if (s_Lines[li].len)
				Gfx_TextLine(col, row0 + i, wtiles, 1, msg + s_Lines[li].start, s_Lines[li].len, color);
		}
		const char* label = (page + 1 < pages) ? "More..." : finalLabel;
		u8 lastMouse = g_MouseOn;
		prompt(promptRow, promptColor, label, 0);
		Ui_WaitRelease();
		for (;;)
		{
			Ui_Frame();
			if (g_MouseOn != lastMouse) // input device changed: SPACE <-> CLICK
			{
				lastMouse = g_MouseOn;
				prompt(promptRow, promptColor, label, 0);
			}
			if (g_Push & IN_A)
				break;
		}
		Sfx_Play(SFX_CLICK);
	}
}

void Ui_Credits(u8 row, u8 color)
{
	Gfx_Text(0, row, 17, CREDIT_PORT, color);
	Gfx_TextRight(17, row, 15, CREDIT_ORIG, color);
}
