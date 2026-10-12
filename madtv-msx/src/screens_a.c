// Telas: Grade de programacao, Agencia de filmes, Agencia de publicidade, Audiencias. Compilado no segmento 5 (banco 2).
#include "app.h"

// ---------------------------------------------------------------- GRID
static u8 s_GridCol;        // 0 = programa, 1 = anuncio
static u8 s_Slot;           // slot sob o cursor
static u8 s_Pick;           // 0 = navegando a grade, 1 = escolhendo filme, 2 = escolhendo contrato, 3 = resumo da semana (so leitura)
static u8 s_View;           // 1 = o menu de contratos so mostra (icone da grade): OK/Esc fecham
static u8 s_PickList[NUM_ALL + 1];
static u8 s_PickN;

static void BuildPick(void)
{
	u8 i;
	s_PickN = 0;
	if (!s_View) s_PickList[s_PickN++] = NONE;
	if (s_Pick == 1) { for (i = 0; i < NUM_ALL; i++) if (g_Game.owned[i]) s_PickList[s_PickN++] = i; }
	else             { for (i = 0; i < MAX_CONTRACTS; i++) if (g_Game.contract[i].ad != NONE) s_PickList[s_PickN++] = i; }
}

#if ART_SCENES
#define GR_Y(s)  ((u8)(30 + (s) * 16))                    // linhas da grade (7 x 16 px) a partir do topo do conteudo
#define GR_TX    31                                       // coluna da hora
#define GR_TW    35
#define GR_PX    68                                       // coluna do programa
#define GR_PW    100
#define GR_AX    170                                      // coluna do anuncio
#define GR_AW    62
#define PK_ROWS  7                                        // escolha em menu suspenso sobre a grade (como no original)
#define PK_X     38
#define PK_W     180
#define PK_Y(i)  ((u8)(60 + (i) * ROW_H))
#else
#define PK_ROWS  LIST_ROWS
#endif

static void DrawRow_Grid_Body(u8 s)
{
	char b[8];
	const Slot* sl;
	u8 y;
	if (s >= NUM_SLOTS) return;
	sl = &g_Game.slot[0][s];
	y = CONTENT_Y + 14 + s * 14;
#if ART_SCENES
	y = GR_Y(s);                                          // grade no estilo do original: hora | programa (2 linhas) | anuncio (2 linhas); rosa = ja comecou
	{
		u8 aired = (g_Game.t >= FIRST_SLOT_T + (u16)s * 60 + 5), fp, fa;
		u8 mb = sl->movie, cont = (u8)(mb != NONE && sl->part);
		fp = (s == s_Slot && s_GridCol == 0) ? 5 : (aired ? 9 : 14);
		fa = (s == s_Slot && s_GridCol == 1) ? 5 : (aired ? 9 : 14);
		Ui_Fill(GR_TX, y, GR_AX + GR_AW - GR_TX, 16, 1);
		Chrome_Box(GR_TX, y, GR_TW, 15, aired ? 9 : 14, 1);
		Chrome_Box(GR_PX, cont ? (u8)(y - 1) : y, GR_PW, cont ? 16 : 15, fp, 1);
		Chrome_Box(GR_AX, y, GR_AW, 15, fa, 1);
		Ui_Color(UI_BLACK);
		ClockStr(b, FIRST_SLOT_T + s * 60);
		Ui_Text(GR_TX + 3, (u8)(y + 4), b);
		if (mb != NONE)
		{
			char t[8];
			if (!cont) Ui_TextN(GR_PX + 3, (u8)(y + 1), (*Mov(mb)).title, 15);
			t[0] = 'B'; t[1] = 'L'; t[2] = 'K'; t[3] = ' '; t[4] = (char)('1' + sl->part); t[5] = '/'; t[6] = (char)('0' + (*Mov(mb)).blocks); t[7] = 0;   // "BLK 1/2" em micro-fonte
			Chrome_Micro(GR_PX + 3, (u8)(cont ? y + 5 : y + 9), t);
		}
		if (sl->ad != NONE)
		{
			const Contract* ct = &g_Game.contract[sl->ad];
			char t[8];
			Ui_TextN(GR_AX + 3, (u8)(y + 1), g_Ads[ct->ad].title, 9);
			t[0] = (char)('0' + ct->reps_left % 10); t[1] = 'X'; t[2] = ' '; t[3] = (char)('0' + ct->days_left % 10); t[4] = 'D'; t[5] = 0;                      // "3X 3D"
			Chrome_Micro(GR_AX + 3, (u8)(y + 9), t);
		}
	}
	return;
#endif
	Ui_Fill(0, y - 2, 255, 13, UI_BG);
	ClockStr(b, FIRST_SLOT_T + s * 60);
	Ui_Color(s == s_Slot ? UI_YELLOW : UI_GRAY);
	Ui_Text(4, y, s == s_Slot ? ">" : " "); Ui_Text(12, y, b);
	if (sl->movie == NONE) { Ui_Color(UI_GRAY); Ui_Text(48, y, "- empty -"); }
	else if (sl->part) { Ui_Color(UI_GRAY); Ui_Text(48, y, "  (cont.)"); }
	else { Ui_Color((s == s_Slot && s_GridCol == 0) ? UI_YELLOW : UI_WHITE); Ui_TextN(48, y, (*Mov(sl->movie)).title, 17); }
	if (sl->ad == NONE) { Ui_Color(UI_GRAY); Ui_Text(156, y, "no ad"); }
	else { Ui_Color((s == s_Slot && s_GridCol == 1) ? UI_YELLOW : UI_CYAN); Ui_TextN(156, y, g_Ads[g_Game.contract[sl->ad].ad].title, 16); }
}

