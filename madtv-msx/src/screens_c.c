// Telas: Escritorio do Chefe (credito) e Salvar/Carregar (codigo). Compilado no segmento 8 (banco 2).
#include "app.h"

// ---------------------------------------------------------------- CHEFE (credito)
#define BORROW_STEP 500
#define BOSS_ROWS   3
static const char* const k_BossItems[BOSS_ROWS] = { "Borrow $500k", "Repay $500k", "Repay all debt" };

static void BossRowBody(u8 i)
{
	u8 y = CONTENT_Y + 98 + i * ROW_H;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, i == g_Sel ? ">" : " ");
	Ui_Text(14, y, k_BossItems[i]);
}

static void BossRow(u8 i)
{
	if (i >= BOSS_ROWS) return;
	Ui_Begin();
	BossRowBody(i);
	Ui_End(0, (u8)(CONTENT_Y + 98 + i * ROW_H - 1), 255, ROW_H);
}

void Boss_Enter(void) __banked { }

void Boss_Draw(void) __banked
{
	u8 i;
	i32 lim = Sim_CreditLimit();
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Mr. Raffer's office");
	Ui_Color(g_Game.image[0] >= 40 ? UI_GREEN : (g_Game.image[0] < 20 ? UI_RED : UI_WHITE));
	Ui_Text(4, CONTENT_Y + 16, g_Game.image[0] >= 40 ? "\"Not bad, kid. Keep the money coming.\"" :
	                           g_Game.image[0] < 20 ? "\"Your image is a DISGRACE! Raus hier!\"" : "\"Money, money, money. That's all I ask.\"");
	Ui_Color(UI_WHITE);
	Ui_Text(4, CONTENT_Y + 36, "Your cash");      Money(110, CONTENT_Y + 36, g_Game.money);
	Ui_Text(4, CONTENT_Y + 48, "Debt");           Ui_Color(g_Game.debt ? UI_RED : UI_WHITE); Money(110, CONTENT_Y + 48, g_Game.debt);
	Ui_Color(UI_WHITE);
	Ui_Text(4, CONTENT_Y + 60, "Credit limit");   Money(110, CONTENT_Y + 60, lim);
	Ui_Text(4, CONTENT_Y + 72, "Interest per day"); Ui_Int(110, CONTENT_Y + 72, INTEREST_PCT); Ui_Text(122, CONTENT_Y + 72, "%");
	Ui_Color(UI_GRAY); Ui_Text(4, CONTENT_Y + 84, "(limit grows with your station image)");
	for (i = 0; i < BOSS_ROWS; i++) BossRowBody(i);
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:choose  BACK:office");
}

void Boss_Row(u8 r) __banked { BossRow(r); }

void Boss_Input(u8 ev) __banked
{
	u8 old = g_Sel, r;
	if ((ev & IN_DOWN) && g_Sel + 1 < BOSS_ROWS) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) MarkRows(old, g_Sel);
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		if (g_Sel == 0)      { r = Sim_Borrow(BORROW_STEP); Sim_Msg(r ? "Mr. Raffer: That is above your credit limit." : "Mr. Raffer: Don't spend it all at once."); }
		else if (g_Sel == 1) { r = Sim_Repay(BORROW_STEP);  Sim_Msg(r == 0 ? "Debt reduced." : r == 1 ? "You don't have that much cash." : "You owe nothing."); }
		else                 { r = Sim_Repay(g_Game.debt);  Sim_Msg(r == 0 ? "Debt cleared. Mr. Raffer is almost smiling." : r == 1 ? "Not enough cash to clear it all." : "You owe nothing."); }
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

// ---------------------------------------------------------------- SALVAR / CARREGAR (codigo)
// Alfabeto do codigo (mesmo de Sim_SaveCode): 0-9 A-H J-N P-X (sem I, O, Y, Z)
static const char k_Alpha[] = "0123456789ABCDEFGHJKLMNPQRSTUVWX";
#define CODE_COLS  21
#define CODE_LINES ((SAVE_CHARS + CODE_COLS - 1) / CODE_COLS)

static u8 s_SaveMode;                 // 0 = menu, 1 = mostrando o codigo, 2 = digitando
char g_SaveCode[SAVE_CHARS + 1];     // global (nao static) para os testes lerem pelo mapa de simbolos
static u8 s_Pos;

static void SaveMenuRowBody(u8 i)
{
	u8 y = CONTENT_Y + 24 + i * ROW_H;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, i == g_Sel ? ">" : " ");
	Ui_Text(14, y, i == 0 ? "Show my save code" : "Enter a code (load game)");
}

