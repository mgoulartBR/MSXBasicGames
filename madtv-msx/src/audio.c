// Audio (PSG AY-3-8910) e tela de Opcoes. Segmento 12 (banco 2). Escrita direta nas portas A0h/A1h (padrao MSX).
// Canais: A = melodia, B = baixo, C = efeitos. Registro 7 = B8h (3 tons ligados, ruido desligado, bits de E/S como a BIOS);
// os registros 14/15 (joystick) nao sao tocados. Musica e efeitos originais deste projeto (nao sao as do Mad TV).
#include "app.h"

__sfr __at(0xA0) PSG_ADDR;
__sfr __at(0xA1) PSG_DATA;

u8 g_SndSfx, g_SndMus, g_SfxCount, g_MusicPos;       // g_SfxCount / g_MusicPos: ganchos de teste (smoke.sh)
static u8 s_Frame, s_SfxId, s_SfxStep, s_SfxLeft;

static void Wr(u8 r, u8 v) { PSG_ADDR = r; PSG_DATA = v; }
static void Tone(u8 ch, u16 period) { Wr((u8)(ch * 2), (u8)period); Wr((u8)(ch * 2 + 1), (u8)(period >> 8)); }

static const u16 k_Base[12] = { 1710, 1614, 1523, 1437, 1357, 1280, 1208, 1140, 1076, 1016, 959, 905 };   // C2..B2 (periodo = 111861/Hz)
static u16 NotePeriod(u8 n) { return (u16)(k_Base[n % 12] >> (n / 12)); }

#define STEP_FRAMES 9
#define MEL_LEN 32
static const u8 k_Lead[MEL_LEN] = { 28,31,36,31,28,31,36,40,  26,31,35,31,26,31,35,38,  24,28,33,28,24,28,33,36,  24,29,33,29,24,29,33,31 };
static const u8 k_Bass[8] = { 12, 7, 9, 5, 12, 7, 9, 5 };   // fundamental de cada compasso (C G Am F), repetido

// efeitos: ate 3 trechos {periodo, quadros}
static const u16 k_Sfx[5][3][2] = {
	{ { 250, 2 }, { 0, 0 }, { 0, 0 } },                     // 0 mover cursor
	{ { 380, 3 }, { 285, 4 }, { 0, 0 } },                   // 1 confirmar
	{ { 1100, 5 }, { 1300, 7 }, { 0, 0 } },                 // 2 erro
	{ { 320, 4 }, { 254, 4 }, { 214, 7 } },                 // 3 novo dia
	{ { 285, 3 }, { 214, 3 }, { 160, 6 } },                 // 4 conquista (casamento etc.)
};

void Audio_Init(void) __banked
{
	u8 r;
	g_SndSfx = 1; g_SndMus = 1; g_SfxCount = 0; g_MusicPos = 0; s_Frame = 0; s_SfxLeft = 0;
	for (r = 0; r < 7; r++) Wr(r, 0);
	Wr(7, 0xB8);
	Wr(8, 0); Wr(9, 0); Wr(10, 0);
}

void Audio_Sfx(u8 id) __banked
{
	if (!g_SndSfx) return;
	g_SfxCount++;
	s_SfxId = id; s_SfxStep = 0; s_SfxLeft = 0;
}

static u8 Has(const char* m, const char* w)
{
	for (; *m; m++) { const char *a = m, *b = w; while (*b && *a == *b) { a++; b++; } if (!*b) return 1; }
	return 0;
}

void Audio_Result(const char* msg) __banked      // resultado de uma acao do jogador: erro (grave) ou confirmacao
{
	static const char* const k_Bad[] = { "lready", "Not enough", "not fit", "none", "Keep it", "fond", "more", "Rival", "No more", "full", "first", "Nothing", "Closed", "locked", "today's", "limit", "Max", "Invalid", "Wrong", "Corrupt", "n't", "Time is" };
	u8 i;
	for (i = 0; i < sizeof(k_Bad) / sizeof(k_Bad[0]); i++) if (Has(msg, k_Bad[i])) { Audio_Sfx(2); return; }
	Audio_Sfx(1);
}

void Audio_Tick(void) __banked
{
	u8 step, sub, v;
	if (g_SndMus)
	{
		sub = s_Frame;
		if (sub == 0)
		{
			step = g_MusicPos;
			Tone(0, NotePeriod(k_Lead[step]));
			if ((step & 1) == 0) { Tone(1, NotePeriod((u8)(k_Bass[step >> 2] + ((step & 3) == 2 ? 7 : 0)))); }
		}
		v = (u8)(11 - sub); if (v < 3) v = 3;
		Wr(8, v);
		Wr(9, (g_MusicPos & 1) ? 0 : 6);
		if (++s_Frame >= STEP_FRAMES) { s_Frame = 0; g_MusicPos = (u8)((g_MusicPos + 1) % MEL_LEN); }
	}
	else { Wr(8, 0); Wr(9, 0); }
	if (g_SndSfx && s_SfxId < 5)
	{
		if (s_SfxLeft == 0)
		{
			const u16* seg = k_Sfx[s_SfxId][s_SfxStep];
			if (s_SfxStep >= 3 || seg[1] == 0) { Wr(10, 0); s_SfxId = 0xFF; }
			else { Tone(2, seg[0]); Wr(10, 11); s_SfxLeft = (u8)seg[1]; s_SfxStep++; }
		}
		if (s_SfxLeft) s_SfxLeft--;
	}
	else Wr(10, 0);
}

// ---------------------------------------------------------------- OPCOES (som e mouse)
static const char* const k_SpeedName[4] = { "slow", "normal", "fast", "very fast" };

static void OptBody(u8 i)
{
	u8 y = (u8)(CONTENT_Y + 14 + i * ROW_H);
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(g_Sel == i ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, g_Sel == i ? ">" : " ");
	if (i == 0) { Ui_Text(14, y, "Sound effects"); Ui_Color(g_SndSfx ? UI_GREEN : UI_GRAY); Ui_Text(160, y, g_SndSfx ? "on" : "off"); }
	else if (i == 1) { Ui_Text(14, y, "Music"); Ui_Color(g_SndMus ? UI_GREEN : UI_GRAY); Ui_Text(160, y, g_SndMus ? "on" : "off"); }
	else { Ui_Text(14, y, "Mouse speed"); Ui_Color(UI_CYAN); Ui_Text(160, y, k_SpeedName[g_PtrSpeed]); }
}

void Options_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Options");
	for (i = 0; i < 3; i++) OptBody(i);
	Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 54), "LEFT/RIGHT or OK change the value.");
	Ui_Text(4, (u8)(CONTENT_Y + 64), "Settings last until you switch off.");
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:change  BACK:office");
}

void Options_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel < 2) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) g_Dirty |= D_CON;
	if (ev & IN_BACK) { Goto(SCR_OFFICE); return; }
	if (ev & (IN_OK | IN_LEFT | IN_RIGHT))
	{
		if (g_Sel == 0) g_SndSfx ^= 1;
		else if (g_Sel == 1) g_SndMus ^= 1;
		else if (ev & IN_LEFT) g_PtrSpeed = (u8)((g_PtrSpeed + 3) & 3);
		else g_PtrSpeed = (u8)((g_PtrSpeed + 1) & 3);
		g_Dirty |= D_CON;
		if (g_SndSfx) Audio_Sfx(1);
	}
}

void Options_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(CONTENT_Y + 13), ROW_H, 3);
	(void)x;
	if (r == 0xFF) return;
	if (r != g_Sel) { g_Sel = r; g_Dirty |= D_CON; }
	if (btn) Options_Input(IN_OK);
}
