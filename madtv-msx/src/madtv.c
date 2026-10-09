// MadTV-MSX 0.3 - nucleo de gestao: relogio, grade de programacao, audiencia, Image, contratos e dinheiro.
#include "msxgl.h"
#include "ui.h"
#include "sim.h"
#include "data/db_data.h"

#define VERSION_STR "0.3.3"
#define CONTENT_Y   28
#define ROW_H       10
#define MSG_Y       201

enum { SCR_TITLE, SCR_HUB, SCR_GRID, SCR_AGENCY, SCR_ADS, SCR_RATINGS, SCR_OVER };
enum { SPEED_PAUSE, SPEED_1, SPEED_2, SPEED_3, SPEED_COUNT };

static u8 s_Screen = SCR_TITLE;
static u8 s_Speed = SPEED_1;
static u8 s_StationCol[NUM_STATIONS] = { UI_RED, UI_GREEN, UI_CYAN };

// ---------------------------------------------------------------- utilitarios
static void Pad2(char* d, u8 v) { d[0] = '0' + v / 10; d[1] = '0' + v % 10; }

static void ClockStr(char* b, u16 t)     // t = minutos desde 17:00
{
	u16 m = 17 * 60 + t;
	m %= 24 * 60;
	Pad2(b, (u8)(m / 60)); b[2] = ':'; Pad2(b + 3, (u8)(m % 60)); b[5] = 0;
}

static void Money(u8 x, u8 y, i32 k)     // k$
{
	Ui_Text(x, y, "$");
	x = Ui_Int(x + 6, y, (i16)k);
	Ui_Text(x, y, "k");
}

static u8 s_Dirty;       // bits: 1 = cabecalho, 2 = conteudo, 4 = rodape
#define D_HDR 1
#define D_CON 2
#define D_MSG 4
#define D_DAT 8    // atualizar so os dados dinamicos da tela (sem limpar tudo)
#define D_ROWS 16  // redesenhar so as linhas s_RowA/s_RowB (cursor andou sem rolar)
#define D_LIST 32  // redesenhar todas as linhas da lista (rolagem/troca de genero), sem limpar a tela
#define D_DET 64   // redesenhar so o painel de detalhes

static u8 s_RowA = 0xFF, s_RowB = 0xFF;   // linhas visiveis a redesenhar (D_ROWS)

static void MarkRows(u8 a, u8 b) { s_RowA = a; s_RowB = b; s_Dirty |= D_ROWS; }

// Cabecalho incremental: cada campo so e redesenhado se mudou (evita apagar/redesenhar tudo a cada minuto de jogo)
static u16 c_Day = 0xFFFF, c_T = 0xFFFF;
static i32 c_Money = 0x7FFFFFFF;
static u8  c_Image[NUM_STATIONS] = { 255, 255, 255 };
static u8  c_Speed = 255;

static void Header_Invalidate(void)
{
	u8 i;
	c_Day = 0xFFFF; c_T = 0xFFFF; c_Money = 0x7FFFFFFF; c_Speed = 255;
	for (i = 0; i < NUM_STATIONS; i++) c_Image[i] = 255;
	Ui_Fill(0, 0, 255, 24, UI_BG);
	Ui_Fill(0, 23, 255, 1, COLOR_GRAY);
}

static void Draw_Header(void)
{
	char b[8];
	u8 st;
	if (c_Day != g_Game.day)
	{
		c_Day = g_Game.day;
		Ui_Begin(); Ui_Fill(0, 1, 60, 10, UI_BG);
		Ui_Color(UI_YELLOW); Ui_Text(4, 2, "Day "); Ui_Int(28, 2, g_Game.day);
		Ui_End(0, 1, 60, 10);
	}
	if (c_T != g_Game.t)
	{
		c_T = g_Game.t;
		Ui_Begin(); Ui_Fill(62, 1, 34, 10, UI_BG);
		Ui_Color(UI_YELLOW); ClockStr(b, g_Game.t); Ui_Text(64, 2, b);
		Ui_End(62, 1, 34, 10);
	}
	if (c_Money != g_Game.money)
	{
		c_Money = g_Game.money;
		Ui_Begin(); Ui_Fill(118, 1, 72, 10, UI_BG);
		Ui_Color(g_Game.money < 0 ? UI_RED : UI_WHITE);
		Money(120, 2, g_Game.money);
		Ui_End(118, 1, 72, 10);
	}
	if (c_Speed != s_Speed)
	{
		c_Speed = s_Speed;
		Ui_Begin(); Ui_Fill(200, 1, 55, 10, UI_BG);
		Ui_Color(UI_GRAY);
		Ui_TextR(252, 2, s_Speed == SPEED_PAUSE ? "PAUSE" : s_Speed == SPEED_1 ? ">" : s_Speed == SPEED_2 ? ">>" : ">>>");
		Ui_End(200, 1, 55, 10);
	}
	for (st = 0; st < NUM_STATIONS; st++)
		if (c_Image[st] != g_Game.image[st])
		{
			c_Image[st] = g_Game.image[st];
			Ui_Begin(); Ui_Fill(4 + st * 84, 12, 80, 10, UI_BG);
			Ui_Color(s_StationCol[st]);
			Ui_Text(4 + st * 84, 13, g_StationName[st]);
			Ui_Int(40 + st * 84, 13, g_Game.image[st]);
			Ui_End(4 + st * 84, 12, 80, 10);
		}
}