#if ART_SCENES
static void GridDetail_Body(void)                          // painel de baixo: o que esta no slot selecionado
{
	const Slot* sl = &g_Game.slot[0][s_Slot];
	char b[8];
	Chrome_Box(0, 154, 256, 32, 14, 1);
	Ui_Color(UI_BLACK);
	ClockStr(b, FIRST_SLOT_T + s_Slot * 60);
	Ui_Text(8, 158, b);
	if (sl->movie == NONE) Ui_Text(48, 158, "(empty slot)");
	else
	{
		Ui_TextN(48, 158, (*Mov(sl->movie)).title, 33);
		Ui_Text(8, 170, "Q"); Ui_Int(18, 170, Sim_Quality(sl->movie));
		Ui_Text(44, 170, "block"); { u8 x = Ui_Int(80, 170, sl->part + 1); Ui_Text(x, 170, "/"); Ui_Int((u8)(x + 6), 170, (*Mov(sl->movie)).blocks); }
	}
	if (sl->ad == NONE) Ui_Text(132, 170, "no ad contract");
	else { const Ad* ad = &g_Ads[g_Game.contract[sl->ad].ad]; Ui_Text(132, 170, "min"); Ui_Dec1(156, 170, ad->min_audience); Ui_Text(184, 170, "M"); Money(200, 170, ad->profit); }
}
#endif

static void DrawRow_Grid(u8 s)
{
	if (s >= NUM_SLOTS) return;
	Ui_Begin();
	DrawRow_Grid_Body(s);
#if ART_SCENES
	Ui_End(GR_TX, (u8)(GR_Y(s) - 1), GR_AX + GR_AW - GR_TX, 17);
	if (s == s_Slot) { Ui_Begin(); GridDetail_Body(); Ui_End(0, 154, 256, 32); }
#else
	Ui_End(0, (u8)(CONTENT_Y + 14 + s * 14 - 2), 255, 13);
#endif
}

static void Draw_GridRows(void)
{
	u8 s;
	for (s = 0; s < NUM_SLOTS; s++) DrawRow_Grid(s);
}

