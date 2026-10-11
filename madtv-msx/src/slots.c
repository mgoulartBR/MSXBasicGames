// Salvar/carregar em SRAM do cartucho (mapper ASCII8 com SRAM) + tela de slots. Segmento 13 (banco 2).
// A SRAM e mapeada no banco 3 (A000h) escrevendo no registrador do banco (7800h) o valor "bit de habilitacao"; esse bit depende do
// tamanho da ROM (128 KB = 10h, 256 KB = 20h no openMSX), entao o jogo SONDA 10h/20h/40h/80h e usa o primeiro que se comporta como RAM.
// Enquanto mapeada, o banco 3 NAO le dados da ROM (catalogo): as rotinas abaixo copiam de/para RAM e restauram o segmento anterior.
#include "app.h"

u8 g_SramBit;                       // 0 = sem SRAM (usa-se o codigo digitado)
static u8 s_Mode;                   // 0 menu, 1 salvar, 2 carregar
static u8 s_Prev;
#define SR            ((volatile u8*)0xA000)
#define BANK3_REG     ((volatile u8*)0x7800)
#define NSLOTS        4
#define SLOT_SZ       (SAVE_BYTES + 1)
#define SLOT_OFS(i)   (8 + (u16)(i) * SLOT_SZ)
#define PROBE_OFS     0x0600

static void Map(void)   { s_Prev = GET_BANK_SEGMENT(3); *BANK3_REG = g_SramBit; }
static void Unmap(void) { *BANK3_REG = s_Prev; }

static u8 Magic(void)               // chamar com a SRAM mapeada: (re)inicializa se nao for nosso formato
{
	static const char k_M[5] = "MTVS";
	u8 i, ok = 1;
	for (i = 0; i < 4; i++) if (SR[i] != (u8)k_M[i]) ok = 0;
	if (!ok)
	{
		for (i = 0; i < 4; i++) SR[i] = (u8)k_M[i];
		SR[4] = 1;
		for (i = 0; i < NSLOTS; i++) SR[SLOT_OFS(i)] = 0;
	}
	return ok;
}

void Sram_Detect(void) __banked
{
	static const u8 k_Bits[4] = { 0x10, 0x20, 0x40, 0x80 };
	u8 k, a, b, ok;
	g_SramBit = 0;
	for (k = 0; k < 4; k++)
	{
		g_SramBit = k_Bits[k];
		Map();
		a = SR[PROBE_OFS]; b = SR[PROBE_OFS + 1];
		SR[PROBE_OFS] = 0xA5; SR[PROBE_OFS + 1] = 0x5A;
		ok = (u8)(SR[PROBE_OFS] == 0xA5 && SR[PROBE_OFS + 1] == 0x5A);
		if (ok) { SR[PROBE_OFS] = 0x5A; SR[PROBE_OFS + 1] = 0xA5; ok = (u8)(SR[PROBE_OFS] == 0x5A && SR[PROBE_OFS + 1] == 0xA5); }
		if (ok) { SR[PROBE_OFS] = a; SR[PROBE_OFS + 1] = b; }
		Unmap();
		if (ok) return;
	}
	g_SramBit = 0;
}

// resumo de um slot: 0 = vazio; senao 1 e preenche dia/caixa/dificuldade
static u8 SlotInfo(u8 i, u16* day, i32* money, u8* diff)
{
	u8 b[9], k, used;
	Map(); Magic();
	used = SR[SLOT_OFS(i)] == 0xA5;
	if (used) for (k = 0; k < 9; k++) b[k] = SR[SLOT_OFS(i) + 1 + k];
	Unmap();
	if (!used) return 0;
	*day = (u16)(b[1] | (b[2] << 8));
	*money = (i32)((u32)b[5] | ((u32)b[6] << 8) | ((u32)b[7] << 16) | ((u32)b[8] << 24));
	*diff = (u8)(b[0] >> 4);
	return 1;
}

static u8 AnyUsed(void)
{
	u8 i, any = 0;
	u16 d; i32 m; u8 f;
	for (i = 0; i < NSLOTS; i++) if (SlotInfo(i, &d, &m, &f)) any = 1;
	return any;
}

static void DoSave(u8 i)
{
	u8 b[SAVE_BYTES], k;
	Sim_SaveBytes(b);
	Map(); Magic();
	for (k = 0; k < SAVE_BYTES; k++) SR[SLOT_OFS(i) + 1 + k] = b[k];
	SR[SLOT_OFS(i)] = 0xA5;
	Unmap();
}