static void CodeLineBody(u8 l)
{
	char buf[CODE_COLS + 1];
	u8 y = CONTENT_Y + 28 + l * 14, n = 0, i;
	Ui_Fill(0, y - 2, 255, 13, UI_BG);
	for (i = 0; i < CODE_COLS && l * CODE_COLS + i < SAVE_CHARS; i++) buf[n++] = g_SaveCode[l * CODE_COLS + i];
	buf[n] = 0;
	Ui_Color(UI_WHITE);
	Ui_Text(8, y, buf);
	if (s_SaveMode == 2 && s_Pos / CODE_COLS == l)     // caractere sob o cursor em destaque
	{
		char c[2];
		u8 col = s_Pos % CODE_COLS;
		c[0] = g_SaveCode[s_Pos]; c[1] = 0;
		Ui_Fill((u8)(8 + col * 6), y - 1, 6, 10, UI_PANEL);
		Ui_Color(UI_YELLOW); Ui_Text((u8)(8 + col * 6), y, c);
	}
}

static void CodeLine(u8 l)
{
	if (l >= CODE_LINES) return;
	Ui_Begin();
	CodeLineBody(l);
	Ui_End(0, (u8)(CONTENT_Y + 28 + l * 14 - 2), 255, 13);
}

void Save_Enter(void) __banked { s_SaveMode = 0; s_Pos = 0; }
void Save_EnterLoad(void) __banked { u8 i; s_SaveMode = 2; s_Pos = 0; for (i = 0; i < SAVE_CHARS; i++) g_SaveCode[i] = '0'; g_SaveCode[SAVE_CHARS] = 0; }

void Save_Draw(void) __banked
{
	u8 l;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Save / Load");
	if (s_SaveMode == 0)
	{
		SaveMenuRowBody(0); SaveMenuRowBody(1);
		Ui_Color(UI_GRAY);
		Ui_Text(4, CONTENT_Y + 60, "No battery in the cartridge: your game is");
		Ui_Text(4, CONTENT_Y + 70, "stored as a code. Write it down (or take a");
		Ui_Text(4, CONTENT_Y + 80, "photo) and type it back to continue later.");
		Ui_Text(4, CONTENT_Y + 96, "Not saved: film wear, news, rival plans.");
		Ui_Text(4, (u8)(CONTENT_Y + 106), "Time is stopped on this screen.");
	}
	else
	{
		Ui_Color(UI_GRAY);
		Ui_Text(96, CONTENT_Y, s_SaveMode == 1 ? "your code" : "type the code");
		for (l = 0; l < CODE_LINES; l++) CodeLineBody(l);
	}
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint(s_SaveMode == 0 ? "OK:choose  BACK:office" : s_SaveMode == 1 ? "BACK:menu" : "UP/DN:char L/R:move OK:load BACK:menu");
}

void Save_Row(u8 r) __banked
{
	if (s_SaveMode == 0)
	{
		if (r >= 2) return;
		Ui_Begin(); SaveMenuRowBody(r); Ui_End(0, (u8)(CONTENT_Y + 24 + r * ROW_H - 1), 255, ROW_H);
	}
	else CodeLine(r);
}

static void SetMode(u8 m) { s_SaveMode = m; g_Sel = 0; g_Dirty |= D_CON; }

void Save_Input(u8 ev) __banked
{
	u8 typed, old;
	if (s_SaveMode == 0)
	{
		old = g_Sel;
		if ((ev & IN_DOWN) && g_Sel < 1) g_Sel++;
		if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
		if (g_Sel != old) MarkRows(old, g_Sel);
		if (ev & IN_BACK) { Goto(SCR_OFFICE); return; }
		if (ev & IN_OK)
		{
			if (g_Sel == 0) { Sim_SaveCode(g_SaveCode); SetMode(1); }
			else Save_EnterLoad(), g_Dirty |= D_CON;
		}
		return;
	}
	if (ev & IN_BACK) { SetMode(0); return; }
	if (s_SaveMode == 1) return;
	// digitando
	old = s_Pos;
	if (ev & (IN_UP | IN_DOWN))                     // percorre o alfabeto
	{
		u8 k = 0;
		while (k_Alpha[k] != g_SaveCode[s_Pos] && k < 31) k++;
		k = (ev & IN_UP) ? (u8)((k + 1) & 31) : (u8)((k + 31) & 31);
		g_SaveCode[s_Pos] = k_Alpha[k];
		MarkRows(s_Pos / CODE_COLS, s_Pos / CODE_COLS);
	}
	if (ev & IN_RIGHT) s_Pos = (s_Pos + 1 >= SAVE_CHARS) ? 0 : s_Pos + 1;
	if (ev & IN_LEFT)  s_Pos = s_Pos ? s_Pos - 1 : SAVE_CHARS - 1;
	typed = Input_TypedChar();
	if (typed == 8) { s_Pos = s_Pos ? s_Pos - 1 : 0; g_SaveCode[s_Pos] = '0'; MarkRows(s_Pos / CODE_COLS, old / CODE_COLS); }
	else if (typed)
	{
		u8 k = 0;
		if (typed == 'O') typed = '0';
		if (typed == 'I') typed = '1';
		while (k < 32 && k_Alpha[k] != typed) k++;
		if (k < 32) { g_SaveCode[s_Pos] = typed; s_Pos = (s_Pos + 1 >= SAVE_CHARS) ? 0 : s_Pos + 1; MarkRows(old / CODE_COLS, s_Pos / CODE_COLS); }
	}
	if (s_Pos != old) MarkRows(old / CODE_COLS, s_Pos / CODE_COLS);
	if (ev & IN_OK)
	{
		u8 r = Sim_LoadCode(g_SaveCode);
		if (r == 0) { Sim_Msg("Game loaded."); Header_Invalidate(); g_Dirty |= D_HDR | D_MSG; Goto(SCR_HUB); }
		else { Sim_Msg(r == 2 ? "Wrong code: checksum mismatch." : r == 1 ? "Invalid code." : "Corrupt save data."); g_Dirty |= D_MSG; }
	}
}