static void DrawRow_Pick_Body(u8 i)
{
	u8 v, y;
	if (i >= PK_ROWS) return;
	y = CONTENT_Y + 14 + i * ROW_H;
#if ART_SCENES
	y = PK_Y(i);
	Ui_Fill(PK_X + 4, y - 1, PK_W - 8, ROW_H, 14);
	if (g_First + i >= s_PickN) return;
	if (g_First + i == g_Sel) Chrome_Box(PK_X + 4, y - 1, PK_W - 8, ROW_H, 5, 0);
	Ui_Color(UI_BLACK);
	v = s_PickList[g_First + i];
	if (v == NONE) { Ui_Text(PK_X + 8, y, "(none)"); return; }
	if (s_Pick == 1)
	{
		Ui_TextN(PK_X + 8, y, (*Mov(v)).title, 17);
		{ u8 x = Ui_Int(PK_X + 112, y, (*Mov(v)).blocks); Ui_Text(x, y, "bl"); }
		Ui_Text(PK_X + 134, y, "Q"); Ui_Int(PK_X + 140, y, Sim_Quality(v));
		Ui_Text(PK_X + 158, y, "x"); Ui_Int(PK_X + 164, y, g_Game.plays[0][v]);
	}
	else
	{
		const Contract* c = &g_Game.contract[v];
		u8 x;
		Ui_TextN(PK_X + 8, y, g_Ads[c->ad].title, 10);
		Ui_Text(PK_X + 72, y, "min"); Ui_Dec1(PK_X + 94, y, g_Ads[c->ad].min_audience);
		x = Ui_Int(PK_X + 122, y, c->reps_left); Ui_Text(x, y, "x");
		x = Ui_Int(PK_X + 148, y, c->days_left); Ui_Text(x, y, "d");
	}
	return;
#endif
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (g_First + i >= s_PickN) return;
	v = s_PickList[g_First + i];
	Ui_Color(g_First + i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, g_First + i == g_Sel ? ">" : " ");
	if (v == NONE) { Ui_Text(14, y, "(none)"); return; }
	if (s_Pick == 1)
	{
		Ui_TextN(14, y, (*Mov(v)).title, 22);
		Ui_Color(UI_GRAY);
		Ui_Int(170, y, (*Mov(v)).blocks); Ui_Text(178, y, "bl");
		Ui_Text(196, y, "Q"); Ui_Int(204, y, Sim_Quality(v));
		Ui_Text(226, y, "x"); Ui_Int(234, y, g_Game.plays[0][v]);
	}
	else
	{
		const Contract* c = &g_Game.contract[v];
		Ui_TextN(14, y, g_Ads[c->ad].title, 18);
		Ui_Color(UI_GRAY);
		Ui_Text(128, y, "min "); Ui_Dec1(152, y, g_Ads[c->ad].min_audience);
		Ui_Int(184, y, c->reps_left); Ui_Text(196, y, "x");
		Ui_Int(208, y, c->days_left); Ui_Text(220, y, "d");
	}
}

static void DrawRow_Pick(u8 i)
{
	if (i >= PK_ROWS || s_Pick == 3) return;
	Ui_Begin();
	DrawRow_Pick_Body(i);
#if ART_SCENES
	Ui_End(PK_X + 4, (u8)(PK_Y(i) - 1), PK_W - 8, ROW_H);
#else
	Ui_End(0, (u8)(CONTENT_Y + 14 + i * ROW_H - 1), 255, ROW_H);
#endif
}

static void DrawList_Pick(void)
{
	u8 i;
	if (s_Pick == 3) return;
	for (i = 0; i < PK_ROWS; i++) DrawRow_Pick(i);
}

static void Draw_Grid_Body(void)
{
#if ART_SCENES
	Ui_Fill(0, CONTENT_Y - 2, 0, 160, UI_BG);
	Chrome_Box(0, CONTENT_Y - 2, 256, 160, 14, 1);
	Scene_Part(SCN_GRIDCOL, 0, 14, 2, 30);                // setas e lixeira (esquerda) e os 6 icones (direita), recortados do original
	Scene_Part(SCN_GRIDCOL, 14, 10, 234, 30);
	Draw_GridRows();
	GridDetail_Body();
	if (s_Pick == 0) { Hint("L/R:column OK:edit  icons: ratings, erase, save"); return; }
	Chrome_Box(PK_X, 44, PK_W, 14 + PK_ROWS * ROW_H + 6, 14, 1);          // menu suspenso por cima da grade
	Ui_Color(UI_BLACK); Ui_Text(PK_X + 8, 49, s_Pick == 1 ? "Choose programme" : s_Pick == 3 ? "Week summary" : s_View ? "Your ad contracts" : "Choose ad contract");
	if (s_Pick == 3)
	{
		u8 x, left = (u8)((SAMMY_DAYS - g_Game.day % SAMMY_DAYS) % SAMMY_DAYS), st, y = 62;
		Ui_Text(PK_X + 8, y, "Income"); Money(PK_X + 56, y, g_Game.day_income); Ui_Text(PK_X + 100, y, "Costs"); Money(PK_X + 134, y, g_Game.day_cost);
		y += 10; Ui_Text(PK_X + 8, y, "Cash"); Money(PK_X + 56, y, g_Game.money); Ui_Text(PK_X + 100, y, "Debt"); Money(PK_X + 134, y, g_Game.debt);
		y += 10; Ui_Text(PK_X + 8, y, "Sammy in"); x = Ui_Int(PK_X + 62, y, left); Ui_Text(x, y, left == 1 ? " day" : " days");
		y += 12; Ui_Text(PK_X + 8, y, "Image:");
		for (st = 0; st < NUM_STATIONS; st++) { y += 10; Ui_Text(PK_X + 14, y, g_StationName[st]); Ui_Int(PK_X + 70, y, g_Game.image[st]); }
		Hint("OK/BACK: close");
		return;
	}
	DrawList_Pick();
	Hint(s_View ? "OK/BACK: close" : "OK:place BACK:cancel");
	return;
#endif
	ClearContent();
	if (s_Pick == 0)
	{
		Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Programme grid (today)");
		Draw_GridRows();
		Hint("L/R:column OK:edit BACK:office");
		return;
	}
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, s_Pick == 1 ? "Choose programme" : "Choose ad contract");
	DrawList_Pick();
	Hint("OK:place BACK:cancel");
}

