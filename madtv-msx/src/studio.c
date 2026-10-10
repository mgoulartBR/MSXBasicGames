// Producao propria: Agencia de Roteiros (compra) + Estudios (producao) + logica. Segmento 11 (banco 2).
// Modelo PROPRIO (nao e o do Mad TV nem o do TVTower): roteiro -> orcamento -> filme exclusivo na biblioteca (max NUM_OWN).
#include "app.h"

extern Movie g_OwnMv[NUM_OWN];
static char s_OwnTitle[NUM_OWN][16];

static const char* const k_WordA[16] = { "Night", "Dream", "Shadow", "Last", "Golden", "Silent", "Red", "Wild", "Lost", "Dark", "Blue", "Iron", "Secret", "Final", "Crazy", "Hidden" };
static const char* const k_WordB[16] = { "Harbor", "Kingdom", "Summer", "Express", "Garden", "Letters", "Wolves", "City", "Mirror", "Island", "Hotel", "Tango", "Planet", "Bridge", "Circus", "Desert" };
static const u16 k_Budget[3] = { 150, 250, 400 };
static const u8  k_BudgetDays[3] = { 2, 3, 4 };
static const u8  k_BudgetQ[3] = { 30, 50, 70 };     // qualidade = base + 0..25
static const char* const k_BudgetName[3] = { "Low budget", "Medium budget", "High budget" };

void Sim_ScriptTitle(char* out, u8 name) __banked
{
	const char* a = k_WordA[name >> 4];
	const char* b = k_WordB[name & 15];
	u8 n = 0;
	while (*a) out[n++] = *a++;
	out[n++] = ' ';
	while (*b) out[n++] = *b++;
	out[n] = 0;
}

u16 Sim_ScriptCost(u8 blocks) __banked { return (u16)(40 + 40 * blocks); }
u16 Sim_BudgetCost(u8 b) __banked { return k_Budget[b]; }

void Sim_OwnRebuild(void) __banked
{
	u8 k;
	for (k = 0; k < NUM_OWN; k++)
	{
		const OwnMovie* o = &g_Game.own[k];
		Movie* m = &g_OwnMv[k];
		if (!o->used) continue;
		Sim_ScriptTitle(s_OwnTitle[k], o->name);
		m->title = s_OwnTitle[k]; m->cat = o->cat; m->year = 140; m->blocks = o->blocks;
		m->critics = o->q; m->speed = o->q; m->outcome = (u8)(o->q + (o->name & 7) > 100 ? 100 : o->q + (o->name & 7));
		m->price = 100; m->fsk18 = 0;
	}
}

static u8 FreeOwn(void)
{
	u8 k;
	for (k = 0; k < NUM_OWN; k++) if (!g_Game.own[k].used) return k;
	return NONE;
}

void Sim_OwnFree(u8 idx) __banked
{
	u8 k = (u8)(idx - DB_NUM_MOVIES), st;
	if (idx < DB_NUM_MOVIES || k >= NUM_OWN) return;
	g_Game.own[k].used = 0; g_Game.owned[idx] = 0;
	for (st = 0; st < NUM_STATIONS; st++) g_Game.plays[st][idx] = 0;
}

void Sim_StudioDay(void) __banked
{
	u8 i;
	if (g_Game.pstate == 2)
	{
		if (g_Game.pdays) g_Game.pdays--;
		if (!g_Game.pdays)
		{
			u8 k = FreeOwn();
			if (k != NONE)
			{
				OwnMovie* o = &g_Game.own[k];
				o->used = 1; o->cat = g_Game.pcat; o->blocks = g_Game.pblocks; o->name = g_Game.pname;
				o->q = (u8)(k_BudgetQ[g_Game.pbudget] + Sim_Rnd(26));
				g_Game.owned[DB_NUM_MOVIES + k] = 1;
				Sim_OwnRebuild();
				g_Game.pstate = 0;
				Sim_Msg("Production finished! See the Archive.");
			}
			else g_Game.pdays = 1;                                     // biblioteca cheia: o filme espera ate liberar espaco
		}
	}
	for (i = 0; i < 3; i++)                                            // roteiros do dia
	{
		ScriptOffer* o = &g_Game.soffer[i];
		o->cat = Sim_Rnd(DB_NUM_CATEGORIES);
		o->blocks = (u8)(1 + Sim_Rnd(3));
		o->name = (u8)(Sim_Rnd(16) << 4 | Sim_Rnd(16));
	}
}

u8 Sim_BuyScript(u8 i) __banked
{
	const ScriptOffer* o = &g_Game.soffer[i];
	u16 c = Sim_ScriptCost(o->blocks);
	if (g_Game.pstate) return 2;
	if (g_Game.money < (i32)c) return 1;
	g_Game.money -= c; g_Game.day_cost += c;
	g_Game.pstate = 1; g_Game.pcat = o->cat; g_Game.pblocks = o->blocks; g_Game.pname = o->name;
	return 0;
}

u8 Sim_StartProd(u8 b) __banked
{
	if (g_Game.pstate != 1) return 2;
	if (FreeOwn() == NONE) return 3;
	if (g_Game.money < (i32)k_Budget[b]) return 1;
	g_Game.money -= k_Budget[b]; g_Game.day_cost += k_Budget[b];
	g_Game.pstate = 2; g_Game.pbudget = b; g_Game.pdays = k_BudgetDays[b];
	return 0;
}

