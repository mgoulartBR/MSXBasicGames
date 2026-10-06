// MadTV-MSX 0.3 - nucleo de gestao: relogio, grade de programacao, audiencia, Image, contratos e dinheiro.
#include "msxgl.h"
#include "ui.h"
#include "sim.h"
#include "data/db_data.h"

#define VERSION_STR "0.3"
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

static void Draw_Header(void)
{
	char b[8];
	u8 st;
	Ui_Fill(0, 0, 255, 24, UI_BG);
	Ui_Color(UI_YELLOW);
	Ui_Text(4, 2, "Day "); Ui_Int(28, 2, g_Game.day);
	ClockStr(b, g_Game.t); Ui_Text(64, 2, b);
	Ui_Color(g_Game.money < 0 ? UI_RED : UI_WHITE);
	Money(120, 2, g_Game.money);
	Ui_Color(UI_GRAY);
	Ui_TextR(252, 2, s_Speed == SPEED_PAUSE ? "PAUSE" : s_Speed == SPEED_1 ? ">" : s_Speed == SPEED_2 ? ">>" : ">>>");
	for (st = 0; st < NUM_STATIONS; st++)
	{
		Ui_Color(s_StationCol[st]);
		Ui_Text(4 + st * 84, 13, g_StationName[st]);
		Ui_Int(40 + st * 84, 13, g_Game.image[st]);
	}
	Ui_Fill(0, 23, 255, 1, COLOR_GRAY);
}

static void Draw_Msg(void)
{
	Ui_Fill(0, (u8)(MSG_Y - 1), 255, 11, UI_BG);
	Ui_Color(UI_YELLOW);
	Ui_Text(4, MSG_Y, g_Msg);
}

static void Hint(const char* s)
{
	Ui_Fill(0, 190, 255, 10, UI_BG);
	Ui_Color(UI_GRAY);
	Ui_Text(4, 191, s);
}

static void ClearContent(void) { Ui_Fill(0, CONTENT_Y - 2, 255, 160, UI_BG); }

// ---------------------------------------------------------------- HUB
static const char* const k_Menu[] = { "Programme grid", "Film agency", "Ad agency", "Ratings & image" };
#define MENU_N 4
static u8 s_Menu;

static void Draw_Hub(void)
{
	u8 i, n = 0;
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Office");
	for (i = 0; i < MENU_N; i++)
	{
		Ui_Color(i == s_Menu ? UI_YELLOW : UI_WHITE);
		Ui_Text(4, CONTENT_Y + 14 + i * ROW_H, i == s_Menu ? "> " : "  ");
		Ui_Text(16, CONTENT_Y + 14 + i * ROW_H, k_Menu[i]);
	}
	Ui_Color(UI_GRAY);
	for (i = 0; i < DB_NUM_MOVIES; i++) n += g_Game.owned[i];
	Ui_Text(4, CONTENT_Y + 62, "Movies owned:"); Ui_Int(86, CONTENT_Y + 62, n);
	Ui_Text(4, CONTENT_Y + 76, "Contracts:");
	for (i = 0; i < MAX_CONTRACTS; i++)
	{
		const Contract* c = &g_Game.contract[i];
		if (c->ad == NONE) continue;
		Ui_Color(UI_WHITE);
		Ui_TextN(4, CONTENT_Y + 86 + i * ROW_H, g_Ads[c->ad].title, 16);
		Ui_Color(UI_GRAY);
		Ui_Int(112, CONTENT_Y + 86 + i * ROW_H, c->reps_left); Ui_Text(124, CONTENT_Y + 86 + i * ROW_H, "x");
		Ui_Int(140, CONTENT_Y + 86 + i * ROW_H, c->days_left); Ui_Text(152, CONTENT_Y + 86 + i * ROW_H, "d");
	}
	Hint("OK:enter  TAB:speed  P:pause");
}

// ---------------------------------------------------------------- listas genericas
#define LIST_ROWS 11
static u8 s_Sel, s_First;