void Grid_Draw(void) __banked
{
	Ui_Begin();
	Draw_Grid_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

void Grid_Input(u8 ev) __banked
{
	if (s_Pick == 0)
	{
		u8 old = s_Slot;
		if ((ev & IN_DOWN) && s_Slot + 1 < NUM_SLOTS) s_Slot++;
		if ((ev & IN_UP) && s_Slot > 0) s_Slot--;
		if (s_Slot != old) MarkRows(old, s_Slot);
		if (ev & (IN_LEFT | IN_RIGHT)) { s_GridCol ^= 1; MarkRows(s_Slot, s_Slot); }
		if (ev & IN_BACK) { Goto(SCR_OFFICE); return; }
		if (ev & IN_OK)
		{
			if (g_Game.t >= FIRST_SLOT_T + (u16)s_Slot * 60 + 5) { Sim_Msg("That hour has already started."); g_Dirty |= D_MSG; return; }
			s_Pick = s_GridCol ? 2 : 1;
			BuildPick(); g_Sel = 0; g_First = 0; g_Dirty |= D_CON;
		}
		return;
	}
	if ((s_View || s_Pick == 3) && (ev & (IN_OK | IN_BACK))) { s_View = 0; s_Pick = 0; g_Dirty |= D_CON; return; }
	NavApply(Nav(ev, s_PickN, PK_ROWS));
	if (ev & IN_BACK) { s_Pick = 0; g_Dirty |= D_CON; return; }
	if (ev & IN_OK)
	{
		u8 v = s_PickList[g_Sel];
		if (s_Pick == 1)
		{
			if (v == NONE) { Sim_ClearSlot(s_Slot); Sim_Msg("Slot cleared."); }
			else if (Sim_PlaceMovie(s_Slot, v)) Sim_Msg("Movie does not fit before midnight.");
			else Sim_Msg("Programme set.");
		}
		else { Sim_PlaceAd(s_Slot, v); Sim_Msg(v == NONE ? "Ad removed." : "Ad contract scheduled."); }
		s_Pick = 0; g_Dirty |= D_CON | D_MSG;
	}
}

// ---------------------------------------------------------------- AGENCIA DE FILMES
static u8 s_Cat;
static u8 s_CatStart[DB_NUM_CATEGORIES + 1];

static void Index_Movies(void)
{
	u8 i, c;
	for (c = 0; c <= DB_NUM_CATEGORIES; c++) s_CatStart[c] = DB_NUM_MOVIES;
	for (i = DB_NUM_MOVIES; i > 0; i--) s_CatStart[g_Movies[i - 1].cat] = i - 1;
	s_CatStart[DB_NUM_CATEGORIES] = DB_NUM_MOVIES;
	for (c = DB_NUM_CATEGORIES; c > 0; c--) if (s_CatStart[c - 1] > s_CatStart[c]) s_CatStart[c - 1] = s_CatStart[c];
}

static u8 CatCount(u8 c) { return (u8)(s_CatStart[c + 1] - s_CatStart[c]); }

#define AGENCY_ROWS 9
#if ART_SCENES
#define AG_RY(i)   ((u8)(CONTENT_Y + 16 + (i) * ROW_H))
#define AG_X       126
#define AG_W       128
#else
#define AG_RY(i)   ((u8)(CONTENT_Y + 12 + (i) * ROW_H))
#define AG_X       0
#define AG_W       255
#endif

static void DrawRow_Agency_Body(u8 i)
{
	u8 idx, y = AG_RY(i);
	if (i >= AGENCY_ROWS) return;
#if ART_SCENES
	Ui_Fill(126, y - 1, 128, ROW_H, 14);
	if (g_First + i >= CatCount(s_Cat)) return;
	idx = s_CatStart[s_Cat] + g_First + i;
	if (g_First + i == g_Sel) Chrome_Box(126, y - 1, 128, ROW_H, 5, 0);
	Ui_Color(UI_BLACK);
	Ui_TextN(130, y, g_Movies[idx].title, 16);
	if (g_Game.owned[idx]) Ui_Text(230, y, "own");
	else if (Sim_TakenBy(idx)) { Ui_Color(UI_RED); Ui_Text(230, y, Sim_TakenBy(idx) == 1 ? "Fun" : "Sun"); }
#else
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (g_First + i >= CatCount(s_Cat)) return;
	idx = s_CatStart[s_Cat] + g_First + i;
	Ui_Color(g_First + i == g_Sel ? UI_YELLOW : (g_Game.owned[idx] ? UI_GREEN : UI_WHITE));
	Ui_Text(4, y, g_First + i == g_Sel ? ">" : " ");
	Ui_TextN(14, y, g_Movies[idx].title, 24);
	if (g_Game.owned[idx]) { Ui_Color(UI_GRAY); Ui_Text(232, y, "own"); }
	else if (Sim_TakenBy(idx)) { Ui_Color(UI_RED); Ui_Text(232, y, Sim_TakenBy(idx) == 1 ? "Fun" : "Sun"); }
#endif
}

static void DrawRow_Agency(u8 i)
{
	if (i >= AGENCY_ROWS) return;
	Ui_Begin();
	DrawRow_Agency_Body(i);
	Ui_End(AG_X, (u8)(AG_RY(i) - 1), AG_W, ROW_H);
}

static void DrawList_Agency(void)
{
	u8 i;
	for (i = 0; i < AGENCY_ROWS; i++) DrawRow_Agency(i);
}

static void DrawTitle_Agency_Body(void)
{
#if ART_SCENES
	Ui_Fill(124, CONTENT_Y, 130, 10, 14);
	Ui_Color(UI_BLACK); Ui_Text(128, CONTENT_Y + 2, "<"); Ui_Text(140, CONTENT_Y + 2, g_CategoryName[s_Cat]); Ui_Text(244, CONTENT_Y + 2, ">");
#else
	Ui_Fill(96, CONTENT_Y - 1, 120, 10, UI_BG);
	Ui_Color(UI_WHITE);  Ui_Text(100, CONTENT_Y, "< "); Ui_Text(112, CONTENT_Y, g_CategoryName[s_Cat]);
	Ui_Text((u8)(112 + 6 * 10 + 6), CONTENT_Y, ">");
#endif
}

static void DrawTitle_Agency(void)
{
	Ui_Begin();
	DrawTitle_Agency_Body();
	#if ART_SCENES
	Ui_End(124, CONTENT_Y, 130, 10);
#else
	Ui_End(96, CONTENT_Y - 1, 120, 10);
#endif
}

static void DrawDetail_Agency_Body(void)
{
	u8 idx = s_CatStart[s_Cat] + g_Sel;
	const Movie* m = &g_Movies[idx];
#if ART_SCENES
	Chrome_Box(0, 138, 256, 48, 14, 1);
	Ui_Color(UI_BLACK);
	Ui_Text(8, 141, "Year"); Ui_Int(38, 141, 1850 + m->year);
	Ui_Text(80, 141, "Blocks"); Ui_Int(122, 141, m->blocks);
	Ui_Text(144, 141, "Price"); Money(180, 141, Sim_MoviePrice(idx));
	Ui_Text(8, 152, "Critics"); Ui_Bar(56, 153, 52, 5, m->critics, COLOR_LIGHT_GREEN);
	Ui_Text(120, 152, "Speed"); Ui_Bar(156, 153, 52, 5, m->speed, COLOR_CYAN);
	Ui_Text(8, 163, "Box off."); Ui_Bar(56, 164, 52, 5, m->outcome, COLOR_LIGHT_YELLOW);
	Ui_Text(120, 163, "Quality"); Ui_Int(168, 163, Sim_Quality(idx));
	Ui_Text(196, 163, "Stars"); Ui_Int(232, 163, Db_Stars(idx));
	Chrome_CastLines(idx);
	return;
#endif
	Ui_Fill(0, 132, 255, 35, UI_BG);
	Ui_Color(UI_WHITE);
	Ui_Text(4, 134, "Year"); Ui_Int(34, 134, 1850 + m->year);
	Ui_Text(76, 134, "Blocks"); Ui_Int(118, 134, m->blocks);
	Ui_Text(140, 134, "Price"); Money(176, 134, Sim_MoviePrice(idx));
	if (m->fsk18) { Ui_Color(UI_RED); Ui_Text(226, 134, "FSK18"); }
	Ui_Color(UI_WHITE);
	Ui_Text(4, 145, "Critics"); Ui_Bar(52, 146, 52, 5, m->critics, COLOR_LIGHT_GREEN);
	Ui_Text(112, 145, "Speed"); Ui_Bar(148, 146, 52, 5, m->speed, COLOR_CYAN);
	Ui_Text(4, 156, "Box off."); Ui_Bar(52, 157, 52, 5, m->outcome, COLOR_LIGHT_YELLOW);
	Ui_Text(112, 156, "Quality"); Ui_Int(160, 156, Sim_Quality(idx));
}

static void DrawDetail_Agency(void)
{
	Ui_Begin();
	DrawDetail_Agency_Body();
#if ART_SCENES
	Ui_End(0, 138, 256, 48);
#else
	Ui_End(0, 132, 255, 35);
#endif
}

static void Draw_Agency_Body(void)
{
#if ART_SCENES
	Room_Frame(SCN_MOVIE);
#else
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Film agency");
#endif
	DrawTitle_Agency();
	DrawList_Agency();
	DrawDetail_Agency();
	Hint("L/R:genre OK:buy BACK:office");
}

void Agency_Draw(void) __banked
{
	Ui_Begin();
	Draw_Agency_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

void Agency_Input(u8 ev) __banked
{
	NavApply(Nav(ev, CatCount(s_Cat), AGENCY_ROWS));
	if (ev & (IN_LEFT | IN_RIGHT))
	{
		s_Cat = (ev & IN_RIGHT) ? (s_Cat + 1) % DB_NUM_CATEGORIES : (s_Cat + DB_NUM_CATEGORIES - 1) % DB_NUM_CATEGORIES;
		g_Sel = g_First = 0;
		DrawTitle_Agency();                      // so o titulo do genero + lista (sem limpar a tela)
		g_Dirty |= D_LIST | D_DET;
	}
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_Buy(s_CatStart[s_Cat] + g_Sel);
		if (r == 0) Sim_Msg("Movie bought - now in your archive.");
		else if (r == 1) Sim_Msg("You already own this movie.");
		else if (r == 3) Sim_MsgWho(Sim_TakenBy(s_CatStart[s_Cat] + g_Sel), 0);
		else Sim_Msg("Not enough money!");
		MarkRows(g_Sel - g_First, g_Sel - g_First);   // so a linha (marca "own"/cor)
		g_Dirty |= D_MSG | D_HDR;
	}
}

// ---------------------------------------------------------------- AGENCIA DE PUBLICIDADE
#if ART_SCENES
#define ADS_RY(i)  ((u8)(CONTENT_Y + 16 + (i) * 12))      // lista num painel cinza a direita da cena; detalhes num painel embaixo
static void DrawRow_Ads_Body(u8 i)
{
	u8 a, y = ADS_RY(i);
	if (i >= NUM_OFFERS) return;
	a = g_Game.offer[i];
	Ui_Fill(126, y - 1, 128, 11, 14);
	if (i == g_Sel) Chrome_Box(126, y - 1, 128, 11, 5, 0);
	Ui_Color(UI_BLACK);
	if (a == NONE) Ui_Text(132, y, "(signed)");
	else Ui_TextN(132, y, g_Ads[a].title, 20);
}

static void DrawRow_Ads(u8 i)
{
	if (i >= NUM_OFFERS) return;
	Ui_Begin();
	DrawRow_Ads_Body(i);
	Ui_End(126, (u8)(ADS_RY(i) - 1), 128, 11);
}

static void DrawDetail_Ads_Body(void)
{
	Chrome_Box(0, 138, 256, 48, 14, 1);
	Ui_Color(UI_BLACK);
	if (g_Game.offer[g_Sel] != NONE)
	{
		const Ad* ad = &g_Ads[g_Game.offer[g_Sel]];
		Ui_Text(8, 142, "Min audience"); Ui_Dec1(92, 142, ad->min_audience); Ui_Text(116, 142, "M viewers");
		Ui_Text(8, 154, "Spots"); Ui_Int(44, 154, ad->reps); Ui_Text(56, 154, "in"); Ui_Int(74, 154, ad->days); Ui_Text(86, 154, "day(s)");
		Ui_Text(8, 166, "Pay"); Money(34, 166, ad->profit);
		Ui_Text(104, 166, "Penalty"); Money(152, 166, Sim_Penalty(g_Game.offer[g_Sel]));
	}
}

static void DrawDetail_Ads(void)
{
	Ui_Begin();
	DrawDetail_Ads_Body();
	Ui_End(0, 138, 256, 48);
}

void Ads_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
	Scene_Draw(SCN_ADS, 0, CONTENT_Y - 2);
	Chrome_Box(122, CONTENT_Y - 2, 134, 112, 14, 1);
	Ui_Color(UI_BLACK); Ui_Text(128, CONTENT_Y + 2, "Today's offers");
	for (i = 0; i < NUM_OFFERS; i++) DrawRow_Ads_Body(i);
	DrawDetail_Ads_Body();
	Ui_End(0, CONTENT_Y - 2, 0, 160);
	Hint("OK:sign contract BACK:office");
}
#else
static void DrawRow_Ads_Body(u8 i)
{
	u8 a, y = CONTENT_Y + 14 + i * 12;
	if (i >= NUM_OFFERS) return;
	a = g_Game.offer[i];
	Ui_Fill(0, y - 1, 255, 11, UI_BG);
	Ui_Color(i == g_Sel ? UI_YELLOW : (a == NONE ? UI_GRAY : UI_WHITE));
	Ui_Text(4, y, i == g_Sel ? ">" : " ");
	if (a == NONE) Ui_Text(14, y, "(signed)");
	else Ui_TextN(14, y, g_Ads[a].title, 22);
}

