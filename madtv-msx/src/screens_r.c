// Tela de Audiencias / Image (movida de screens_a.c na 2.8 porque o segmento 5 estourou 8 KB). Segmento 38 (banco 2).
#include "app.h"

// ---------------------------------------------------------------- AUDIENCIAS / IMAGE
static void RatingsDynLocal(void)
{
	u8 s, st, y;
	Ui_Begin();
	char b[8];
	for (s = 0; s < NUM_SLOTS; s++)
	{
		y = CONTENT_Y + 24 + s * 10;
		Ui_Fill(56, y - 1, 190, 10, UI_BG);
		ClockStr(b, FIRST_SLOT_T + s * 60);
		Ui_Color(UI_GRAY); Ui_Text(8, y, b);
		for (st = 0; st < NUM_STATIONS; st++)
		{
			if (!g_Game.aud_done[s]) { Ui_Color(UI_GRAY); Ui_Text(64 + st * 54, y, "-"); continue; }
			Ui_Color(g_StationCol[st]); Ui_Dec1(60 + st * 54, y, g_Game.aud[st][s]);
			Ui_Color(UI_GRAY); Ui_Int(84 + st * 54, y, Sim_Quota(st, s));
		}
	}
	Ui_End(56, (u8)(CONTENT_Y + 23), 190, 70);       // tabela (linhas sao preenchidas por inteiro antes de escrever)
	Ui_Begin();
	Ui_Fill(50, (u8)(CONTENT_Y + 111), 200, 32, UI_BG);
	for (st = 0; st < NUM_STATIONS; st++)
	{
		y = (u8)(CONTENT_Y + 112 + st * 10);
		Ui_Bar(50, y + 1, 160, 6, g_Game.image[st], g_StationCol[st] == UI_RED ? COLOR_MEDIUM_RED : g_StationCol[st] == UI_GREEN ? COLOR_LIGHT_GREEN : COLOR_CYAN);
		Ui_Fill(214, y - 1, 30, 9, UI_BG);
		Ui_Color(g_StationCol[st]); Ui_Int(216, y, g_Game.image[st]);
	}
	Ui_End(50, (u8)(CONTENT_Y + 111), 200, 32);
}

static void RatingsDynLocal(void);
static void Draw_Ratings_Body(void)
{
	u8 st;
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Ratings (million viewers)");
	for (st = 0; st < NUM_STATIONS; st++) { Ui_Color(g_StationCol[st]); Ui_Text(60 + st * 54, CONTENT_Y + 12, g_StationName[st]); }
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 100), "Image");
	for (st = 0; st < NUM_STATIONS; st++) { Ui_Color(g_StationCol[st]); Ui_Text(4, CONTENT_Y + 112 + st * 10, g_StationName[st]); }
	RatingsDynLocal();
	Hint("BACK:office");
}

void Ratings_Draw(void) __banked
{
	Ui_Begin();
	Draw_Ratings_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}



void Ratings_Dyn(void) __banked { RatingsDynLocal(); }
void Ratings_Input(u8 ev) __banked { if (ev & IN_BACK) Goto(SCR_OFFICE); }
