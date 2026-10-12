// Audio (PSG AY-3-8910) e tela de Opcoes. Segmento 12 (banco 2). Escrita direta nas portas A0h/A1h (padrao MSX).
// Canais: A = melodia, B = baixo, C = efeitos. Registro 7 = B8h (3 tons ligados, ruido desligado, bits de E/S como a BIOS);
// os registros 14/15 (joystick) nao sao tocados. Musica e efeitos originais deste projeto (nao sao as do Mad TV).
#include "app.h"

__sfr __at(0xA0) PSG_ADDR;
__sfr __at(0xA1) PSG_DATA;

u8 g_SndSfx, g_SndMus, g_SfxCount, g_MusicPos;       // g_SfxCount / g_MusicPos: ganchos de teste (smoke.sh)
static u8 s_SfxId, s_SfxStep, s_SfxLeft;

static void Wr(u8 r, u8 v) { PSG_ADDR = r; PSG_DATA = v; }
static void Tone(u8 ch, u16 period) { Wr((u8)(ch * 2), (u8)period); Wr((u8)(ch * 2 + 1), (u8)(period >> 8)); }

static const u16 k_Base[12] = { 1710, 1614, 1523, 1437, 1357, 1280, 1208, 1140, 1076, 1016, 959, 905 };   // C2..B2 (periodo PSG = 111861/Hz)
static u16 NotePeriod(u8 n) { return (u16)(k_Base[n % 12] >> (n / 12)); }

// ---- composicao original (16 compassos, colcheia = 1 passo; A = 1-8, B = 9-16; Cmaj7 Am7 Dm7 G7 ... F Em A7 Dm G7). n = semitons acima de C2.
static const u8 k_Lead[128] = { /* 253 = sustenta, 255 = pausa */
	28, 253, 253, 31, 36, 253, 35, 253, 33, 253, 253, 31, 28, 253, 255, 255,
	29, 253, 33, 253, 38, 253, 253, 36, 35, 253, 253, 38, 31, 253, 255, 255,
	28, 253, 31, 253, 36, 253, 40, 253, 38, 253, 253, 36, 33, 253, 31, 253,
	29, 253, 33, 253, 31, 253, 35, 253, 36, 253, 253, 253, 253, 253, 255, 255,
	33, 253, 253, 36, 41, 253, 40, 253, 31, 253, 253, 35, 40, 253, 38, 253,
	29, 253, 253, 33, 38, 253, 36, 253, 35, 253, 38, 253, 43, 253, 41, 253,
	40, 253, 253, 36, 33, 253, 255, 255, 31, 253, 35, 253, 37, 253, 40, 253,
	38, 253, 253, 41, 33, 253, 255, 255, 35, 253, 31, 253, 38, 253, 255, 255
};
static const u8 k_Root[32] = { 12, 12, 9, 9, 14, 14, 7, 7, 12, 12, 9, 9, 14, 7, 12, 12, 5, 5, 16, 16, 14, 14, 7, 7, 5, 5, 16, 9, 14, 14, 7, 7 };
static const u8 k_Comp[32] = { 23, 23, 19, 19, 24, 24, 17, 17, 23, 23, 19, 19, 24, 17, 23, 23, 21, 21, 19, 19, 24, 24, 17, 17, 21, 21, 19, 21, 24, 24, 17, 17 };
static const u8 k_PatA[4] = { 0, 7, 12, 7 }, k_PatB[4] = { 0, 12, 7, 12 };   // baixo: passos pares do compasso (fundamental/quinta/oitava)

// ---- chip MSX-MUSIC (YM2413/OPLL), se existir: musica em FM nos canais 1-3; senao o PSG toca a musica. Efeitos sempre no PSG (canal C).
__sfr __at(0x7C) FM_ADDR;
__sfr __at(0x7D) FM_DATA;
u8 g_MusicType;                                      // 0 = PSG, 1 = FM (gancho de teste)
static const u16 k_FmF[12] = { 345, 366, 388, 411, 435, 461, 488, 259, 274, 290, 308, 326 };   // F-number (C..B, bloco ajustado)

static void FmWr(u8 r, u8 v)
{
	FM_ADDR = r;
	__asm
		nop
		nop
		nop
		nop
	__endasm;
	FM_DATA = v;
	__asm
		ld b, #8
	00001$:
		djnz 00001$
	__endasm;
}