static void DrawRow_Ads(u8 i)
{
	if (i >= NUM_OFFERS) return;
	Ui_Begin();
	DrawRow_Ads_Body(i);
	Ui_End(0, (u8)(CONTENT_Y + 14 + i * 12 - 1), 255, 11);
}

static void DrawDetail_Ads_Body(void)
{
	Ui_Fill(0, 116, 255, 36, UI_BG);
	if (g_Game.offer[g_Sel] != NONE)
	{
		const Ad* ad = &g_Ads[g_Game.offer[g_Sel]];
		Ui_Color(UI_WHITE);
		Ui_Text(4, 118, "Min audience"); Ui_Dec1(88, 118, ad->min_audience); Ui_Text(112, 118, "M viewers");
		Ui_Text(4, 129, "Spots"); Ui_Int(40, 129, ad->reps); Ui_Text(52, 129, "in"); Ui_Int(70, 129, ad->days); Ui_Text(82, 129, "day(s)");
		Ui_Color(UI_GREEN); Ui_Text(4, 140, "Pay"); Money(30, 140, ad->profit);
		Ui_Color(UI_RED);   Ui_Text(100, 140, "Penalty"); Money(148, 140, Sim_Penalty(g_Game.offer[g_Sel]));
	}
}

static void DrawDetail_Ads(void)
{
	Ui_Begin();
	DrawDetail_Ads_Body();
	Ui_End(0, 116, 255, 36);
}

