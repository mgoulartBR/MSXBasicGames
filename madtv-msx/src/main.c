// MadTV-MSX - nucleo fixo: laco principal, cabecalho, mensagens, hub e despacho para as telas (segmentos banked).
#include "app.h"


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
static u8  c_Speed = 255, c_Sym = 255;

void Header_Invalidate(void)
{
	u8 i;
	c_Day = 0xFFFF; c_T = 0xFFFF; c_Money = 0x7FFFFFFF; c_Speed = 255; c_Sym = 255;
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
		Ui_Begin(); Ui_Fill(118, 1, 52, 10, UI_BG);
		Ui_Color(g_Game.money < 0 ? UI_RED : UI_WHITE);
		Money(120, 2, g_Game.money);
		Ui_End(118, 1, 52, 10);
	}
	if (c_Sym != g_Game.sym[0])
	{
		c_Sym = g_Game.sym[0];                                // simpatia da Betty
		Ui_Begin(); Ui_Fill(172, 1, 28, 10, UI_BG);
		Ui_Color(UI_RED); Ui_Text(172, 2, "B"); Ui_Int(180, 2, g_Game.sym[0]);
		Ui_End(172, 1, 28, 10);
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

u8 HitRow(u8 y, u8 y0, u8 h, u8 n)
{
	u8 r;
	if (y < y0) return 0xFF;
	r = (u8)((u8)(y - y0) / h);
	return (r < n) ? r : 0xFF;
}

u8 SelVisible(u8 row, u8 n)
{
	u8 s = (u8)(g_First + row);
	if (s >= n) return 0;
	if (s != g_Sel) { u8 old = g_Sel; g_Sel = s; MarkRows((u8)(old - g_First), row); g_Dirty |= D_DET; }
	return 1;
}

void NavApply(u8 r)
{
	if (r == 1) g_Dirty |= D_ROWS | D_DET;
	else if (r == 2) g_Dirty |= D_LIST | D_DET;
}

// ---------------------------------------------------------------- telas especiais
const char* const k_MouseTxt[4] = { "Mouse: off (press M)", "Mouse: port 1 (joystick on port 2)", "Mouse: port 2", "Mouse: auto-detect (or press M)" };

#define Draw_Title Title_Draw
#define Draw_Over Over_Draw

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
	case SCR_HUB:    Building_Enter(); break;
	case SCR_OFFICE: Office_Enter(); break;
	case SCR_SHOP:   Shop_Enter(); break;
	case SCR_BETTY:  Betty_Enter(); break;
	}
}

void Goto(u8 scr)
{
	if (g_Screen == SCR_HUB && scr != SCR_HUB) Building_Leave();
	g_Screen = scr; g_Sel = 0; g_First = 0; g_Dirty |= D_CON;
	ScreenEnter(scr);
}

static void ScreenInput(u8 ev)
{
	switch (g_Screen)
	{
	case SCR_HUB:     Building_Input(ev); break;
	case SCR_OFFICE:  Office_Input(ev); break;
	case SCR_GRID:    Grid_Input(ev); break;
	case SCR_AGENCY:  Agency_Input(ev); break;
	case SCR_ADS:     Ads_Input(ev); break;
	case SCR_RATINGS: Ratings_Input(ev); break;
	case SCR_NEWS:    News_Input(ev); break;
	case SCR_ARCHIVE: Archive_Input(ev); break;
	case SCR_BOSS:    Boss_Input(ev); break;
	case SCR_SAVE:    Save_Input(ev); break;
	case SCR_SHOP:    Shop_Input(ev); break;
	case SCR_BETTY:   Betty_Input(ev); break;
	case SCR_PORTER:  Porter_Input(ev); break;
	case SCR_REALTOR: Realtor_Input(ev); break;
	case SCR_SCRIPTS: Scripts_Input(ev); break;
	case SCR_STUDIO:  Studio_Input(ev); break;
	case SCR_FUN: case SCR_SUN: Rival_Input(ev); break;
	}
}

static void ScreenDraw(void)
{
	switch (g_Screen)
	{
	case SCR_HUB:     Building_Draw(); break;
	case SCR_OFFICE:  Office_Draw(); break;
	case SCR_GRID:    Grid_Draw(); break;
	case SCR_AGENCY:  Agency_Draw(); break;
	case SCR_ADS:     Ads_Draw(); break;
	case SCR_RATINGS: Ratings_Draw(); break;
	case SCR_NEWS:    News_Draw(); break;
	case SCR_ARCHIVE: Archive_Draw(); break;
	case SCR_BOSS:    Boss_Draw(); break;
	case SCR_SAVE:    Save_Draw(); break;
	case SCR_SHOP:    Shop_Draw(); break;
	case SCR_BETTY:   Betty_Draw(); break;
	case SCR_SCRIPTS: Scripts_Draw(); break;
	case SCR_STUDIO:  Studio_Draw(); break;
	case SCR_REALTOR: Realtor_Draw(); break;
	case SCR_PORTER:  Porter_Draw(); break;
	case SCR_FUN: case SCR_SUN: Rival_Draw(); break;
	}
}