static void ListMove(u8 ev, u8 n)
{
	if ((ev & IN_DOWN) && s_Sel + 1 < n) { s_Sel++; if (s_Sel >= s_First + LIST_ROWS) s_First++; s_Dirty |= D_CON; }
	if ((ev & IN_UP) && s_Sel > 0)       { s_Sel--; if (s_Sel < s_First) s_First--;               s_Dirty |= D_CON; }
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

static void Draw_GridRows(void)
{
	u8 s;
	char b[8];
	for (s = 0; s < NUM_SLOTS; s++)
	{
		const Slot* sl = &g_Game.slot[0][s];
		u8 y = CONTENT_Y + 14 + s * 14;
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
}

static void Draw_Grid(void)
{
	u8 i;
	ClearContent();
	if (s_Pick == 0)
	{
		Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Programme grid (today)");
		Draw_GridRows();
		Hint("L/R:column OK:edit BACK:office");
		return;
	}
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, s_Pick == 1 ? "Choose programme" : "Choose ad contract");
	for (i = 0; i < LIST_ROWS && s_First + i < s_PickN; i++)
	{
		u8 v = s_PickList[s_First + i], y = CONTENT_Y + 14 + i * ROW_H;
		Ui_Color(s_First + i == s_Sel ? UI_YELLOW : UI_WHITE);
		Ui_Text(4, y, s_First + i == s_Sel ? ">" : " ");
		if (v == NONE) { Ui_Text(14, y, "(none)"); continue; }
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
	Hint("OK:place BACK:cancel");
}

static void Grid_Input(u8 ev)
{
	if (s_Pick == 0)
	{
		if ((ev & IN_DOWN) && s_Slot + 1 < NUM_SLOTS) { s_Slot++; s_Dirty |= D_CON; }
		if ((ev & IN_UP) && s_Slot > 0) { s_Slot--; s_Dirty |= D_CON; }
		if (ev & (IN_LEFT | IN_RIGHT)) { s_GridCol ^= 1; s_Dirty |= D_CON; }
		if (ev & IN_BACK) { s_Screen = SCR_HUB; s_Dirty |= D_CON; return; }
		if (ev & IN_OK)
		{
			if (g_Game.t >= FIRST_SLOT_T + (u16)s_Slot * 60 + 5) { Sim_Msg("That hour has already started."); s_Dirty |= D_MSG; return; }
			s_Pick = s_GridCol ? 2 : 1;
			BuildPick(); s_Sel = 0; s_First = 0; s_Dirty |= D_CON;
		}
		return;
	}
	ListMove(ev, s_PickN);
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

static void Draw_Agency(void)
{
	u8 i, n = CatCount(s_Cat), idx;
	const Movie* m;
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Film agency");
	Ui_Color(UI_WHITE);  Ui_Text(100, CONTENT_Y, "< "); Ui_Text(112, CONTENT_Y, g_CategoryName[s_Cat]); 
	Ui_Text((u8)(112 + 6 * 10 + 6), CONTENT_Y, ">");
	for (i = 0; i < 9; i++)
	{
		u8 y = CONTENT_Y + 12 + i * ROW_H;
		if (s_First + i >= n) break;
		idx = s_CatStart[s_Cat] + s_First + i;
		Ui_Color(s_First + i == s_Sel ? UI_YELLOW : (g_Game.owned[idx] ? UI_GREEN : UI_WHITE));
		Ui_Text(4, y, s_First + i == s_Sel ? ">" : " ");
		Ui_TextN(14, y, g_Movies[idx].title, 24);
		Ui_Color(UI_GRAY);
		if (g_Game.owned[idx]) Ui_Text(232, y, "own");
	}
	idx = s_CatStart[s_Cat] + s_Sel; m = &g_Movies[idx];
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
	Hint("L/R:genre OK:buy BACK:office");
}

static void Agency_Input(u8 ev)
{
	u8 n = CatCount(s_Cat);
	if ((ev & IN_DOWN) && s_Sel + 1 < n) { s_Sel++; if (s_Sel >= s_First + 9) s_First++; s_Dirty |= D_CON; }
	if ((ev & IN_UP) && s_Sel > 0)       { s_Sel--; if (s_Sel < s_First) s_First--; s_Dirty |= D_CON; }
	if (ev & IN_RIGHT) { s_Cat = (s_Cat + 1) % DB_NUM_CATEGORIES; s_Sel = s_First = 0; s_Dirty |= D_CON; }
	if (ev & IN_LEFT)  { s_Cat = (s_Cat + DB_NUM_CATEGORIES - 1) % DB_NUM_CATEGORIES; s_Sel = s_First = 0; s_Dirty |= D_CON; }
	if (ev & IN_BACK) { s_Screen = SCR_HUB; s_Dirty |= D_CON; return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_Buy(s_CatStart[s_Cat] + s_Sel);
		if (r == 0) Sim_Msg("Movie bought - now in your archive.");
		else if (r == 1) Sim_Msg("You already own this movie.");
		else Sim_Msg("Not enough money!");
		s_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

// ---------------------------------------------------------------- AGENCIA DE PUBLICIDADE
static void Draw_Ads(void)
{
	u8 i;
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Ad agency - today's offers");
	for (i = 0; i < NUM_OFFERS; i++)
	{
		u8 a = g_Game.offer[i], y = CONTENT_Y + 14 + i * 12;
		Ui_Color(i == s_Sel ? UI_YELLOW : (a == NONE ? UI_GRAY : UI_WHITE));
		Ui_Text(4, y, i == s_Sel ? ">" : " ");
		if (a == NONE) { Ui_Text(14, y, "(signed)"); continue; }
		Ui_TextN(14, y, g_Ads[a].title, 22);
	}
	if (g_Game.offer[s_Sel] != NONE)
	{
		const Ad* ad = &g_Ads[g_Game.offer[s_Sel]];
		Ui_Color(UI_WHITE);
		Ui_Text(4, 118, "Min audience"); Ui_Dec1(88, 118, ad->min_audience); Ui_Text(112, 118, "M viewers");
		Ui_Text(4, 129, "Spots"); Ui_Int(40, 129, ad->reps); Ui_Text(52, 129, "in"); Ui_Int(70, 129, ad->days); Ui_Text(82, 129, "day(s)");
		Ui_Color(UI_GREEN); Ui_Text(4, 140, "Pay"); Money(30, 140, ad->profit);
		Ui_Color(UI_RED);   Ui_Text(100, 140, "Penalty"); Money(148, 140, ad->penalty);
	}
	Hint("OK:sign contract BACK:office");
}

static void Ads_Input(u8 ev)
{
	if ((ev & IN_DOWN) && s_Sel + 1 < NUM_OFFERS) { s_Sel++; s_Dirty |= D_CON; }
	if ((ev & IN_UP) && s_Sel > 0) { s_Sel--; s_Dirty |= D_CON; }
	if (ev & IN_BACK) { s_Screen = SCR_HUB; s_Dirty |= D_CON; return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_SignAd(s_Sel);
		Sim_Msg(r == 0 ? "Contract signed - schedule it in the grid." : r == 1 ? "Max 4 contracts at a time." : "Already signed.");
		s_Dirty |= D_CON | D_MSG;
	}
}

// ---------------------------------------------------------------- AUDIENCIAS / IMAGE
static void Draw_Ratings(void)
{
	u8 s, st;
	char b[8];
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Ratings (million viewers)");
	for (st = 0; st < NUM_STATIONS; st++) { Ui_Color(s_StationCol[st]); Ui_Text(60 + st * 54, CONTENT_Y + 12, g_StationName[st]); }
	for (s = 0; s < NUM_SLOTS; s++)
	{
		u8 y = CONTENT_Y + 24 + s * 10;
		ClockStr(b, FIRST_SLOT_T + s * 60);
		Ui_Color(UI_GRAY); Ui_Text(8, y, b);
		for (st = 0; st < NUM_STATIONS; st++)
		{
			if (!g_Game.aud_done[s]) { Ui_Color(UI_GRAY); Ui_Text(64 + st * 54, y, "-"); continue; }
			Ui_Color(s_StationCol[st]); Ui_Dec1(60 + st * 54, y, g_Game.aud[st][s]);
			Ui_Color(UI_GRAY); Ui_Int(84 + st * 54, y, Sim_Quota(st, s)); 
		}
	}
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 100), "Image");
	for (st = 0; st < NUM_STATIONS; st++)
	{
		Ui_Color(s_StationCol[st]); Ui_Text(4, CONTENT_Y + 112 + st * 10, g_StationName[st]);
		Ui_Bar(50, CONTENT_Y + 113 + st * 10, 160, 6, g_Game.image[st], s_StationCol[st] == UI_RED ? COLOR_MEDIUM_RED : s_StationCol[st] == UI_GREEN ? COLOR_LIGHT_GREEN : COLOR_CYAN);
		Ui_Int(216, CONTENT_Y + 112 + st * 10, g_Game.image[st]);
	}
	Hint("BACK:office");
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
	if ((ev & IN_DOWN) && s_Menu + 1 < MENU_N) { s_Menu++; s_Dirty |= D_CON; }
	if ((ev & IN_UP) && s_Menu > 0) { s_Menu--; s_Dirty |= D_CON; }
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
	s_Screen = SCR_HUB;
	s_Dirty = D_HDR | D_CON | D_MSG;
}

void main()
{
	u8 ev, e, hz, fc = 0, fpm;
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
			if (e & (EV_SLOT | EV_DAY)) { if (s_Screen == SCR_RATINGS || s_Screen == SCR_ADS || s_Screen == SCR_HUB || s_Screen == SCR_GRID) { if (!s_Pick) s_Dirty |= D_CON; } }
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
		if (s_Dirty & D_MSG) Draw_Msg();
		s_Dirty = 0;
	}
}
