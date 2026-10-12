// Telas: Escritorio do Chefe (credito) e Salvar/Carregar (codigo). Compilado no segmento 8 (banco 2).
#include "app.h"

// ---------------------------------------------------------------- CHEFE (credito)
#define BORROW_STEP 500
#define BOSS_ROWS   3
static const char* const k_BossItems[BOSS_ROWS] = { "Borrow $500k", "Repay $500k", "Repay all debt" };

#if ART_SCENES
#define BOSS_RY(i)  ((u8)(CONTENT_Y + 72 + (i) * ROW_H))    // linhas do menu dentro do painel cinza (a direita da cena)
static void BossRowBody(u8 i)
{
	u8 y = BOSS_RY(i), sel = (g_Sel == i);
	Ui_Fill(114, y - 1, 136, ROW_H, 14);
	if (sel) Chrome_Box(114, y - 1, 136, ROW_H, 5, 0);
	Ui_Color(UI_BLACK);
	Ui_Text(120, y, k_BossItems[i]);
}
#else
#define BOSS_RY(i)  ((u8)(CONTENT_Y + 98 + (i) * ROW_H))
static void BossRowBody(u8 i)
{
	u8 y = BOSS_RY(i);
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, i == g_Sel ? ">" : " ");
	Ui_Text(14, y, k_BossItems[i]);
}
#endif

static void BossRow(u8 i)
{
	if (i >= BOSS_ROWS) return;
	Ui_Begin();
	BossRowBody(i);
#if ART_SCENES
	Ui_End(114, (u8)(BOSS_RY(i) - 1), 136, ROW_H);
#else
	Ui_End(0, (u8)(BOSS_RY(i) - 1), 255, ROW_H);
#endif
}

void Boss_Enter(void) __banked { }