static void Draw_Ads_Body(void)
{
	u8 i;
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Ad agency - today's offers");
	for (i = 0; i < NUM_OFFERS; i++) DrawRow_Ads(i);
	DrawDetail_Ads();
	Hint("OK:sign contract BACK:office");
}

void Ads_Draw(void) __banked
{
	Ui_Begin();
	Draw_Ads_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

#endif

void Ads_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel + 1 < NUM_OFFERS) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) { MarkRows(old, g_Sel); g_Dirty |= D_DET; }
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_SignAd(g_Sel);
		Sim_Msg(r == 0 ? "Contract signed - schedule it in the grid." : r == 1 ? "Max 4 contracts at a time." : "Already signed.");
		MarkRows(g_Sel, g_Sel); g_Dirty |= D_DET | D_MSG;
	}
}

// ---------------------------------------------------------------- pontos de entrada banked
void Grid_Enter(void) __banked { s_Slot = 0; s_GridCol = 0; s_Pick = 0; s_View = 0; s_PickN = 0; }
void Grid_Row(u8 r) __banked { if (s_Pick) DrawRow_Pick(r); else DrawRow_Grid(r); }
void Grid_List(void) __banked { if (s_Pick) DrawList_Pick(); }
void Grid_Dyn(void) __banked { if (!s_Pick) Draw_GridRows(); }