// ---------------------------------------------------------------- telas (so no MSX; os testes de PC compilam so a logica acima)
#ifndef HOST_BUILD
static void ScriptLine(u8 y, u8 cat, u8 blocks, u8 name)
{
	char t[24];
	Sim_ScriptTitle(t, name);
	Ui_Text(14, y, t);
	Ui_Color(UI_GRAY); Ui_Text(104, y, g_CategoryName[cat]);
	Ui_Int(166, y, blocks); Ui_Text(174, y, "bl");
}

static void StatusBody(u8 y)
{
	char t[24];
	if (g_Game.pstate == 0) { Ui_Color(UI_GRAY); Ui_Text(4, y, "No script in hand."); return; }
	Sim_ScriptTitle(t, g_Game.pname);
	Ui_Color(UI_WHITE); Ui_Text(4, y, g_Game.pstate == 1 ? "Script ready:" : "Filming:");
	Ui_Color(UI_YELLOW); Ui_Text(84, y, t);
	Ui_Color(UI_GRAY); Ui_Text(180, y, g_CategoryName[g_Game.pcat]);
}

void Scripts_Draw(void) __banked
{
	u8 i, y;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Script agency");
	Ui_Color(UI_GRAY);   Ui_Text(104, CONTENT_Y, "genre      len   price");
	for (i = 0; i < 3; i++)
	{
		const ScriptOffer* o = &g_Game.soffer[i];
		y = (u8)(CONTENT_Y + 14 + i * ROW_H);
		Ui_Color(g_Sel == i ? UI_YELLOW : UI_WHITE);
		Ui_Text(4, y, g_Sel == i ? ">" : " ");
		ScriptLine(y, o->cat, o->blocks, o->name);
		Ui_Color(g_Game.money >= (i32)Sim_ScriptCost(o->blocks) ? UI_GREEN : UI_RED);
		Money(206, y, Sim_ScriptCost(o->blocks));
	}
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 50), "Your script");
	StatusBody((u8)(CONTENT_Y + 62));
	Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 84), "Buy a script, then film it at the Studios.");
	Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 94), "New scripts are offered every day.");
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:buy script  BACK:building");
}

void Scripts_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel < 2) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) g_Dirty |= D_CON;
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_BuyScript(g_Sel);
		Sim_Msg(r == 0 ? "Script bought. Take it to the Studios." : r == 1 ? "Not enough money!" : "You already have a script in hand.");
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

void Scripts_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(CONTENT_Y + 13), ROW_H, 3);
	(void)x;
	if (r == 0xFF) return;
	if (r != g_Sel) { g_Sel = r; g_Dirty |= D_CON; }
	if (btn) Scripts_Input(IN_OK);
}

void Studio_Draw(void) __banked
{
	u8 i, y;
	char t[24];
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Studios");
	StatusBody((u8)(CONTENT_Y + 12));
	if (g_Game.pstate == 2)
	{
		u8 x = Ui_Int(4, (u8)(CONTENT_Y + 22), g_Game.pdays);
		Ui_Color(UI_GRAY); Ui_Text(x, (u8)(CONTENT_Y + 22), " day(s) until the premiere.");
	}
	else if (g_Game.pstate == 1)
	{
		Ui_Color(UI_GRAY); Ui_Text(104, (u8)(CONTENT_Y + 22), "cost    days  quality");
		for (i = 0; i < 3; i++)
		{
			y = (u8)(CONTENT_Y + 32 + i * ROW_H);
			Ui_Color(g_Sel == i ? UI_YELLOW : UI_WHITE);
			Ui_Text(4, y, g_Sel == i ? ">" : " ");
			Ui_Text(14, y, k_BudgetName[i]);
			Ui_Color(g_Game.money >= (i32)k_Budget[i] ? UI_GREEN : UI_RED); Money(104, y, k_Budget[i]);
			Ui_Color(UI_GRAY); Ui_Int(152, y, k_BudgetDays[i]);
			Ui_Int(184, y, k_BudgetQ[i]); Ui_Text(196, y, "-"); Ui_Int(204, y, k_BudgetQ[i] + 25);
		}
	}
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 70), "Your productions (exclusive +10%)");
	for (i = 0; i < NUM_OWN; i++)
	{
		const OwnMovie* o = &g_Game.own[i];
		y = (u8)(CONTENT_Y + 82 + i * ROW_H);
		if (!o->used) { Ui_Color(UI_GRAY); Ui_Text(14, y, "- free -"); continue; }
		Sim_ScriptTitle(t, o->name);
		Ui_Color(UI_WHITE); Ui_Text(14, y, t);
		Ui_Color(UI_GRAY); Ui_Text(120, y, g_CategoryName[o->cat]);
		Ui_Text(186, y, "Q"); Ui_Int(194, y, o->q); Ui_Int(222, y, o->blocks); Ui_Text(230, y, "bl");
	}
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:start filming  BACK:building");
}

void Studio_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel < 2) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) g_Dirty |= D_CON;
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_StartProd(g_Sel);
		Sim_Msg(r == 0 ? "Filming started!" : r == 1 ? "Not enough money!" : r == 2 ? "Buy a script first (Script agency)." : "Library full: sell a production first.");
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

void Studio_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(CONTENT_Y + 31), ROW_H, 3);
	(void)x;
	if (r == 0xFF || g_Game.pstate != 1) return;
	if (r != g_Sel) { g_Sel = r; g_Dirty |= D_CON; }
	if (btn) Studio_Input(IN_OK);
}

#endif