static void FmNote(u8 ch, u8 n, u8 on)
{
	u8 s = n % 12, blk = (u8)(n / 12 + 1 + (s >= 7 ? 1 : 0));
	u16 f = k_FmF[s];
	FmWr((u8)(0x20 + ch), 0);                              // key off (retrigger)
	if (!on) return;
	FmWr((u8)(0x10 + ch), (u8)f);
	FmWr((u8)(0x20 + ch), (u8)(0x10 | (blk << 1) | (f >> 8)));
}

static u8 DetectFm(void)                                  // procura "APRLOPLL" em 4018h de algum slot (assinatura da BIOS MSX-MUSIC)
{
	static const char k_Sig[9] = "APRLOPLL";
	u8 p, s, i, id;
	for (p = 0; p < 4; p++)
	{
		u8 sec = (*(volatile u8*)(0xFCC1 + p) & 0x80) ? 4 : 1;
		for (s = 0; s < sec; s++)
		{
			id = (u8)(p | (sec == 4 ? (0x80 | (s << 2)) : 0));
			for (i = 0; i < 8; i++) if (BIOS_InterSlotRead(id, (u16)(0x4018 + i)) != (u8)k_Sig[i]) break;
			if (i == 8) { BIOS_InterSlotWrite(id, 0x7FF6, 1); return 1; }     // habilita E/S (FM-PAC); no MSX-MUSIC interno e inofensivo
		}
	}
	return 0;
}

static u8 s_Step, s_Sub, s_LAge, s_BAge, s_CAge, s_LOn, s_BOn, s_COn;

void Audio_Init(void) __banked
{
	u8 r;
	g_SndSfx = 1; g_SndMus = 1; g_SfxCount = 0; g_MusicPos = 0; s_Sub = 0; s_SfxLeft = 0; s_SfxId = 0xFF;
	s_Step = VDP_GetFrequency() ? 15 : 18;                  // colcheia = ~0,3 s (96 BPM) nos dois sistemas de video
	s_LOn = s_BOn = s_COn = 0; s_LAge = s_BAge = s_CAge = 0;
	for (r = 0; r < 7; r++) Wr(r, 0);
	Wr(7, 0xB8);
	Wr(8, 0); Wr(9, 0); Wr(10, 0);
	g_MusicType = DetectFm();
	if (g_MusicType)
	{
		for (r = 0; r < 3; r++) FmWr((u8)(0x20 + r), 0);
		FmWr(0x30, (u8)((4 << 4) | 3));                    // canal 1: melodia = Flute
		FmWr(0x31, (u8)((14 << 4) | 2));                   // canal 2: baixo = Acoustic bass
		FmWr(0x32, (u8)((2 << 4) | 6));                    // canal 3: acompanhamento = Guitar
	}
}

// efeitos: ate 3 trechos {periodo, quadros}
static const u16 k_Sfx[5][3][2] = {
	{ { 250, 2 }, { 0, 0 }, { 0, 0 } },                     // 0 mover cursor
	{ { 380, 3 }, { 285, 4 }, { 0, 0 } },                   // 1 confirmar
	{ { 1100, 5 }, { 1300, 7 }, { 0, 0 } },                 // 2 erro
	{ { 320, 4 }, { 254, 4 }, { 214, 7 } },                 // 3 novo dia
	{ { 285, 3 }, { 214, 3 }, { 160, 6 } },                 // 4 conquista (casamento etc.)
};

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

static void MusicOff(void)
{
	Wr(8, 0); Wr(9, 0);
	if (g_MusicType) { FmWr(0x20, 0); FmWr(0x21, 0); FmWr(0x22, 0); }
	s_LOn = s_BOn = s_COn = 0;
}