void Boss_Draw(void) __banked
{
	u8 i;
	i32 lim = Sim_CreditLimit();
	Ui_Begin();
#if ART_SCENES
	Scene_Draw(SCN_BOSS, 0, CONTENT_Y - 2);
	{                                                     // painel inferior: o premio Sammy (melhor telejornal)
		u8 left = (u8)((SAMMY_DAYS - g_Game.day % SAMMY_DAYS) % SAMMY_DAYS), x;
		Chrome_Box(0, 138, 256, 48, 14, 1);
		Ui_Color(UI_BLACK);
		Ui_Text(8, 143, "Sammy - best newscast of the week");
		if (left) { x = Ui_Int(8, 156, left); Ui_Text(x, 156, left == 1 ? " day to go." : " days to go."); }
		else Ui_Text(8, 156, "Awarded tonight!");
		Ui_Text(8, 168, "Your newscast"); x = Ui_Int(92, 168, Sim_NewsQuality()); Ui_Text(x, 168, "%"); Ui_Text(150, 168, "prize $300k");
	}
	Chrome_Box(112, CONTENT_Y, 140, 106, 14, 1);
	Ui_Color(UI_BLACK);
	if (g_Game.day % SAMMY_DAYS >= SAMMY_DAYS - 1 || g_Game.day % SAMMY_DAYS == 0)      // dia do Sammy (ou a vespera): a fala do chefe do original
	{
		Ui_Text(120, CONTENT_Y + 3, "\"The Sammy for best");
		Ui_Text(120, CONTENT_Y + 12, g_Game.day % SAMMY_DAYS ? "news comes tomorrow." : "news is tonight!");
		Ui_Text(120, CONTENT_Y + 21, "I want US to get it!\"");
	}
	else if (g_Game.image[0] >= 40) { Ui_Text(120, CONTENT_Y + 6, "\"Not bad, kid. Keep"); Ui_Text(120, CONTENT_Y + 16, "the money coming.\""); }
	else if (g_Game.image[0] < 20) { Ui_Text(120, CONTENT_Y + 6, "\"Your image is a"); Ui_Text(120, CONTENT_Y + 16, "DISGRACE! Raus hier!\""); }
	else { Ui_Text(120, CONTENT_Y + 6, "\"Money, money, money."); Ui_Text(120, CONTENT_Y + 16, "That's all I ask.\""); }
	Ui_Text(120, CONTENT_Y + 32, "Your cash");        Money(184, CONTENT_Y + 32, g_Game.money);
	Ui_Text(120, CONTENT_Y + 42, "Debt");             Ui_Color(g_Game.debt ? UI_RED : UI_BLACK); Money(184, CONTENT_Y + 42, g_Game.debt);
	Ui_Color(UI_BLACK);
	Ui_Text(120, CONTENT_Y + 52, "Credit limit");     Money(196, CONTENT_Y + 52, lim);
	Ui_Text(120, CONTENT_Y + 62, "Interest/day");     Ui_Int(196, CONTENT_Y + 62, INTEREST_PCT); Ui_Text(208, CONTENT_Y + 62, "%");
	for (i = 0; i < BOSS_ROWS; i++) BossRowBody(i);
	Ui_End(0, CONTENT_Y - 2, 0, 160);
	Hint("OK:choose  BACK:office");
	return;
#endif
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
	u8 r = HitRow(y, (u8)(BOSS_RY(0) - 1), ROW_H, BOSS_ROWS);
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
// geometria dos botoes do titulo (estilo da tela "Select a player" do original)
#define TB_ST_Y    106                         // botoes da emissora (nova na 2.8)
#define TB_DIFF_Y  124
#define TB_ACT_Y   148

static void Btn(u8 x, u8 y, u8 w, u8 face, const char* t, u8 tx)
{
	Chrome_Box(x, y, w, 14, face, 1);
	Ui_Color(UI_BLACK); Ui_Text((u8)(x + tx), (u8)(y + 3), t);
}

void Title_Opts(void) __banked                   // so os botoes de dificuldade e a linha do mouse (atomico, sem redesenhar o titulo todo)
{
	static const char* const k_D[3] = { "Easy", "Normal", "Hard" };
	u8 i;
	static const char* const k_S[3] = { "MadTV", "FunTV", "SunTV" };
	static const char* const k_Perk[3] = { "Balanced start.", "+500k cash, Image -5.", "Respected: Image +8, -500k." };
	Ui_Begin();
	Ui_Fill(12, TB_ST_Y - 2, 232, 18, 14);
	for (i = 0; i < 3; i++) Btn((u8)(16 + i * 80), TB_ST_Y, 64, g_Station == i ? 10 : 14, k_S[i], 14);
	Ui_Fill(12, TB_DIFF_Y - 2, 232, 18, 14);
	for (i = 0; i < 3; i++) Btn((u8)(16 + i * 80), TB_DIFF_Y, 64, g_Diff == i ? 10 : 14, k_D[i], i == 1 ? 14 : 20);
	Ui_Fill(12, 139, 232, 8, 14);
	Ui_Color(UI_BLACK); Ui_Text(16, 139, k_Perk[g_Station]);
	Ui_Fill(12, 168, 232, 10, 14);
	Ui_Color(UI_BLACK); Ui_Text(16, 169, k_MouseTxt[g_PtrMode]);
	Ui_End(12, TB_ST_Y - 2, 232, 74);
}

u8 Title_Click(u8 x, u8 y) __banked              // 0 nada, 1 novo jogo, 2 carregar; os botoes de dificuldade mudam na hora
{
	u8 i;
	if (y >= TB_ACT_Y && y < TB_ACT_Y + 14) { if (x >= 16 && x < 120) return 1; if (x >= 136 && x < 240) return 2; }
	if (y >= TB_ST_Y && y < TB_ST_Y + 14)
		for (i = 0; i < 3; i++) if (x >= 16 + i * 80 && x < 80 + i * 80) { if (g_Station != i) { g_Station = i; Title_Opts(); } break; }
	if (y >= TB_DIFF_Y && y < TB_DIFF_Y + 14)
		for (i = 0; i < 3; i++) if (x >= 16 + i * 80 && x < 80 + i * 80) { if (g_Diff != i) { g_Diff = i; Title_Opts(); } break; }
	return 0;
}

void Splash_Draw(void) __banked
{
	Ui_Begin();
#if ART_SCENES
	Ui_Fill(0, 0, 0, 212, SPLASH_BG);
	Scene_Draw(SCN_TITLE, 0, 8);
	Ui_Color(UI_GRAY);  Ui_Text(75, 139, "MSX port by");
	Ui_Color(UI_WHITE); Ui_Text(63, 150, "BigFive Studios");
#else
	Ui_Clear();
	Ui_Color(UI_RED);    Ui_Text(100, 70, "MAD TV");
	Ui_Color(UI_WHITE);  Ui_Text(64, 90, "MSX2 port - open source");
	Ui_Color(UI_GRAY);   Ui_Text(28, 110, "Original game (c) 1991 Rainbow Arts");
	Ui_Color(UI_GRAY);   Ui_Text(28, 120, "Data: TVTower (altered for MSX)");
#endif
	Ui_End(0, 0, 256, 212);
}

void Title_Draw(void) __banked
{
	Ui_Begin();                                  // tudo composto no buffer e copiado de uma vez: sem piscar
	Ui_Fill(0, 0, 256, 212, 12);                 // moldura de madeira
	Ui_Fill(0, 0, 256, 2, 6); Ui_Fill(0, 210, 256, 2, 6);
	Chrome_Box(6, 6, 244, 200, 14, 1);           // painel
	Chrome_Box(12, 14, 232, 18, 5, 0);           // titulo (campo azul)
	Ui_Color(UI_BLACK); Ui_Text(70, 19, "M A D   T V");
	Ui_Text(74, 38, "MSX2 port - v" VERSION_STR);
	Ui_Text(16, 48, "Run the station: buy movies, sign");
	Ui_Text(16, 58, "ad contracts, fill the programme");
	Ui_Text(16, 68, "grid and beat the rival stations in");
	Ui_Text(16, 78, "the ratings.");
	Ui_Text(16, 92, "Choose your station and difficulty:");
	Title_Opts();
	Btn(16, TB_ACT_Y, 104, 8, "New game (OK)", 8);
	Btn(136, TB_ACT_Y, 104, 8, "Load game (Esc)", 4);
	Ui_Color(UI_BLACK);
	Ui_Text(16, 181, "Arrows: move   OK: Enter/Space");
	Ui_Text(16, 189, "TAB: speed  P: pause  M: mouse");
	Ui_Text(16, 197, "Data: TVTower (altered for MSX)");
	Ui_End(0, 0, 256, 212);
}

#if ART_SCENES
void Over_Draw(void) __banked              // final: cena a esquerda (Betty = casamento, estudio = magnata, chefe = falencia) + painel com o resumo
{
	u8 x;
	Ui_Begin();
	Ui_Clear();
	Ui_Fill(0, 20, 0, 160, 12);
	if (g_Game.won) Scene_Draw(g_Game.won == 1 ? SCN_BETTY : SCN_NEWS, 8, 28);
	else Scene_Draw(SCN_BOSS, 0, 28);                  // cena do chefe tem 256 px: o painel cobre o resto
	Chrome_Box(132, 28, 116, 111, 14, 1);
	Ui_Color(UI_BLACK);
	if (g_Game.won == 1) { Ui_Text(142, 36, "BETTY SAID YES!"); Ui_Text(138, 54, "You are the king"); Ui_Text(138, 64, "of television."); }
	else if (g_Game.won == 2) { Ui_Text(142, 36, "TV TYCOON!"); Ui_Text(138, 54, "Both rivals are"); Ui_Text(138, 64, "off the air and"); Ui_Text(138, 74, "you own the town."); }
	else
	{
		Ui_Text(142, 36, "BANKRUPT!");
		if (g_Game.alive[0]) { Ui_Text(138, 54, "Mr. Raffer shows"); Ui_Text(138, 64, "you the door."); }
		else { Ui_Text(138, 54, "Nobody watches"); Ui_Text(138, 64, "your station."); }
	}
	Ui_Text(138, 96, "Days"); Ui_Int(180, 96, g_Game.day);
	Ui_Text(138, 108, "Image"); Ui_Int(180, 108, g_Game.image[0]);
	Ui_Text(138, 120, "Cash"); Money(180, 120, g_Game.money);
	Chrome_Box(8, 144, 240, 36, 14, 1);
	Ui_Color(UI_BLACK);
	Ui_Text(16, 150, "Coverage level"); x = Ui_Int(112, 150, g_Game.tower); Ui_Text(x, 150, "/"); Ui_Int((u8)(x + 6), 150, TOWER_MAX);
	Ui_Text(16, 164, "Press OK to restart");
	Ui_End(0, 0, 256, 212);
}
#else
void Over_Draw(void) __banked
{
	Ui_Begin();
	Ui_Clear();
	Ui_Fill(0, 40, 256, 130, 12); Chrome_Box(24, 48, 208, 110, 14, 1);
	if (g_Game.won) { Ui_Color(UI_BLACK); Ui_Text(60, 70, "BETTY SAID YES!"); Ui_Color(UI_BLACK); Ui_Text(28, 90, "You are the king of television."); }
	else
	{
		Ui_Color(UI_BLACK);   Ui_Text(84, 70, "BANKRUPT!");
		Ui_Color(UI_BLACK); Ui_Text(40, 90, g_Game.alive[0] ? "Mr. Raffer shows you the door." : "Nobody watches your station.");
	}
	Ui_Color(UI_BLACK);  Ui_Text(60, 110, "Survived "); Ui_Int(114, 110, g_Game.day); Ui_Text(132, 110, "day(s)");
	Ui_Color(UI_BLACK); Ui_Text(60, 150, "Press OK to restart");
	Ui_End(0, 0, 256, 212);
}
#endif