void Agency_Enter(void) __banked { Index_Movies(); s_Cat = 0; }
void Agency_Row(u8 r) __banked { DrawRow_Agency(r); }
void Agency_List(void) __banked { DrawList_Agency(); }
void Agency_Detail(void) __banked { DrawDetail_Agency(); }

void Ads_Enter(void) __banked { }
void Ads_Row(u8 r) __banked { DrawRow_Ads(r); }
void Ads_Detail(void) __banked { DrawDetail_Ads(); }


// ---------------------------------------------------------------- mouse
#if ART_SCENES
static void GridClear(void)                                        // lixeira / borracha: esvazia o slot selecionado (se ainda nao comecou)
{
	if (g_Game.t >= FIRST_SLOT_T + (u16)s_Slot * 60 + 5) { Sim_Msg("That hour has already started."); g_Dirty |= D_MSG; return; }
	if (g_Game.slot[0][s_Slot].movie == NONE && g_Game.slot[0][s_Slot].ad == NONE) { Sim_Msg("Nothing to erase."); g_Dirty |= D_MSG; return; }
	Sim_ClearSlot(s_Slot); Sim_Msg("Slot cleared.");
	g_Dirty |= D_DAT | D_MSG;
}
#endif

void Grid_Mouse(u8 x, u8 y, u8 btn) __banked
{
	if (s_Pick == 0)
	{
#if ART_SCENES
		u8 r, col = (x >= GR_AX);
		if (x < 30)                                                    // coluna da esquerda: seta p/ cima, seta p/ baixo, lixeira
		{
			if (!btn) return;
			if (y >= 36 && y < 52) Grid_Input(IN_UP);
			else if (y >= 52 && y < 70) Grid_Input(IN_DOWN);
			else if (y >= 114 && y < 134) GridClear();
			return;
		}
		if (x >= 232)                                                  // 6 icones (como no original): so alguns existem nesta versao
		{
			u8 k = (u8)((y - 30) / 17);
			if (!btn || y < 30 || k > 5) return;
			if (k == 0) Goto(SCR_OPTIONS);
			else if (k == 3) Goto(SCR_RATINGS);
			else if (k == 4) GridClear();
			else if (k == 5) Goto(SCR_SLOTS);
			else if (k == 1) { s_View = 1; s_Pick = 2; BuildPick(); g_Sel = 0; g_First = 0; g_Dirty |= D_CON; }      // contratos de anuncio (so leitura)
			else { s_View = 0; s_Pick = 3; g_Dirty |= D_CON; }                                              // resumo da semana
			return;
		}
		r = HitRow(y, GR_Y(0), 16, NUM_SLOTS);
#else
		u8 r = HitRow(y, (u8)(CONTENT_Y + 12), 14, NUM_SLOTS), col = (x >= 152);
#endif
		if (r == 0xFF) return;
		if (r != s_Slot || col != s_GridCol) { u8 old = s_Slot; s_Slot = r; s_GridCol = col; MarkRows(old, r); }
		if (btn) Grid_Input(IN_OK);
	}
	else
	{
#if ART_SCENES
		u8 r = HitRow(y, (u8)(PK_Y(0) - 1), ROW_H, PK_ROWS);
		if (s_Pick == 3) { if (btn) Grid_Input(IN_OK); return; }       // resumo: qualquer clique fecha
		if (x < PK_X || x >= PK_X + PK_W) return;
#else
		u8 r = HitRow(y, (u8)(CONTENT_Y + 13), ROW_H, LIST_ROWS);
#endif
		if (r == 0xFF || !SelVisible(r, s_PickN)) return;
		if (btn) Grid_Input(IN_OK);
	}
}

