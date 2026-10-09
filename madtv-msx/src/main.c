// MadTV-MSX - nucleo fixo: laco principal, cabecalho, mensagens, hub e despacho para as telas (segmentos banked).
#include "app.h"

#define VERSION_STR "0.4"

u8 g_Screen, g_Speed, g_Dirty, g_RowA, g_RowB, g_Sel, g_First;
u8 g_StationCol[NUM_STATIONS] = { UI_RED, UI_GREEN, UI_CYAN };
// Medicao de desempenho: maior tempo (jiffies = interrupcoes do VDP) gasto no desenho completo de cada tela. Lido pelos testes.
u8 g_DrawMax[SCR_OVER + 1];
#define JIFFY (*(volatile u16*)0xFC9E)

// ---------------------------------------------------------------- utilitarios
static void Pad2(char* d, u8 v) { d[0] = '0' + v / 10; d[1] = '0' + v % 10; }

void ClockStr(char* b, u16 t)     // t = minutos desde 17:00
{
	u16 m = 17 * 60 + t;
	m %= 24 * 60;
	Pad2(b, (u8)(m / 60)); b[2] = ':'; Pad2(b + 3, (u8)(m % 60)); b[5] = 0;
}

void Money(u8 x, u8 y, i32 k)     // k$
{
	Ui_Text(x, y, "$");
	x = Ui_Int(x + 6, y, (i16)k);
	Ui_Text(x, y, "k");
}

void MarkRows(u8 a, u8 b) { g_RowA = a; g_RowB = b; g_Dirty |= D_ROWS; }

// Cabecalho incremental: cada campo so e redesenhado se mudou (evita apagar/redesenhar tudo a cada minuto de jogo)
static u16 c_Day = 0xFFFF, c_T = 0xFFFF;
static i32 c_Money = 0x7FFFFFFF;
static u8  c_Image[NUM_STATIONS] = { 255, 255, 255 };
static u8  c_Speed = 255;

