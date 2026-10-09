// Telas: Supermercado (compra de presentes) e escritorio da Betty (dar presentes, pedir em casamento). Segmento 10 (banco 2).
#include "app.h"

static const char* const k_GiftName[NUM_GIFTS] = {
	"Chocolates", "Flowers", "Perfume", "Jewelry box", "Silk scarf", "Gold watch", "Fur coat", "Diamond ring", "Sports car", "Dream trip"
};

// ---------------------------------------------------------------- SUPERMERCADO
#define SHOP_Y(i) ((u8)(CONTENT_Y + 14 + (i) * ROW_H))

static void ShopRowBody(u8 i)
{
	u8 y = SHOP_Y(i), sel = (g_Sel == i);
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, sel ? ">" : " ");
	Ui_Text(14, y, k_GiftName[i]);
	Ui_Color(g_Game.money >= (i32)Sim_GiftCost(i) ? UI_GREEN : UI_RED);
	Money(104, y, Sim_GiftCost(i));
	Ui_Color(UI_GRAY);
	if (i == GIFT_DREAM) Ui_Text(150, y, "wedding");
	else { Ui_Text(150, y, "+"); Ui_Int(158, y, Sim_GiftEffect(i)); }
	Ui_Text(196, y, "have"); Ui_Int(226, y, g_Game.gift_have[i]);
}

static void ShopRow(u8 i)
{
	if (i >= NUM_GIFTS) return;
	Ui_Begin(); ShopRowBody(i); Ui_End(0, (u8)(SHOP_Y(i) - 1), 255, ROW_H);
}

void Shop_Enter(void) __banked { }

void Shop_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Supermarket");
	Ui_Color(UI_GRAY);   Ui_Text(90, CONTENT_Y, "price / gain / stock");
	for (i = 0; i < NUM_GIFTS; i++) ShopRowBody(i);
	Ui_Color(UI_GRAY); Ui_Text(4, (u8)(SHOP_Y(NUM_GIFTS) + 6), "Betty is on the top floor. Repeats weaken.");
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:buy  BACK:building");
}

void Shop_Row(u8 r) __banked { ShopRow(r); }

void Shop_Input(u8 ev) __banked
{
	NavApply(Nav(ev, NUM_GIFTS, NUM_GIFTS));
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_BuyGift(g_Sel);
		Sim_Msg(r == 0 ? "Bought." : r == 1 ? "Not enough money!" : "You already have 3 of those.");
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

void Shop_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(CONTENT_Y + 13), ROW_H, NUM_GIFTS);
	(void)x;
	if (r == 0xFF || !SelVisible(r, NUM_GIFTS)) return;
	if (btn) Shop_Input(IN_OK);
}

// ---------------------------------------------------------------- ESCRITORIO DA BETTY
// linhas: 0..9 = presentes em estoque (OK = dar), 10 = pedir em casamento
#define BET_ROWS (NUM_GIFTS + 1)
#define BET_Y(i) ((u8)(CONTENT_Y + 45 + (i) * ROW_H))

static void BettyRowBody(u8 i)
{
	u8 y = BET_Y(i), sel = (g_Sel == i);
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, sel ? ">" : " ");
	if (i == NUM_GIFTS) { Ui_Color(sel ? UI_YELLOW : UI_CYAN); Ui_Text(14, y, "Propose marriage!"); return; }
	Ui_Color(g_Game.gift_have[i] ? (sel ? UI_YELLOW : UI_WHITE) : UI_GRAY);
	Ui_Text(14, y, k_GiftName[i]);
	Ui_Color(UI_GRAY);
	if (i == GIFT_DREAM) Ui_Text(150, y, "wedding");
	else { Ui_Text(150, y, "+"); Ui_Int(158, y, Sim_GiftEffect(i)); }
	Ui_Text(196, y, "have"); Ui_Int(226, y, g_Game.gift_have[i]);
}

static void BettyRow(u8 i)
{
	if (i >= BET_ROWS) return;
	Ui_Begin(); BettyRowBody(i); Ui_End(0, (u8)(BET_Y(i) - 1), 255, ROW_H);
}

static void BettyStatusBody(void)
{
	u8 st, x;
	Ui_Fill(0, CONTENT_Y + 10, 255, 31, UI_BG);
	Ui_Color(UI_WHITE); Ui_Text(4, (u8)(CONTENT_Y + 12), "You");
	Ui_Bar(32, (u8)(CONTENT_Y + 12), 120, 7, g_Game.sym[0], COLOR_LIGHT_GREEN);
	x = Ui_Int(158, (u8)(CONTENT_Y + 12), g_Game.sym[0]);
	Ui_Color(UI_GRAY); Ui_Text(x, (u8)(CONTENT_Y + 12), "/100");
	Ui_Text(196, (u8)(CONTENT_Y + 12), "cap"); Ui_Int(220, (u8)(CONTENT_Y + 12), g_Game.image[0]);
	for (st = 1; st < NUM_STATIONS; st++)
	{
		u8 y = (u8)(CONTENT_Y + 12 + st * 10);
		Ui_Color(UI_GRAY); Ui_Text(4, y, st == 1 ? "FunTV" : "SunTV");
		if (!g_Game.alive[st]) { Ui_Color(UI_RED); Ui_Text(52, y, "bankrupt"); continue; }
		Ui_Bar(52, y, 100, 7, g_Game.sym[st], COLOR_MEDIUM_RED);
		Ui_Int(158, y, g_Game.sym[st]);
	}
}

