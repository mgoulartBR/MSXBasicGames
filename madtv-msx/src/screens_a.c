// Telas: Grade de programacao, Agencia de filmes, Agencia de publicidade, Audiencias. Compilado no segmento 5 (banco 2).
#include "app.h"

// ---------------------------------------------------------------- GRID
static u8 s_GridCol;        // 0 = programa, 1 = anuncio
static u8 s_Slot;           // slot sob o cursor
static u8 s_Pick;           // 0 = navegando a grade, 1 = escolhendo filme, 2 = escolhendo contrato
static u8 s_PickList[DB_NUM_MOVIES + 1];
static u8 s_PickN;

static void BuildPick(void)
{
	u8 i;
	s_PickN = 0;
	s_PickList[s_PickN++] = NONE;
	if (s_Pick == 1) { for (i = 0; i < DB_NUM_MOVIES; i++) if (g_Game.owned[i]) s_PickList[s_PickN++] = i; }
	else             { for (i = 0; i < MAX_CONTRACTS; i++) if (g_Game.contract[i].ad != NONE) s_PickList[s_PickN++] = i; }
}

static void DrawRow_Grid_Body(u8 s)
{
	char b[8];
	const Slot* sl;
	u8 y;
	if (s >= NUM_SLOTS) return;
	sl = &g_Game.slot[0][s];
	y = CONTENT_Y + 14 + s * 14;
	Ui_Fill(0, y - 2, 255, 13, UI_BG);
	ClockStr(b, FIRST_SLOT_T + s * 60);
	Ui_Color(s == s_Slot ? UI_YELLOW : UI_GRAY);
	Ui_Text(4, y, s == s_Slot ? ">" : " "); Ui_Text(12, y, b);
	if (sl->movie == NONE) { Ui_Color(UI_GRAY); Ui_Text(48, y, "- empty -"); }
	else if (sl->part) { Ui_Color(UI_GRAY); Ui_Text(48, y, "  (cont.)"); }
	else { Ui_Color((s == s_Slot && s_GridCol == 0) ? UI_YELLOW : UI_WHITE); Ui_TextN(48, y, g_Movies[sl->movie].title, 17); }
	if (sl->ad == NONE) { Ui_Color(UI_GRAY); Ui_Text(156, y, "no ad"); }
	else { Ui_Color((s == s_Slot && s_GridCol == 1) ? UI_YELLOW : UI_CYAN); Ui_TextN(156, y, g_Ads[g_Game.contract[sl->ad].ad].title, 16); }
}

static void DrawRow_Grid(u8 s)
{
	if (s >= NUM_SLOTS) return;
	Ui_Begin();
	DrawRow_Grid_Body(s);
	Ui_End(0, (u8)(CONTENT_Y + 14 + s * 14 - 2), 255, 13);
}

static void Draw_GridRows(void)
{
	u8 s;
	for (s = 0; s < NUM_SLOTS; s++) DrawRow_Grid(s);
}

static void DrawRow_Pick_Body(u8 i)
{
	u8 v, y;
	if (i >= LIST_ROWS) return;
	y = CONTENT_Y + 14 + i * ROW_H;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (g_First + i >= s_PickN) return;
	v = s_PickList[g_First + i];
	Ui_Color(g_First + i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, g_First + i == g_Sel ? ">" : " ");
	if (v == NONE) { Ui_Text(14, y, "(none)"); return; }
	if (s_Pick == 1)
	{
		Ui_TextN(14, y, g_Movies[v].title, 22);
		Ui_Color(UI_GRAY);
		Ui_Int(170, y, g_Movies[v].blocks); Ui_Text(178, y, "bl");
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
	if (i >= LIST_ROWS) return;
	Ui_Begin();
	DrawRow_Pick_Body(i);
	Ui_End(0, (u8)(CONTENT_Y + 14 + i * ROW_H - 1), 255, ROW_H);
}

static void DrawList_Pick(void)
{
	u8 i;
	for (i = 0; i < LIST_ROWS; i++) DrawRow_Pick(i);
}

static void Draw_Grid_Body(void)
{
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
		if (ev & IN_BACK) { Goto(SCR_HUB); return; }
		if (ev & IN_OK)
		{
			if (g_Game.t >= FIRST_SLOT_T + (u16)s_Slot * 60 + 5) { Sim_Msg("That hour has already started."); g_Dirty |= D_MSG; return; }
			s_Pick = s_GridCol ? 2 : 1;
			BuildPick(); g_Sel = 0; g_First = 0; g_Dirty |= D_CON;
		}
		return;
	}
	NavApply(Nav(ev, s_PickN, LIST_ROWS));
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

static void DrawRow_Agency_Body(u8 i)
{
	u8 idx, y = CONTENT_Y + 12 + i * ROW_H;
	if (i >= AGENCY_ROWS) return;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (g_First + i >= CatCount(s_Cat)) return;
	idx = s_CatStart[s_Cat] + g_First + i;
	Ui_Color(g_First + i == g_Sel ? UI_YELLOW : (g_Game.owned[idx] ? UI_GREEN : UI_WHITE));
	Ui_Text(4, y, g_First + i == g_Sel ? ">" : " ");
	Ui_TextN(14, y, g_Movies[idx].title, 24);
	if (g_Game.owned[idx]) { Ui_Color(UI_GRAY); Ui_Text(232, y, "own"); }
}

static void DrawRow_Agency(u8 i)
{
	if (i >= AGENCY_ROWS) return;
	Ui_Begin();
	DrawRow_Agency_Body(i);
	Ui_End(0, (u8)(CONTENT_Y + 12 + i * ROW_H - 1), 255, ROW_H);
}

static void DrawList_Agency(void)
{
	u8 i;
	for (i = 0; i < AGENCY_ROWS; i++) DrawRow_Agency(i);
}

static void DrawTitle_Agency_Body(void)
{
	Ui_Fill(96, CONTENT_Y - 1, 120, 10, UI_BG);
	Ui_Color(UI_WHITE);  Ui_Text(100, CONTENT_Y, "< "); Ui_Text(112, CONTENT_Y, g_CategoryName[s_Cat]);
	Ui_Text((u8)(112 + 6 * 10 + 6), CONTENT_Y, ">");
}

static void DrawTitle_Agency(void)
{
	Ui_Begin();
	DrawTitle_Agency_Body();
	Ui_End(96, CONTENT_Y - 1, 120, 10);
}

static void DrawDetail_Agency_Body(void)
{
	u8 idx = s_CatStart[s_Cat] + g_Sel;
	const Movie* m = &g_Movies[idx];
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
	Ui_End(0, 132, 255, 35);
}

static void Draw_Agency_Body(void)
{
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Film agency");
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
		else Sim_Msg("Not enough money!");
		MarkRows(g_Sel - g_First, g_Sel - g_First);   // so a linha (marca "own"/cor)
		g_Dirty |= D_MSG | D_HDR;
	}
}

// ---------------------------------------------------------------- AGENCIA DE PUBLICIDADE
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


// ---------------------------------------------------------------- pontos de entrada banked
void Grid_Enter(void) __banked { s_Slot = 0; s_GridCol = 0; s_Pick = 0; s_PickN = 0; }
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

void Ratings_Dyn(void) __banked { RatingsDynLocal(); }
void Ratings_Input(u8 ev) __banked { if (ev & IN_BACK) Goto(SCR_HUB); }