static void Draw_Msg(void)
{
	Ui_Begin();
	Ui_Fill(0, (u8)(MSG_Y - 1), 255, 11, UI_BG);
	Ui_Color(UI_YELLOW);
	Ui_Text(4, MSG_Y, g_Msg);
	Ui_End(0, (u8)(MSG_Y - 1), 255, 11);
}

static void Hint(const char* s)
{
	u16 saved = Ui_DirectBegin();      // dica: direto na tela (pode ser chamada dentro de um desenho em buffer)
	Ui_Fill(0, 190, 255, 10, UI_BG);
	Ui_Color(UI_GRAY);
	Ui_Text(4, 191, s);
	Ui_DirectEnd(saved);
}

static void ClearContent(void) { Ui_Fill(0, CONTENT_Y - 2, 255, 160, UI_BG); }

// ---------------------------------------------------------------- HUB
static const char* const k_Menu[] = { "Programme grid", "Film agency", "Ad agency", "Ratings & image" };
#define MENU_N 4
static u8 s_Menu;

static void Draw_HubDyn_Body(void)
{
	u8 i, n = 0, y;
	Ui_Fill(0, CONTENT_Y + 60, 255, 80, UI_BG);
	Ui_Color(UI_GRAY);
	for (i = 0; i < DB_NUM_MOVIES; i++) n += g_Game.owned[i];
	Ui_Text(4, CONTENT_Y + 62, "Movies owned:"); Ui_Int(86, CONTENT_Y + 62, n);
	Ui_Text(4, CONTENT_Y + 76, "Contracts:");
	for (i = 0; i < MAX_CONTRACTS; i++)
	{
		const Contract* c = &g_Game.contract[i];
		if (c->ad == NONE) continue;
		y = CONTENT_Y + 86 + i * ROW_H;
		Ui_Color(UI_WHITE);
		Ui_TextN(4, y, g_Ads[c->ad].title, 16);
		Ui_Color(UI_GRAY);
		Ui_Int(112, y, c->reps_left); Ui_Text(124, y, "x");
		Ui_Int(140, y, c->days_left); Ui_Text(152, y, "d");
	}
}

static void Draw_HubDyn(void)
{
	Ui_Begin();
	Draw_HubDyn_Body();
	Ui_End(0, CONTENT_Y + 60, 255, 80);
}

static void DrawRow_Hub_Body(u8 i)
{
	u8 y = CONTENT_Y + 14 + i * ROW_H;
	if (i >= MENU_N) return;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(i == s_Menu ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, i == s_Menu ? ">" : " ");
	Ui_Text(16, y, k_Menu[i]);
}

static void DrawRow_Hub(u8 i)
{
	if (i >= MENU_N) return;
	Ui_Begin();
	DrawRow_Hub_Body(i);
	Ui_End(0, (u8)(CONTENT_Y + 14 + i * ROW_H - 1), 255, ROW_H);
}

static void Draw_Hub_Body(void)
{
	u8 i;
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Office");
	for (i = 0; i < MENU_N; i++) DrawRow_Hub(i);
	Draw_HubDyn();
	Hint("OK:enter  TAB:speed  P:pause");
}