void Betty_Enter(void) __banked { }

void Betty_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Betty's office");
	Ui_Color(UI_GRAY);   Ui_Text(110, CONTENT_Y, "her sympathy");
	BettyStatusBody();
	for (i = 0; i < BET_ROWS; i++) BettyRowBody(i);
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:give gift / propose  BACK:building");
}

void Betty_Row(u8 r) __banked { BettyRow(r); }

void Betty_Input(u8 ev) __banked
{
	NavApply(Nav(ev, BET_ROWS, BET_ROWS));
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		u8 r;
		if (g_Sel == NUM_GIFTS)
		{
			r = Sim_Propose();
			if (r == 0) { g_Game.game_over = 1; return; }           // casou: main mostra a tela final
			Sim_Msg(r == 1 ? "She wants more: sympathy must reach 100." : r == 2 ? "Rival stations still stand. Beat them first!" : "You need a Dream trip in stock.");
		}
		else
		{
			r = Sim_GiveGift(g_Sel);
			if (r == 0) Sim_MsgNum("Betty smiles! Sympathy +", g_Game.last_gain, "");
			else Sim_Msg(r == 1 ? "You have none of those." : r == 2 ? "She is as fond as your Image allows." : "Keep it for the proposal.");
		}
		g_Dirty |= D_CON | D_MSG;
	}
}

void Betty_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(CONTENT_Y + 44), ROW_H, BET_ROWS);
	(void)x;
	if (r == 0xFF || !SelVisible(r, BET_ROWS)) return;
	if (btn) Betty_Input(IN_OK);
}

// ---------------------------------------------------------------- PORTEIRO (resumo do dia + dicas)
void Porter_Draw(void) __banked
{
	u8 st, y = (u8)(CONTENT_Y + 52), empty = 0, nc = 0, s, x;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Porter's desk");
	Ui_Color(UI_GRAY);   Ui_Text(4, (u8)(CONTENT_Y + 12), "Income today");   Ui_Color(UI_GREEN); Money(100, (u8)(CONTENT_Y + 12), g_Game.day_income);
	Ui_Color(UI_GRAY);   Ui_Text(4, (u8)(CONTENT_Y + 22), "Costs today");    Ui_Color(UI_RED);   Money(100, (u8)(CONTENT_Y + 22), g_Game.day_cost);
	Ui_Color(UI_GRAY);   Ui_Text(4, (u8)(CONTENT_Y + 32), "Debt to Raffer"); Ui_Color(UI_WHITE); Money(100, (u8)(CONTENT_Y + 32), g_Game.debt);
	Ui_Color(UI_GRAY);   Ui_Text(150, CONTENT_Y + 12, "Standings (Image)");
	for (st = 0; st < NUM_STATIONS; st++)
	{
		u8 yy = (u8)(CONTENT_Y + 22 + st * 10);
		Ui_Color(g_StationCol[st]); Ui_Text(150, yy, g_StationName[st]);
		if (g_Game.alive[st]) Ui_Int(196, yy, g_Game.image[st]); else Ui_Text(196, yy, "out");
	}
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(y - 2), "Tips from the porter");
	y += 10;
	for (s = 0; s < NUM_SLOTS; s++) if (g_Game.slot[0][s].movie == NONE) empty++;
	for (s = 0; s < MAX_CONTRACTS; s++) if (g_Game.contract[s].ad != NONE) nc++;
	Ui_Color(UI_WHITE);
	if (empty) { x = Ui_Int(4, y, empty); Ui_Text(x, y, " programme slot(s) still empty."); y += 10; }
	if (!nc) { Ui_Text(4, y, "No ad contracts: try the Ad agency."); y += 10; }
	if (g_Game.image[0] < IMAGE_LOW) { Ui_Color(UI_RED); Ui_Text(4, y, "Image below 20: ads pay less, no credit!"); y += 10; Ui_Color(UI_WHITE); }
	if (!g_Game.gift_today && g_Game.sym[0] < g_Game.image[0]) { Ui_Text(4, y, "Betty is waiting for a gift today."); y += 10; }
	if (g_Game.money < 300) { Ui_Text(4, y, "The cash box is almost empty."); y += 10; }
	if (y == (u8)(CONTENT_Y + 62)) Ui_Text(4, y, "All quiet. Keep an eye on the ratings.");
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("BACK:building");
}

void Porter_Input(u8 ev) __banked { if (ev & IN_BACK) Goto(SCR_HUB); }