static void ScreenDyn(void)
{
	switch (g_Screen)
	{
	case SCR_HUB:     Building_Dyn(); break;
	case SCR_GRID:    Grid_Dyn(); break;
	case SCR_RATINGS: Ratings_Dyn(); break;
	case SCR_NEWS:    News_Dyn(); break;
	case SCR_PORTER:  Porter_Draw(); break;
	case SCR_FUN: case SCR_SUN: Rival_Draw(); break;
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
	case SCR_HUB:    Building_Row(r); break;
	case SCR_OFFICE: Office_Row(r); break;
	case SCR_GRID:   Grid_Row(r); break;
	case SCR_AGENCY: Agency_Row(r); break;
	case SCR_ADS:    Ads_Row(r); break;
	case SCR_NEWS:   News_Row(r); break;
	case SCR_ARCHIVE: Archive_Row(r); break;
	case SCR_BOSS:   Boss_Row(r); break;
	case SCR_SAVE:   Save_Row(r); break;
	case SCR_SHOP:   Shop_Row(r); break;
	case SCR_BETTY:  Betty_Row(r); break;
	}
}

static void ScreenDetail(void)
{
	switch (g_Screen)
	{
	case SCR_AGENCY: Agency_Detail(); break;
	case SCR_ADS:    Ads_Detail(); break;
	case SCR_NEWS:   News_Detail(); break;
	case SCR_HUB:    Building_Detail(); break;
	}
}

static void ScreenMouse(u8 x, u8 y, u8 btn)
{
	switch (g_Screen)
	{
	case SCR_HUB:     Building_Mouse(x, y, btn); break;
	case SCR_OFFICE:  Office_Mouse(x, y, btn); break;
	case SCR_GRID:    Grid_Mouse(x, y, btn); break;
	case SCR_AGENCY:  Agency_Mouse(x, y, btn); break;
	case SCR_ADS:     Ads_Mouse(x, y, btn); break;
	case SCR_NEWS:    News_Mouse(x, y, btn); break;
	case SCR_ARCHIVE: Archive_Mouse(x, y, btn); break;
	case SCR_BOSS:    Boss_Mouse(x, y, btn); break;
	case SCR_SAVE:    Save_Mouse(x, y, btn); break;
	case SCR_SHOP:    Shop_Mouse(x, y, btn); break;
	case SCR_BETTY:   Betty_Mouse(x, y, btn); break;
	case SCR_REALTOR: Realtor_Mouse(x, y, btn); break;
	case SCR_SCRIPTS: Scripts_Mouse(x, y, btn); break;
	case SCR_STUDIO:  Studio_Mouse(x, y, btn); break;
	}
}

static void StartGame(void)
{
	Sim_Init(*(volatile u16*)0xFC9E ^ 0x5A5A);   // semente: JIFFY do BIOS no momento do OK
	g_Speed = SPEED_1;
	Building_Reset();
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
	g_Dirty = 0; g_Sel = 0; g_First = 0; g_RowA = g_RowB = 0xFF; g_Speed = SPEED_1; g_Screen = SCR_TITLE; g_Diff = 1;
	Ui_Init();
	Draw_Title();

	for (;;)
	{
		Halt();
		ev = Input_Poll();
		if (g_InExtra && g_Screen != SCR_SAVE)                     // M: liga/desliga o mouse (na tela de codigo M e uma letra)
		{
			Pointer_Cycle();
			if (g_Screen == SCR_TITLE) Draw_Title();
			else { Sim_Msg(k_MouseTxt[g_PtrMode]); g_Dirty |= D_MSG; }
		}
		{
			u8 p = Pointer_Update();
			if (g_PtrFound) { g_PtrFound = 0; if (g_Screen == SCR_TITLE) Draw_Title(); else { Sim_Msg(k_MouseTxt[g_PtrMode]); g_Dirty |= D_MSG; } }
			if (p & PTR_RIGHT) ev |= IN_BACK;                      // botao direito = voltar ("dentro: esquerdo, fora: direito")
			if (p & PTR_LEFT) { if (g_Screen == SCR_TITLE && g_PtrY > 128 && g_PtrY < 142) ev |= IN_RIGHT; else if (g_Screen == SCR_TITLE || g_Screen == SCR_OVER) ev |= IN_OK; }   // clique na linha de dificuldade = trocar
			if (g_Screen != SCR_TITLE && g_Screen != SCR_OVER && (p & (PTR_MOVED | PTR_LEFT)))
			{
				if ((p & PTR_LEFT) && g_PtrY < 14 && g_PtrX >= 200) ev |= IN_SPEED;      // clicar na velocidade do cabecalho
				else ScreenMouse(g_PtrX, g_PtrY, (p & PTR_LEFT) ? 1 : 0);
			}
		}

		if (g_Screen == SCR_TITLE || g_Screen == SCR_OVER)
		{
			if (g_Screen == SCR_TITLE && (ev & (IN_LEFT | IN_RIGHT))) { g_Diff = (ev & IN_RIGHT) ? (u8)((g_Diff + 1) % NUM_DIFF) : (u8)((g_Diff + NUM_DIFF - 1) % NUM_DIFF); Draw_Title(); continue; }
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
		}

		if (g_Screen == SCR_HUB)                                // predio: movimento das figuras a cada frame
		{
			u8 r = Building_Frame();
			if (r != 0xFF) Goto(r);
		}
		ScreenInput(ev);
		if (g_Game.game_over) { g_Screen = SCR_OVER; Draw_Over(); continue; }

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