void Agency_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r;
#if ART_SCENES
	if (x < 122) return;
	if (y >= CONTENT_Y - 1 && y < CONTENT_Y + 14) { if (btn) Agency_Input(x < 190 ? IN_LEFT : IN_RIGHT); return; }
#else
	if (y >= CONTENT_Y - 1 && y < CONTENT_Y + 10) { if (btn) Agency_Input(x < 150 ? IN_LEFT : IN_RIGHT); return; }   // "< genero >"
#endif
	r = HitRow(y, (u8)(AG_RY(0) - 1), ROW_H, AGENCY_ROWS);
	if (r == 0xFF || !SelVisible(r, CatCount(s_Cat))) return;
	if (btn) Agency_Input(IN_OK);
}

void Ads_Mouse(u8 x, u8 y, u8 btn) __banked
{
#if ART_SCENES
	u8 r = HitRow(y, (u8)(ADS_RY(0) - 1), 12, NUM_OFFERS);
	if (x < 122) return;                                   // sobre a cena: nada
#else
	u8 r = HitRow(y, (u8)(CONTENT_Y + 13), 12, NUM_OFFERS);
	(void)x;
#endif
	if (r == 0xFF) return;
	if (r != g_Sel) { u8 old = g_Sel; g_Sel = r; MarkRows(old, r); g_Dirty |= D_DET; }
	if (btn) Ads_Input(IN_OK);
}