void Header_Invalidate(void)
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
	if (c_Speed != g_Speed)
	{
		c_Speed = g_Speed;
		Ui_Begin(); Ui_Fill(200, 1, 55, 10, UI_BG);
		Ui_Color(UI_GRAY);
		Ui_TextR(252, 2, g_Speed == SPEED_PAUSE ? "PAUSE" : g_Speed == SPEED_1 ? ">" : g_Speed == SPEED_2 ? ">>" : ">>>");
		Ui_End(200, 1, 55, 10);
	}
	for (st = 0; st < NUM_STATIONS; st++)
		if (c_Image[st] != g_Game.image[st])
		{
			c_Image[st] = g_Game.image[st];
			Ui_Begin(); Ui_Fill(4 + st * 84, 12, 80, 10, UI_BG);
			Ui_Color(g_StationCol[st]);
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

void Hint(const char* s)
{
	u16 saved = Ui_DirectBegin();      // dica: direto na tela (pode ser chamada dentro de um desenho em buffer)
	Ui_Fill(0, 190, 255, 10, UI_BG);
	Ui_Color(UI_GRAY);
	Ui_Text(4, 191, s);
	Ui_DirectEnd(saved);
}

void ClearContent(void) { Ui_Fill(0, CONTENT_Y - 2, 255, 160, UI_BG); }


// Navegacao vertical de lista com janela de `rows` linhas. Retorna 0 = nada, 1 = so linhas A/B mudaram, 2 = rolou.
u8 Nav(u8 ev, u8 n, u8 rows)
{
	u8 old = g_Sel, oldfirst = g_First;
	if ((ev & IN_DOWN) && g_Sel + 1 < n) { g_Sel++; if (g_Sel >= g_First + rows) g_First++; }
	if ((ev & IN_UP) && g_Sel > 0)       { g_Sel--; if (g_Sel < g_First) g_First--; }
	if (g_Sel == old) return 0;
	if (g_First != oldfirst) return 2;
	g_RowA = old - g_First; g_RowB = g_Sel - g_First;
	return 1;
}

void NavApply(u8 r)
{
	if (r == 1) g_Dirty |= D_ROWS | D_DET;
	else if (r == 2) g_Dirty |= D_LIST | D_DET;
}

// ---------------------------------------------------------------- HUB
static const char* const k_Menu[] = { "Programme grid", "Film agency", "Ad agency", "News room", "Archive (sell movies)", "Boss office (credit)", "Ratings & image", "Save / Load" };
static const u8 k_MenuScr[] = { SCR_GRID, SCR_AGENCY, SCR_ADS, SCR_NEWS, SCR_ARCHIVE, SCR_BOSS, SCR_RATINGS, SCR_SAVE };
#define MENU_N 8
static u8 s_Menu;

#define HY(n) ((u8)(CONTENT_Y + 98 + (n)))     // (u8) evita falso aviso de overflow do SDCC em constantes > 127
static void Draw_HubDyn_Body(void)
{
	u8 i, n = 0, c = 0, a = 0;
	Ui_Fill(0, HY(0), 255, 34, UI_BG);
	Ui_Color(UI_GRAY);
	for (i = 0; i < DB_NUM_MOVIES; i++) n += g_Game.owned[i];
	for (i = 0; i < MAX_CONTRACTS; i++) c += (g_Game.contract[i].ad != NONE);
	for (i = 0; i < DB_NUM_AGENCIES; i++) a += g_Game.news_sub[i];
	Ui_Text(4, HY(2), "Movies"); Ui_Int(40, HY(2), n);
	Ui_Text(70, HY(2), "Contracts"); Ui_Int(124, HY(2), c);
	Ui_Text(150, HY(2), "Agencies"); Ui_Int(198, HY(2), a);
	Ui_Color(g_Game.debt ? UI_RED : UI_GRAY);
	Ui_Text(4, HY(14), "Debt"); Money(34, HY(14), g_Game.debt);
	Ui_Color(UI_GRAY);
	Ui_Text(110, HY(14), "Betty"); Ui_Text(146, HY(14), "(not yet)");
	Ui_Text(4, HY(26), "Next newscast quality"); Ui_Int(136, HY(26), Sim_NewsQuality()); Ui_Text(154, HY(26), "%");
}

static void Draw_HubDyn(void)
{
	Ui_Begin();
	Draw_HubDyn_Body();
	Ui_End(0, HY(0), 255, 34);
}

static void DrawRow_Hub(u8 i)
{
	u8 y = CONTENT_Y + 14 + i * ROW_H;
	if (i >= MENU_N) return;
	Ui_Begin();
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(i == s_Menu ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, i == s_Menu ? ">" : " ");
	Ui_Text(16, y, k_Menu[i]);
	Ui_End(0, (u8)(y - 1), 255, ROW_H);
}

static void Draw_Hub(void)
{
	u8 i;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Office");
	for (i = 0; i < MENU_N; i++) DrawRow_Hub(i);
	Draw_HubDyn_Body();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:enter  TAB:speed  P:pause");
}

static void Hub_Input(u8 ev)
{
	u8 old = s_Menu;
	if ((ev & IN_DOWN) && s_Menu + 1 < MENU_N) s_Menu++;
	if ((ev & IN_UP) && s_Menu > 0) s_Menu--;
	if (s_Menu != old) MarkRows(old, s_Menu);
	if (ev & IN_OK) Goto(k_MenuScr[s_Menu]);
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
	Ui_Color(UI_YELLOW); Ui_Text(60, 176, "Press OK to start");
	Ui_Color(UI_CYAN);   Ui_Text(36, 188, "Esc: load a game from a save code");
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

// ---------------------------------------------------------------- despacho para as telas (todas __banked)
static void ScreenEnter(u8 scr)
{
	switch (scr)
	{
	case SCR_GRID:   Grid_Enter(); break;
	case SCR_AGENCY: Agency_Enter(); break;
	case SCR_ADS:    Ads_Enter(); break;
	case SCR_NEWS:   News_Enter(); break;
	case SCR_ARCHIVE: Archive_Enter(); break;
	case SCR_BOSS:   Boss_Enter(); break;
	case SCR_SAVE:   Save_Enter(); break;
	}
}

void Goto(u8 scr)
{
	g_Screen = scr; g_Sel = 0; g_First = 0; g_Dirty |= D_CON;
	ScreenEnter(scr);
}

static void ScreenInput(u8 ev)
{
	switch (g_Screen)
	{
	case SCR_HUB:     Hub_Input(ev); break;
	case SCR_GRID:    Grid_Input(ev); break;
	case SCR_AGENCY:  Agency_Input(ev); break;
	case SCR_ADS:     Ads_Input(ev); break;
	case SCR_RATINGS: Ratings_Input(ev); break;
	case SCR_NEWS:    News_Input(ev); break;
	case SCR_ARCHIVE: Archive_Input(ev); break;
	case SCR_BOSS:    Boss_Input(ev); break;
	case SCR_SAVE:    Save_Input(ev); break;
	}
}

static void ScreenDraw(void)
{
	switch (g_Screen)
	{
	case SCR_HUB:     Draw_Hub(); break;
	case SCR_GRID:    Grid_Draw(); break;
	case SCR_AGENCY:  Agency_Draw(); break;
	case SCR_ADS:     Ads_Draw(); break;
	case SCR_RATINGS: Ratings_Draw(); break;
	case SCR_NEWS:    News_Draw(); break;
	case SCR_ARCHIVE: Archive_Draw(); break;
	case SCR_BOSS:    Boss_Draw(); break;
	case SCR_SAVE:    Save_Draw(); break;
	}
}

static void ScreenDyn(void)
{
	switch (g_Screen)
	{
	case SCR_HUB:     Draw_HubDyn(); break;
	case SCR_GRID:    Grid_Dyn(); break;
	case SCR_RATINGS: Ratings_Dyn(); break;
	case SCR_NEWS:    News_Dyn(); break;
	case SCR_BOSS:    Boss_Draw(); break;
	}
}

static void ScreenList(void)
{
	switch (g_Screen)
	{
	case SCR_GRID:   Grid_List(); break;
	case SCR_AGENCY: Agency_List(); break;
	case SCR_ARCHIVE: Archive_List(); break;
	}
}

static void ScreenRow(u8 r)
{
	switch (g_Screen)
	{
	case SCR_HUB:    DrawRow_Hub(r); break;
	case SCR_GRID:   Grid_Row(r); break;
	case SCR_AGENCY: Agency_Row(r); break;
	case SCR_ADS:    Ads_Row(r); break;
	case SCR_NEWS:   News_Row(r); break;
	case SCR_ARCHIVE: Archive_Row(r); break;
	case SCR_BOSS:   Boss_Row(r); break;
	case SCR_SAVE:   Save_Row(r); break;
	}
}

static void ScreenDetail(void)
{
	switch (g_Screen)
	{
	case SCR_AGENCY: Agency_Detail(); break;
	case SCR_ADS:    Ads_Detail(); break;
	case SCR_NEWS:   News_Detail(); break;
	}
}

static void StartGame(void)
{
	Sim_Init(*(volatile u16*)0xFC9E ^ 0x5A5A);   // semente: JIFFY do BIOS no momento do OK
	g_Speed = SPEED_1; s_Menu = 0;
	Ui_Clear();
	Header_Invalidate();
	g_Screen = SCR_HUB;
	g_Dirty = D_HDR | D_CON | D_MSG;
}

void main()
{
	u8 ev, e, hz, fc = 0, fpm;
	// O crt0 do MSXgl NAO zera a RAM (BSS): em hardware real ela contem lixo. Todo estado e inicializado aqui.
	{ u8 k; for (k = 0; k <= SCR_OVER; k++) g_DrawMax[k] = 0; }
	g_Dirty = 0; g_Sel = 0; g_First = 0; g_RowA = g_RowB = 0xFF; s_Menu = 0; g_Speed = SPEED_1; g_Screen = SCR_TITLE;
	Ui_Init();
	Draw_Title();

	for (;;)
	{
		Halt();
		ev = Input_Poll();

		if (g_Screen == SCR_TITLE || g_Screen == SCR_OVER)
		{
			if (ev & IN_OK) StartGame();
			else if ((ev & IN_BACK) && g_Screen == SCR_TITLE) { StartGame(); Goto(SCR_SAVE); Save_EnterLoad(); }   // Esc no titulo: carregar codigo
			continue;
		}

		// velocidade (normalizada para 50/60 Hz): 1 min de jogo = 1/2, 1/5 ou 1/12 de segundo real
		hz = VDP_GetFrequency() ? 50 : 60;
		fpm = (g_Speed == SPEED_1) ? hz / 2 : (g_Speed == SPEED_2) ? hz / 5 : hz / 12;
		if (g_Screen == SCR_SAVE) ev &= (u8)~(IN_SPEED | IN_PAUSE);      // P/TAB sao letras do codigo na tela de salvar/carregar
		if (ev & IN_SPEED) { g_Speed = (g_Speed % (SPEED_COUNT - 1)) + 1; g_Dirty |= D_HDR; }
		if (ev & IN_PAUSE) { g_Speed = (g_Speed == SPEED_PAUSE) ? SPEED_1 : SPEED_PAUSE; g_Dirty |= D_HDR; }

		if (g_Speed != SPEED_PAUSE && g_Screen != SCR_SAVE && ++fc >= fpm)    // tempo parado na tela Salvar/Carregar
		{
			fc = 0;
			e = Sim_Tick();
			g_Dirty |= D_HDR;
			if (e & EV_MSG) g_Dirty |= D_MSG;
			if (e & (EV_SLOT | EV_DAY))
			{
				if (g_Screen == SCR_ADS && (e & EV_DAY)) g_Dirty |= D_CON;       // ofertas novas
				else g_Dirty |= D_DAT;
			}
			if (g_Game.game_over) { g_Screen = SCR_OVER; Draw_Over(); continue; }
		}

		ScreenInput(ev);

		if (g_Dirty & D_HDR) Draw_Header();
		if ((g_Dirty & D_DAT) && !(g_Dirty & D_CON)) ScreenDyn();
		if (g_Dirty & D_CON)
		{
			u16 t0 = JIFFY;
			u8 d;
			ScreenDraw();
			d = (u8)(JIFFY - t0);
			if (d > g_DrawMax[g_Screen]) g_DrawMax[g_Screen] = d;
		}
		else if (g_Dirty & (D_ROWS | D_LIST | D_DET))
		{
			// cursor/lista: so redesenha o que mudou (linhas A/B, lista inteira sem limpar a tela, ou detalhes)
			if (g_Dirty & D_LIST) ScreenList();
			else if (g_Dirty & D_ROWS)
			{
				ScreenRow(g_RowA);
				if (g_RowB != g_RowA) ScreenRow(g_RowB);
			}
			if (g_Dirty & D_DET) ScreenDetail();
		}
		if (g_Dirty & D_MSG) Draw_Msg();
		g_Dirty = 0;
		g_RowA = g_RowB = 0xFF;
	}
}