// ---------------------------------------------------------------- ESCRITORIO DOS RIVAIS (espionagem: grade e audiencias de hoje)
void Rival_Draw(void) __banked
{
	u8 st = (u8)(g_Screen - SCR_FUN + 1), s, y;
	char b[8];
	Ui_Begin();
	ClearContent();
	Ui_Color(g_StationCol[st]); Ui_Text(4, CONTENT_Y, g_StationName[st]);
	Ui_Color(UI_GRAY); Ui_Text(60, CONTENT_Y, "office - tonight's schedule");
	if (!g_Game.alive[st]) { Ui_Color(UI_RED); Ui_Text(14, (u8)(CONTENT_Y + 20), "Bankrupt. The office is empty."); }
	else
	{
		Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 12), "Image"); Ui_Int(40, (u8)(CONTENT_Y + 12), g_Game.image[st]);
		Ui_Text(80, (u8)(CONTENT_Y + 12), "Betty likes them:"); Ui_Int(184, (u8)(CONTENT_Y + 12), g_Game.sym[st]);
		for (s = 0; s < NUM_SLOTS; s++)
		{
			const Slot* sl = &g_Game.slot[st][s];
			y = (u8)(CONTENT_Y + 26 + s * ROW_H);
			ClockStr(b, (u16)(FIRST_SLOT_T + s * 60));
			Ui_Color(UI_GRAY); Ui_Text(4, y, b);
			Ui_Color(UI_WHITE);
			if (sl->movie == NONE) { Ui_Color(UI_GRAY); Ui_Text(44, y, "-"); }
			else
			{
				Ui_TextN(44, y, g_Movies[sl->movie].title, 20);
				Ui_Color(UI_GRAY); Ui_Int(206, y, sl->part + 1); Ui_Text(214, y, "/"); Ui_Int(222, y, g_Movies[sl->movie].blocks);
			}
			if (g_Game.aud_done[s]) { Ui_Color(g_StationCol[st]); Ui_Dec1(236, y, g_Game.aud[st][s]); }
		}
		Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 26 + NUM_SLOTS * ROW_H + 4), "Audience (millions) once aired.");
	}
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("BACK:building");
}

void Rival_Input(u8 ev) __banked { if (ev & IN_BACK) Goto(SCR_HUB); }

// ---------------------------------------------------------------- CORRETOR (torres de transmissao)
void Realtor_Draw(void) __banked
{
	u8 x, st;
	u16 c = Sim_TowerCost();
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Realtor - transmitter towers");
	Ui_Color(UI_GRAY);   Ui_Text(4, (u8)(CONTENT_Y + 16), "Your tower level");  Ui_Color(UI_WHITE); Ui_Int(130, (u8)(CONTENT_Y + 16), g_Game.tower); Ui_Text(142, (u8)(CONTENT_Y + 16), "/"); Ui_Int(150, (u8)(CONTENT_Y + 16), TOWER_MAX);
	Ui_Color(UI_GRAY);   Ui_Text(4, (u8)(CONTENT_Y + 26), "Audience reach");    Ui_Color(UI_WHITE); Ui_Dec1(130, (u8)(CONTENT_Y + 26), Sim_Reach(0)); Ui_Text(154, (u8)(CONTENT_Y + 26), "million");
	Ui_Color(UI_GRAY);   Ui_Text(4, (u8)(CONTENT_Y + 36), "Tower upkeep");      Ui_Color(UI_WHITE); x = Ui_Int(130, (u8)(CONTENT_Y + 36), TOWER_UPKEEP * g_Game.tower); Ui_Text(x, (u8)(CONTENT_Y + 36), "k/day");
	for (st = 1; st < NUM_STATIONS; st++)
	{
		u8 y = (u8)(CONTENT_Y + 50 + st * 10);
		Ui_Color(g_StationCol[st]); Ui_Text(4, y, g_StationName[st]);
		Ui_Color(UI_GRAY); Ui_Text(60, y, "reach"); Ui_Dec1(100, y, Sim_Reach(st));
	}
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 84), c ? ">" : " ");
	if (c) { Ui_Text(14, (u8)(CONTENT_Y + 84), "Build next tower:"); Money(120, (u8)(CONTENT_Y + 84), c); }
	else { Ui_Color(UI_GREEN); Ui_Text(14, (u8)(CONTENT_Y + 84), "Maximum coverage reached."); }
	Ui_Color(UI_GRAY); Ui_Text(4, (u8)(CONTENT_Y + 100), "More reach = more audience, Image and ads.");
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:build tower  BACK:building");
}

void Realtor_Input(u8 ev) __banked
{
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		u8 r = Sim_BuyTower();
		Sim_Msg(r == 0 ? "Tower built! Your reach grows." : r == 1 ? "Not enough money!" : "No more towers to build.");
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

void Realtor_Mouse(u8 x, u8 y, u8 btn) __banked
{
	(void)x;
	if (btn && y >= CONTENT_Y + 82 && y < CONTENT_Y + 96) Realtor_Input(IN_OK);
}