static void MusicStep(void)
{
	u8 pos = g_MusicPos, st = (u8)(pos & 7), v = k_Lead[pos];
	if (v != 253)                                            // melodia: nova nota ou pausa
	{
		if (v == 255) { s_LOn = 0; if (g_MusicType) FmNote(0, 0, 0); }
		else { s_LOn = 1; s_LAge = 0; if (g_MusicType) FmNote(0, v, 1); else { Wr(0, (u8)NotePeriod(v)); Wr(1, (u8)(NotePeriod(v) >> 8)); } }
	}
	if (!(st & 1))                                           // baixo nos passos pares
	{
		u8 n = (u8)(k_Root[pos >> 2] + (((pos >> 3) & 1) ? k_PatB : k_PatA)[st >> 1]);
		s_BOn = 1; s_BAge = 0;
		if (g_MusicType) FmNote(1, n, 1); else { Wr(2, (u8)NotePeriod(n)); Wr(3, (u8)(NotePeriod(n) >> 8)); }
	}
	if (st == 2 || st == 5)                                  // acorde sincopado (terca/setima)
	{
		u8 n = k_Comp[pos >> 2];
		s_COn = 1; s_CAge = 0;
		if (g_MusicType) FmNote(2, n, 1); else if (s_SfxId >= 5) { Wr(4, (u8)NotePeriod(n)); Wr(5, (u8)(NotePeriod(n) >> 8)); }
	}
}

void Audio_Tick(void) __banked
{
	if (g_SndMus)
	{
		if (s_Sub == 0) MusicStep();
		if (!g_MusicType)                                    // envelopes do PSG (a cada quadro): melodia sustenta e decai, baixo "beliscado", acorde curto
		{
			u8 v;
			v = (u8)(s_LOn ? (s_LAge < 3 ? 9 : (u8)(8 - (s_LAge >> 3))) : 0); if (s_LOn && v < 4) v = 4;
			Wr(8, v);
			v = (u8)(s_BOn ? (u8)(9 - (s_BAge >> 1)) : 0); if (s_BOn && (v > 9 || v < 3)) v = 3;
			Wr(9, v);
			if (s_SfxId >= 5) { v = (u8)(s_COn && s_CAge < 12 ? (u8)(6 - (s_CAge >> 2)) : 0); Wr(10, v); }
		}
		if (s_LAge < 250) s_LAge++;
		if (s_BAge < 250) s_BAge++;
		if (s_CAge < 250) s_CAge++;
		if (++s_Sub >= s_Step) { s_Sub = 0; g_MusicPos = (u8)((g_MusicPos + 1) & 127); }
	}
	else MusicOff();
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
	else if (!g_SndMus || g_MusicType) Wr(10, 0);
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

static void OptRow(u8 i)                  // so a linha mudada (o redesenho completo perdia teclas rapidas)
{
	Ui_Begin(); OptBody(i); Ui_End(0, (u8)(CONTENT_Y + 13 + i * ROW_H), 256, ROW_H);
}

void Options_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Options");
	for (i = 0; i < 3; i++) OptBody(i);
	Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 54), "LEFT/RIGHT or OK change the value.");
	Ui_Text(4, (u8)(CONTENT_Y + 64), g_SramBit ? "Settings are kept in cartridge SRAM." : "Settings last until you switch off.");
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 84), "This MSX (for bug reports)");       // diagnostico para teste em hardware real
	Ui_Color(UI_WHITE);
	Ui_Text(4, (u8)(CONTENT_Y + 96), VDP_GetFrequency() ? "Video: 50 Hz" : "Video: 60 Hz");
	Ui_Text(4, (u8)(CONTENT_Y + 106), g_MusicType ? "Music: FM (MSX-MUSIC)" : "Music: PSG only");
	{ Ui_Text(4, (u8)(CONTENT_Y + 116), "SRAM:"); Ui_Text(40, (u8)(CONTENT_Y + 116), g_SramBit == 0x10 ? "bit 10h" : g_SramBit == 0x20 ? "bit 20h" : g_SramBit == 0x40 ? "bit 40h" : g_SramBit == 0x80 ? "bit 80h" : "none (use codes)"); }
	Ui_Text(4, (u8)(CONTENT_Y + 126), k_MouseTxt[g_PtrMode]);
	Ui_Text(4, (u8)(CONTENT_Y + 136), "Version " VERSION_STR);
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:change  BACK:office");
}

void Options_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel < 2) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) { OptRow(old); OptRow(g_Sel); }
	if (ev & IN_BACK) { Goto(SCR_OFFICE); return; }
	if (ev & (IN_OK | IN_LEFT | IN_RIGHT))
	{
		if (g_Sel == 0) g_SndSfx ^= 1;
		else if (g_Sel == 1) g_SndMus ^= 1;
		else if (ev & IN_LEFT) g_PtrSpeed = (u8)((g_PtrSpeed + 3) & 3);
		else g_PtrSpeed = (u8)((g_PtrSpeed + 1) & 3);
		OptRow(g_Sel);
		Options_Save();
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