static u8 DoLoad(u8 i)               // 0 ok, 1 slot vazio, 2/3 erro do Sim_LoadBytes
{
	u8 b[SAVE_BYTES], k;
	Map(); Magic();
	if (SR[SLOT_OFS(i)] != 0xA5) { Unmap(); return 1; }
	for (k = 0; k < SAVE_BYTES; k++) b[k] = SR[SLOT_OFS(i) + 1 + k];
	Unmap();
	return Sim_LoadBytes(b);
}

#ifndef HOST_BUILD
void Slots_Enter(void) __banked
{
	s_Mode = 0;
	if (!g_SramBit) Goto(SCR_SAVE);        // sem SRAM: direto para a tela do codigo
}

void Load_Menu(void) __banked          // Esc no titulo: slots da SRAM (se houver algum usado) ou codigo
{
	if (g_SramBit && AnyUsed()) { Goto(SCR_SLOTS); s_Mode = 2; }
	else { Goto(SCR_SAVE); Save_EnterLoad(); }
}

static u8 RowCount(void) { return s_Mode == 0 ? 3 : NSLOTS + 1; }

void Slots_Draw(void) __banked
{
	static const char* const k_Menu[3] = { "Save game to a slot", "Load game from a slot", "Save code (type it in)" };
	static const char* const k_Diff[3] = { "Easy", "Normal", "Hard" };
	u8 i, y;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, s_Mode == 0 ? "Save / Load" : s_Mode == 1 ? "Save to which slot?" : "Load which slot?");
	Ui_Color(UI_GRAY); Ui_Text(150, CONTENT_Y, "cartridge SRAM");
	for (i = 0; i < RowCount(); i++)
	{
		y = (u8)(CONTENT_Y + 14 + i * ROW_H);
		Ui_Color(g_Sel == i ? UI_YELLOW : UI_WHITE);
		Ui_Text(4, y, g_Sel == i ? ">" : " ");
		if (s_Mode == 0) Ui_Text(14, y, k_Menu[i]);
		else if (i == NSLOTS) Ui_Text(14, y, "Back");
		else
		{
			u16 d; i32 m; u8 f;
			Ui_Text(14, y, "Slot"); Ui_Int(46, y, i + 1);
			if (SlotInfo(i, &d, &m, &f))
			{
				u8 x;
				Ui_Color(UI_GRAY);
				Ui_Text(66, y, "Day"); x = Ui_Int(90, y, d);
				Money(122, y, m);
				Ui_Text(180, y, k_Diff[f % 3]);
				(void)x;
			}
			else { Ui_Color(UI_GRAY); Ui_Text(66, y, "- empty -"); }
		}
	}
	if (s_Mode == 0) { Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 54), "4 slots survive switching the MSX off."); }
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:select  BACK:back");
}

void Slots_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel + 1 < RowCount()) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) g_Dirty |= D_CON;
	if (ev & IN_BACK)
	{
		if (s_Mode == 0) Goto(SCR_OFFICE);
		else { s_Mode = 0; g_Sel = 0; g_Dirty |= D_CON; }
		return;
	}
	if (!(ev & IN_OK)) return;
	if (s_Mode == 0)
	{
		if (g_Sel == 2) { Goto(SCR_SAVE); return; }
		s_Mode = (u8)(g_Sel + 1); g_Sel = 0; g_Dirty |= D_CON;
	}
	else if (g_Sel == NSLOTS) { s_Mode = 0; g_Sel = 0; g_Dirty |= D_CON; }
	else if (s_Mode == 1) { DoSave(g_Sel); Sim_MsgNum("Saved to slot ", g_Sel + 1, "."); g_Dirty |= D_CON | D_MSG; }
	else
	{
		u8 r = DoLoad(g_Sel);
		if (r == 0) { g_Speed = SPEED_PAUSE; Sim_Msg("Game loaded. Time is paused: press P."); Header_Invalidate(); g_Dirty |= D_HDR | D_MSG; Goto(SCR_HUB); }
		else { Sim_Msg(r == 1 ? "That slot is empty." : "Corrupt save data."); g_Dirty |= D_MSG; }
	}
}

void Slots_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(CONTENT_Y + 13), ROW_H, RowCount());
	(void)x;
	if (r == 0xFF) return;
	if (r != g_Sel) { g_Sel = r; g_Dirty |= D_CON; }
	if (btn) Slots_Input(IN_OK);
}
#endif