static void Draw_Hub(void)
{
	Ui_Begin();
	Draw_Hub_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

// ---------------------------------------------------------------- listas genericas
#define LIST_ROWS 11
static u8 s_Sel, s_First;

// Navegacao vertical de lista com janela de `rows` linhas. Retorna 0 = nada, 1 = so linhas A/B mudaram, 2 = rolou.
static u8 Nav(u8 ev, u8 n, u8 rows)
{
	u8 old = s_Sel, oldfirst = s_First;
	if ((ev & IN_DOWN) && s_Sel + 1 < n) { s_Sel++; if (s_Sel >= s_First + rows) s_First++; }
	if ((ev & IN_UP) && s_Sel > 0)       { s_Sel--; if (s_Sel < s_First) s_First--; }
	if (s_Sel == old) return 0;
	if (s_First != oldfirst) return 2;
	s_RowA = old - s_First; s_RowB = s_Sel - s_First;
	return 1;
}

static void NavApply(u8 r)
{
	if (r == 1) s_Dirty |= D_ROWS | D_DET;
	else if (r == 2) s_Dirty |= D_LIST | D_DET;
}

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
	if (s_First + i >= s_PickN) return;
	v = s_PickList[s_First + i];
	Ui_Color(s_First + i == s_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, s_First + i == s_Sel ? ">" : " ");
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

static void Draw_Grid(void)
{
	Ui_Begin();
	Draw_Grid_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

static void Grid_Input(u8 ev)
{
	if (s_Pick == 0)
	{
		u8 old = s_Slot;
		if ((ev & IN_DOWN) && s_Slot + 1 < NUM_SLOTS) s_Slot++;
		if ((ev & IN_UP) && s_Slot > 0) s_Slot--;
		if (s_Slot != old) MarkRows(old, s_Slot);
		if (ev & (IN_LEFT | IN_RIGHT)) { s_GridCol ^= 1; MarkRows(s_Slot, s_Slot); }
		if (ev & IN_BACK) { s_Screen = SCR_HUB; s_Dirty |= D_CON; return; }
		if (ev & IN_OK)
		{
			if (g_Game.t >= FIRST_SLOT_T + (u16)s_Slot * 60 + 5) { Sim_Msg("That hour has already started."); s_Dirty |= D_MSG; return; }
			s_Pick = s_GridCol ? 2 : 1;
			BuildPick(); s_Sel = 0; s_First = 0; s_Dirty |= D_CON;
		}
		return;
	}
	NavApply(Nav(ev, s_PickN, LIST_ROWS));
	if (ev & IN_BACK) { s_Pick = 0; s_Dirty |= D_CON; return; }
	if (ev & IN_OK)
	{
		u8 v = s_PickList[s_Sel];
		if (s_Pick == 1)
		{
			if (v == NONE) { Sim_ClearSlot(s_Slot); Sim_Msg("Slot cleared."); }
			else if (Sim_PlaceMovie(s_Slot, v)) Sim_Msg("Movie does not fit before midnight.");
			else Sim_Msg("Programme set.");
		}
		else { Sim_PlaceAd(s_Slot, v); Sim_Msg(v == NONE ? "Ad removed." : "Ad contract scheduled."); }
		s_Pick = 0; s_Dirty |= D_CON | D_MSG;
	}
}

// ---------------------------------------------------------------- AGENCIA DE FILMES
static u8 s_Cat;
static u8 s_CatStart[DB_NUM_CATEGORIES + 1];

static u8 CatCount(u8 c) { return (u8)(s_CatStart[c + 1] - s_CatStart[c]); }

#define AGENCY_ROWS 9

static void DrawRow_Agency_Body(u8 i)
{
	u8 idx, y = CONTENT_Y + 12 + i * ROW_H;
	if (i >= AGENCY_ROWS) return;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (s_First + i >= CatCount(s_Cat)) return;
	idx = s_CatStart[s_Cat] + s_First + i;
	Ui_Color(s_First + i == s_Sel ? UI_YELLOW : (g_Game.owned[idx] ? UI_GREEN : UI_WHITE));
	Ui_Text(4, y, s_First + i == s_Sel ? ">" : " ");
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
	u8 idx = s_CatStart[s_Cat] + s_Sel;
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

static void Draw_Agency(void)
{
	Ui_Begin();
	Draw_Agency_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

static void Agency_Input(u8 ev)
{
	NavApply(Nav(ev, CatCount(s_Cat), AGENCY_ROWS));
	if (ev & (IN_LEFT | IN_RIGHT))
	{
		s_Cat = (ev & IN_RIGHT) ? (s_Cat + 1) % DB_NUM_CATEGORIES : (s_Cat + DB_NUM_CATEGORIES - 1) % DB_NUM_CATEGORIES;
		s_Sel = s_First = 0;
		DrawTitle_Agency();                      // so o titulo do genero + lista (sem limpar a tela)
		s_Dirty |= D_LIST | D_DET;
	}
	if (ev & IN_BACK) { s_Screen = SCR_HUB; s_Dirty |= D_CON; return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_Buy(s_CatStart[s_Cat] + s_Sel);
		if (r == 0) Sim_Msg("Movie bought - now in your archive.");
		else if (r == 1) Sim_Msg("You already own this movie.");
		else Sim_Msg("Not enough money!");
		MarkRows(s_Sel - s_First, s_Sel - s_First);   // so a linha (marca "own"/cor)
		s_Dirty |= D_MSG | D_HDR;
	}
}

// ---------------------------------------------------------------- AGENCIA DE PUBLICIDADE
static void DrawRow_Ads_Body(u8 i)
{
	u8 a, y = CONTENT_Y + 14 + i * 12;
	if (i >= NUM_OFFERS) return;
	a = g_Game.offer[i];
	Ui_Fill(0, y - 1, 255, 11, UI_BG);
	Ui_Color(i == s_Sel ? UI_YELLOW : (a == NONE ? UI_GRAY : UI_WHITE));
	Ui_Text(4, y, i == s_Sel ? ">" : " ");
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
	if (g_Game.offer[s_Sel] != NONE)
	{
		const Ad* ad = &g_Ads[g_Game.offer[s_Sel]];
		Ui_Color(UI_WHITE);
		Ui_Text(4, 118, "Min audience"); Ui_Dec1(88, 118, ad->min_audience); Ui_Text(112, 118, "M viewers");
		Ui_Text(4, 129, "Spots"); Ui_Int(40, 129, ad->reps); Ui_Text(52, 129, "in"); Ui_Int(70, 129, ad->days); Ui_Text(82, 129, "day(s)");
		Ui_Color(UI_GREEN); Ui_Text(4, 140, "Pay"); Money(30, 140, ad->profit);
		Ui_Color(UI_RED);   Ui_Text(100, 140, "Penalty"); Money(148, 140, ad->penalty);
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

static void Draw_Ads(void)
{
	Ui_Begin();
	Draw_Ads_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

static void Ads_Input(u8 ev)
{
	u8 old = s_Sel;
	if ((ev & IN_DOWN) && s_Sel + 1 < NUM_OFFERS) s_Sel++;
	if ((ev & IN_UP) && s_Sel > 0) s_Sel--;
	if (s_Sel != old) { MarkRows(old, s_Sel); s_Dirty |= D_DET; }
	if (ev & IN_BACK) { s_Screen = SCR_HUB; s_Dirty |= D_CON; return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_SignAd(s_Sel);
		Sim_Msg(r == 0 ? "Contract signed - schedule it in the grid." : r == 1 ? "Max 4 contracts at a time." : "Already signed.");
		MarkRows(s_Sel, s_Sel); s_Dirty |= D_DET | D_MSG;
	}
}

// ---------------------------------------------------------------- AUDIENCIAS / IMAGE
static void Draw_RatingsDyn(void)
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
			Ui_Color(s_StationCol[st]); Ui_Dec1(60 + st * 54, y, g_Game.aud[st][s]);
			Ui_Color(UI_GRAY); Ui_Int(84 + st * 54, y, Sim_Quota(st, s));
		}
	}
	Ui_End(56, (u8)(CONTENT_Y + 23), 190, 70);       // tabela (linhas sao preenchidas por inteiro antes de escrever)
	Ui_Begin();
	Ui_Fill(50, (u8)(CONTENT_Y + 111), 200, 32, UI_BG);
	for (st = 0; st < NUM_STATIONS; st++)
	{
		y = (u8)(CONTENT_Y + 112 + st * 10);
		Ui_Bar(50, y + 1, 160, 6, g_Game.image[st], s_StationCol[st] == UI_RED ? COLOR_MEDIUM_RED : s_StationCol[st] == UI_GREEN ? COLOR_LIGHT_GREEN : COLOR_CYAN);
		Ui_Fill(214, y - 1, 30, 9, UI_BG);
		Ui_Color(s_StationCol[st]); Ui_Int(216, y, g_Game.image[st]);
	}
	Ui_End(50, (u8)(CONTENT_Y + 111), 200, 32);
}

static void Draw_Ratings_Body(void)
{
	u8 st;
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Ratings (million viewers)");
	for (st = 0; st < NUM_STATIONS; st++) { Ui_Color(s_StationCol[st]); Ui_Text(60 + st * 54, CONTENT_Y + 12, g_StationName[st]); }
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 100), "Image");
	for (st = 0; st < NUM_STATIONS; st++) { Ui_Color(s_StationCol[st]); Ui_Text(4, CONTENT_Y + 112 + st * 10, g_StationName[st]); }
	Draw_RatingsDyn();
	Hint("BACK:office");
}

static void Draw_Ratings(void)
{
	Ui_Begin();
	Draw_Ratings_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
}

// ---------------------------------------------------------------- telas especiais
static void Draw_Title(void)
{
	Ui_Clear();
	Ui_Color(UI_YELLOW); Ui_Text(70, 50, "M A D   T V");
	Ui_Color(UI_WHITE);  Ui_Text(52, 66, "MSX2 port - version " VERSION_STR);
	Ui_Color(UI_GRAY);
	Ui_Text(14, 100, "Run the station: buy movies, sign");
	Ui_Text(14, 110, "ad contracts, fill the programme grid");
	Ui_Text(14, 120, "and beat FunTV and SunTV in the ratings.");
	Ui_Color(UI_GREEN);  Ui_Text(14, 146, "Arrows/joystick: move   OK(Enter/Space)");
	Ui_Text(14, 156, "BACK(Esc): back   TAB: speed   P: pause");
	Ui_Color(UI_YELLOW); Ui_Text(60, 184, "Press OK to start");
	Ui_Color(UI_GRAY);   Ui_Text(4, 202, "Data: TVTower (altered for MSX)");
}

static void Draw_Over(void)
{
	Ui_Clear();
	Ui_Color(UI_RED);   Ui_Text(84, 70, "BANKRUPT!");
	Ui_Color(UI_WHITE); Ui_Text(40, 90, "Mr. Raffer shows you the door.");
	Ui_Color(UI_GRAY);  Ui_Text(60, 110, "Survived "); Ui_Int(114, 110, g_Game.day); Ui_Text(132, 110, "day(s)");
	Ui_Color(UI_YELLOW); Ui_Text(60, 150, "Press OK to restart");
}

// ---------------------------------------------------------------- principal
static void Enter(u8 scr)
{
	s_Screen = scr; s_Sel = 0; s_First = 0; s_Pick = 0; s_Dirty |= D_CON;
}

static void Hub_Input(u8 ev)
{
	u8 old = s_Menu;
	if ((ev & IN_DOWN) && s_Menu + 1 < MENU_N) s_Menu++;
	if ((ev & IN_UP) && s_Menu > 0) s_Menu--;
	if (s_Menu != old) MarkRows(old, s_Menu);
	if (ev & IN_OK)
	{
		switch (s_Menu)
		{
		case 0: Enter(SCR_GRID); s_Slot = 0; break;
		case 1: Enter(SCR_AGENCY); break;
		case 2: Enter(SCR_ADS); break;
		default: Enter(SCR_RATINGS); break;
		}
	}
}

static void Index_Movies(void)
{
	u8 i, c;
	for (c = 0; c <= DB_NUM_CATEGORIES; c++) s_CatStart[c] = DB_NUM_MOVIES;
	for (i = DB_NUM_MOVIES; i > 0; i--) s_CatStart[g_Movies[i - 1].cat] = i - 1;
	s_CatStart[DB_NUM_CATEGORIES] = DB_NUM_MOVIES;
	for (c = DB_NUM_CATEGORIES; c > 0; c--) if (s_CatStart[c - 1] > s_CatStart[c]) s_CatStart[c - 1] = s_CatStart[c];
}

static void StartGame(void)
{
	Sim_Init(*(volatile u16*)0xFC9E ^ 0x5A5A);   // semente: JIFFY do BIOS no momento do OK
	s_Speed = SPEED_1; s_Menu = 0;
	Ui_Clear();
	Header_Invalidate();
	s_Screen = SCR_HUB;
	s_Dirty = D_HDR | D_CON | D_MSG;
}

void main()
{
	u8 ev, e, hz, fc = 0, fpm;
	// O crt0 do MSXgl NAO zera a RAM (BSS): em hardware real ela contem lixo. Todo estado e inicializado aqui.
	s_Dirty = 0; s_Menu = 0; s_Sel = 0; s_First = 0; s_GridCol = 0; s_Slot = 0; s_Pick = 0; s_PickN = 0; s_Cat = 0;
	Ui_Init();
	Index_Movies();
	Draw_Title();

	for (;;)
	{
		Halt();
		ev = Input_Poll();

		if (s_Screen == SCR_TITLE) { if (ev & IN_OK) StartGame(); continue; }
		if (s_Screen == SCR_OVER)  { if (ev & IN_OK) StartGame(); continue; }

		// velocidade (normalizada para 50/60 Hz): 1 min de jogo = 1/2, 1/5 ou 1/12 de segundo real
		hz = VDP_GetFrequency() ? 50 : 60;
		fpm = (s_Speed == SPEED_1) ? hz / 2 : (s_Speed == SPEED_2) ? hz / 5 : hz / 12;
		if (ev & IN_SPEED) { s_Speed = (s_Speed % (SPEED_COUNT - 1)) + 1; s_Dirty |= D_HDR; }
		if (ev & IN_PAUSE) { s_Speed = (s_Speed == SPEED_PAUSE) ? SPEED_1 : SPEED_PAUSE; s_Dirty |= D_HDR; }

		if (s_Speed != SPEED_PAUSE && ++fc >= fpm)
		{
			fc = 0;
			e = Sim_Tick();
			s_Dirty |= D_HDR;
			if (e & EV_MSG) s_Dirty |= D_MSG;
			if (e & (EV_SLOT | EV_DAY))
			{
				if (s_Screen == SCR_ADS && (e & EV_DAY)) s_Dirty |= D_CON;       // ofertas novas
				else if (s_Screen == SCR_GRID && !s_Pick) s_Dirty |= D_DAT;
				else if (s_Screen == SCR_RATINGS || s_Screen == SCR_HUB) s_Dirty |= D_DAT;
			}
			if (g_Game.game_over) { s_Screen = SCR_OVER; Draw_Over(); continue; }
		}

		switch (s_Screen)
		{
		case SCR_HUB:     Hub_Input(ev); break;
		case SCR_GRID:    Grid_Input(ev); break;
		case SCR_AGENCY:  Agency_Input(ev); break;
		case SCR_ADS:     Ads_Input(ev); break;
		case SCR_RATINGS: if (ev & IN_BACK) { s_Screen = SCR_HUB; s_Dirty |= D_CON; } break;
		}

		if (s_Dirty & D_HDR) Draw_Header();
		if ((s_Dirty & D_DAT) && !(s_Dirty & D_CON))
		{
			switch (s_Screen)
			{
			case SCR_HUB:     Draw_HubDyn(); break;
			case SCR_GRID:    Draw_GridRows(); break;
			case SCR_RATINGS: Draw_RatingsDyn(); break;
			}
		}
		if (s_Dirty & D_CON)
		{
			switch (s_Screen)
			{
			case SCR_HUB:     Draw_Hub(); break;
			case SCR_GRID:    Draw_Grid(); break;
			case SCR_AGENCY:  Draw_Agency(); break;
			case SCR_ADS:     Draw_Ads(); break;
			case SCR_RATINGS: Draw_Ratings(); break;
			}
		}
		else if (s_Dirty & (D_ROWS | D_LIST | D_DET))
		{
			// cursor/lista: so redesenha o que mudou (linhas A/B, lista inteira sem limpar a tela, ou detalhes)
			u8 pick = (s_Screen == SCR_GRID && s_Pick);
			if (s_Dirty & D_LIST)
			{
				switch (s_Screen)
				{
				case SCR_GRID:   if (pick) DrawList_Pick(); break;
				case SCR_AGENCY: DrawList_Agency(); break;
				}
			}
			else if (s_Dirty & D_ROWS)
			{
				u8 i;
				for (i = 0; i < 2; i++)
				{
					u8 r = i ? s_RowB : s_RowA;
					if (r == 0xFF || (i && s_RowB == s_RowA)) continue;
					switch (s_Screen)
					{
					case SCR_HUB:    DrawRow_Hub(r); break;
					case SCR_GRID:   if (pick) DrawRow_Pick(r); else DrawRow_Grid(r); break;
					case SCR_AGENCY: DrawRow_Agency(r); break;
					case SCR_ADS:    DrawRow_Ads(r); break;
					}
				}
			}
			if (s_Dirty & D_DET)
			{
				switch (s_Screen)
				{
				case SCR_AGENCY: DrawDetail_Agency(); break;
				case SCR_ADS:    DrawDetail_Ads(); break;
				}
			}
		}
		if (s_Dirty & D_MSG) Draw_Msg();
		s_Dirty = 0;
		s_RowA = s_RowB = 0xFF;
	}
}