// ---------------------------------------------------------------- mouse
void Boss_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(CONTENT_Y + 97), ROW_H, BOSS_ROWS);
	(void)x;
	if (r == 0xFF) return;
	if (r != g_Sel) { u8 old = g_Sel; g_Sel = r; MarkRows(old, r); }
	if (btn) Boss_Input(IN_OK);
}

void Save_Mouse(u8 x, u8 y, u8 btn) __banked
{
	if (s_SaveMode == 0)
	{
		u8 r = HitRow(y, (u8)(CONTENT_Y + 23), ROW_H, 2);
		if (r == 0xFF) return;
		if (r != g_Sel) { u8 old = g_Sel; g_Sel = r; MarkRows(old, r); }
		if (btn) Save_Input(IN_OK);
	}
	else if (s_SaveMode == 2 && btn && x >= 8)                         // clique num caractere do codigo move o cursor de edicao
	{
		u8 l = HitRow(y, (u8)(CONTENT_Y + 26), 14, CODE_LINES), col = (u8)((u8)(x - 8) / 6), old = s_Pos;
		if (l == 0xFF || col >= CODE_COLS || l * CODE_COLS + col >= SAVE_CHARS) return;
		s_Pos = (u8)(l * CODE_COLS + col);
		MarkRows(old / CODE_COLS, s_Pos / CODE_COLS);
	}
}

// ---------------------------------------------------------------- titulo e fim de jogo (fora do nucleo fixo p/ poupar espaco)
void Title_Draw(void) __banked
{
	Ui_Clear();
	Ui_Color(UI_YELLOW); Ui_Text(70, 50, "M A D   T V");
	Ui_Color(UI_WHITE);  Ui_Text(52, 66, "MSX2 port - version " VERSION_STR);
	Ui_Color(UI_GRAY);
	Ui_Text(14, 100, "Run the station: buy movies, sign");
	Ui_Text(14, 110, "ad contracts, fill the programme grid");
	Ui_Text(14, 120, "and beat FunTV and SunTV in the ratings.");
	Ui_Fill(0, 131, 255, 10, UI_BG);
	Ui_Color(UI_YELLOW); Ui_Text(34, 132, g_Diff == 0 ? "< Difficulty: Easy >" : g_Diff == 1 ? "< Difficulty: Normal >" : "< Difficulty: Hard >");
	Ui_Color(UI_GREEN);  Ui_Text(14, 146, "Arrows/joystick: move   OK(Enter/Space)");
	Ui_Text(14, 156, "BACK(Esc): back   TAB: speed   P: pause");
	Ui_Color(UI_YELLOW); Ui_Text(60, 176, "Press OK to start");
	Ui_Color(UI_CYAN);   Ui_Text(36, 188, "Esc: load a game from a save code");
	Ui_Fill(0, 166, 255, 10, UI_BG);
	Ui_Color(g_PtrMode == 3 ? UI_CYAN : g_PtrMode ? UI_GREEN : UI_GRAY); Ui_Text(14, 167, k_MouseTxt[g_PtrMode]);
	Ui_Color(UI_GRAY);   Ui_Text(4, 202, "Data: TVTower (altered for MSX)");
}

void Over_Draw(void) __banked
{
	Ui_Clear();
	if (g_Game.won) { Ui_Color(UI_GREEN); Ui_Text(60, 70, "BETTY SAID YES!"); Ui_Color(UI_WHITE); Ui_Text(28, 90, "You are the king of television."); }
	else
	{
		Ui_Color(UI_RED);   Ui_Text(84, 70, "BANKRUPT!");
		Ui_Color(UI_WHITE); Ui_Text(40, 90, g_Game.alive[0] ? "Mr. Raffer shows you the door." : "Nobody watches your station.");
	}
	Ui_Color(UI_GRAY);  Ui_Text(60, 110, "Survived "); Ui_Int(114, 110, g_Game.day); Ui_Text(132, 110, "day(s)");
	Ui_Color(UI_YELLOW); Ui_Text(60, 150, "Press OK to restart");
}

