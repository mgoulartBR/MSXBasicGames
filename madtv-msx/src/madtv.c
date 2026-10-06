// MadTV-MSX - milestone 0.2: navegador de filmes com dados convertidos do TVTower
#include "msxgl.h"
#include "font/font_mgl_sample6.h"
#include "core.h"
#include "data/db_data.h"

// Profiling opcional (compilar com -DMADTV_PROF): maximo de jiffies (1/50 ou 1/60 s) gasto num redraw.
#ifdef MADTV_PROF
u16 g_ProfMaxPartial, g_ProfMaxFull, g_ProfMaxDetail;
static u16 s_ProfT0;
#define JIFFY (*(volatile u16*)0xFC9E)
#define PROF_BEGIN() (s_ProfT0 = JIFFY)
#define PROF_END(full) do { u16 d = JIFFY - s_ProfT0; if (full) { if (d > g_ProfMaxFull) g_ProfMaxFull = d; } else if (d > g_ProfMaxPartial) g_ProfMaxPartial = d; } while (0)
#else
#define PROF_BEGIN()
#define PROF_END(full)
#endif

#define LIST_ROWS      12
#define LIST_Y         28
#define ROW_H          9
#define COL_BG         COLOR_BLACK
#define COL_TXT        COLOR_WHITE
#define COL_SEL        COLOR_LIGHT_YELLOW
#define COL_DIM        COLOR_GRAY

// indices dos filmes por categoria (preenchido no boot)
static u8 s_CatStart[DB_NUM_CATEGORIES + 1];

static void Db_Index(void)
{
	u8 c = 0, i;
	for (i = 0; i < DB_NUM_CATEGORIES; i++) s_CatStart[i] = 0xFF;
	for (i = 0; i < DB_NUM_MOVIES; i++)
		if (g_Movies[i].cat >= c) { /* tabela ordenada por categoria */ if (s_CatStart[g_Movies[i].cat] == 0xFF) s_CatStart[g_Movies[i].cat] = i; }
	s_CatStart[DB_NUM_CATEGORIES] = DB_NUM_MOVIES;
}

static u8 Cat_Count(u8 cat)
{
	u8 n = 0, i;
	for (i = 0; i < DB_NUM_MOVIES; i++) if (g_Movies[i].cat == cat) n++;
	return n;
}

// PRINT_SKIP_SPACE ignora espacos (nao apaga): limpar a area com o VDP antes de redesenhar
static void Clear(u8 x, u8 y, u8 w, u8 h)
{
	VDP_CommandHMMV(x, y, w, h, COLOR_MERGE2(COL_BG));
}

static void Bar(u8 x, u8 y, u8 value, u8 col)
{
	// largura 50 px = 100%; HMMV exige x/largura pares em Screen 5
	u8 w = (u8)((value >> 1) & 0xFE);
	VDP_CommandHMMV(x, y, 52, 5, COLOR_MERGE2(COLOR_DARK_BLUE));
	if (w) VDP_CommandHMMV(x, y, w, 5, COLOR_MERGE2(col));
}

static void Draw_Header(u8 cat)
{
	Print_SetColor(COLOR_LIGHT_YELLOW, COL_BG);
	Print_SetPosition(4, 4);
	Print_DrawText("MadTV-MSX 0.2  Film Agency");
	Clear(0, 15, 255, 9);
	Print_SetColor(COL_TXT, COL_BG);
	Print_SetPosition(4, 16);
	Print_DrawText("< ");
	Print_DrawText(g_CategoryName[cat]);
	Print_DrawText(" >");
}

static void Draw_Row(u8 cat, u8 i, u8 sel, u8 first)
{
	u8 n = Cat_Count(cat);
	Clear(0, LIST_Y + i * ROW_H - 1, 255, ROW_H);
	if (first + i < n)
	{
		Print_SetPosition(4, LIST_Y + i * ROW_H);
		Print_SetColor((first + i == sel) ? COL_SEL : COL_TXT, COL_BG);
		Print_DrawText((first + i == sel) ? "> " : "  ");
		Print_DrawText(g_Movies[s_CatStart[cat] + first + i].title);
	}
}

static void Draw_List(u8 cat, u8 sel, u8 first)
{
	u8 i;
	for (i = 0; i < LIST_ROWS; i++) Draw_Row(cat, i, sel, first);
}

static void Draw_Detail(u8 idx)
{
	const Movie* m = &g_Movies[idx];
	u8 avg = (u8)(((u16)m->critics + m->speed + m->outcome) / 3);
	u16 price = (u16)(((u16)avg * m->price) / 10); // x1000 $ (formula provisoria, NAO e a do TVTower)
	Clear(0, 141, 255, 24);
	Print_SetColor(COL_TXT, COL_BG);
	Print_SetPosition(4, 142); Print_DrawText("Year "); Print_DrawInt(1900 + m->year);
	Print_DrawText("  Blocks "); Print_DrawInt(m->blocks);
	Print_SetPosition(4, 152); Print_DrawText("Price $"); Print_DrawInt(price); Print_DrawText("k");
	if (m->fsk18) { Print_SetColor(COLOR_LIGHT_RED, COL_BG); Print_SetPosition(120, 152); Print_DrawText("FSK 18"); }
	Print_SetColor(COL_TXT, COL_BG);
	Print_SetPosition(4, 166);  Print_DrawText("Critics");  Bar(56, 167, m->critics, COLOR_LIGHT_GREEN);
	Print_SetPosition(4, 176);  Print_DrawText("Speed");    Bar(56, 177, m->speed,   COLOR_CYAN);
	Print_SetPosition(4, 186);  Print_DrawText("Box off.");  Bar(56, 187, m->outcome, COLOR_LIGHT_YELLOW);
	Print_SetColor(COL_DIM, COL_BG);
	Print_SetPosition(4, 200);  Print_DrawText("Data: TVTower (altered for MSX)");
}

// Entrada: teclado + joystick 1 (bits ativos em 1). Retorna mascara de eventos "novos" desde o ultimo frame.
#define IN_UP    (1 << 0)
#define IN_DOWN  (1 << 1)
#define IN_LEFT  (1 << 2)
#define IN_RIGHT (1 << 3)
#define IN_ESC   (1 << 4)
static u8 s_PrevIn;
static u8 Input_Poll(void)
{
	u8 in = 0, joy = Joystick_Read(JOY_PORT_1), pushed;
	if (Keyboard_IsKeyPressed(KEY_UP)    || !(joy & JOY_INPUT_DIR_UP))    in |= IN_UP;
	if (Keyboard_IsKeyPressed(KEY_DOWN)  || !(joy & JOY_INPUT_DIR_DOWN))  in |= IN_DOWN;
	if (Keyboard_IsKeyPressed(KEY_LEFT)  || !(joy & JOY_INPUT_DIR_LEFT))  in |= IN_LEFT;
	if (Keyboard_IsKeyPressed(KEY_RIGHT) || !(joy & JOY_INPUT_DIR_RIGHT)) in |= IN_RIGHT;
	if (Keyboard_IsKeyPressed(KEY_ESC)) in |= IN_ESC;
	pushed = in & ~s_PrevIn;
	s_PrevIn = in;
	return pushed;
}

void main()
{
	u8 cat = 0, sel = 0, first = 0, n, ev;

	VDP_SetMode(VDP_MODE_SCREEN5);
	VDP_SetColor(COL_BG);
	VDP_EnableVBlank(TRUE);
	VDP_ClearVRAM();
	Print_SetBitmapFont(g_Font_MGL_Sample6);
	Db_Index();
#ifdef MADTV_PROF
	g_ProfMaxPartial = 0; g_ProfMaxFull = 0; g_ProfMaxDetail = 0;
#endif
	Draw_Header(0); Draw_List(0, 0, 0); Draw_Detail(s_CatStart[0]);

	for (;;)
	{
		u8 old_sel, old_first, full = 0;
		Halt();
		ev = Input_Poll();
		if (ev & IN_ESC) break;
		if (!ev) continue;
		old_sel = sel; old_first = first;
		n = Cat_Count(cat);
		if ((ev & IN_DOWN) && sel + 1 < n) { sel++; if (sel >= first + LIST_ROWS) first++; }
		if ((ev & IN_UP) && sel > 0)       { sel--; if (sel < first) first--; }
		if (ev & IN_RIGHT) { cat = (cat + 1) % DB_NUM_CATEGORIES; sel = first = 0; full = 1; }
		if (ev & IN_LEFT)  { cat = (cat + DB_NUM_CATEGORIES - 1) % DB_NUM_CATEGORIES; sel = first = 0; full = 1; }
		if (sel == old_sel && first == old_first && !full) continue;
		PROF_BEGIN();
		if (full) { Draw_Header(cat); Draw_List(cat, sel, first); }
		else if (first != old_first) Draw_List(cat, sel, first);       // scroll: lista inteira
		else { Draw_Row(cat, old_sel - first, sel, first); Draw_Row(cat, sel - first, sel, first); } // dirty rows
		PROF_END(full ? 1 : 0);
#ifdef MADTV_PROF
		{ u16 t0 = JIFFY, d;
		  Draw_Detail(s_CatStart[cat] + sel);
		  d = JIFFY - t0; if (d > g_ProfMaxDetail) g_ProfMaxDetail = d; }
#else
		Draw_Detail(s_CatStart[cat] + sel);
#endif
	}
	BIOS_Exit(0);
}
