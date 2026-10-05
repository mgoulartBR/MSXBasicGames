// Cash out, shop and booster pack screens.
#include "ui.h"

//-----------------------------------------------------------------------------
// cash out
//-----------------------------------------------------------------------------
static void cash_row(u8 idx)
{
	Cash* r = &ui.cash[idx];
	u8 y = 30 + idx * 13;
	char lab[24];
	Vid_Fill(48, y, 160, 11, COL_INK);
	switch (r->kind)
	{
		case 0: Vid_Text(52, y + 1, !r->who ? "Saved by Mr. Bones" : (g.blind == BLIND_SMALL ? "Small Blind" : (g.blind == BLIND_BIG ? "Big Blind" : g_Bosses[g.boss].name)), TC_WHITE); break;
		case 1: Vid_Num(52, y + 1, r->who, TC_BLUE); Vid_Text(52 + Vid_NumW(r->who) + 4, y + 1, "Hands left ($1 each)", TC_SLATE); break;
		case 2: Vid_Text(52, y + 1, g_Jokers[g.jk[r->who].id].name, TC_GOLD); break;
		case 4: Vid_Text(52, y + 1, "Investment Tag", TC_GOLD); break;
		case 5: Vid_Num(52, y + 1, r->who, TC_GOLD); Vid_Text(52 + Vid_NumW(r->who) + 4, y + 1, "Gold Cards held ($3 each)", TC_GOLD); break;
		default: Vid_Text(52, y + 1, "Interest ($1 per $5)", TC_SLATE); break;
	}
	(void)lab;
	Vid_Text(190, y + 1, "$", TC_GOLD);
	Vid_NumR(204, y + 1, r->amount, TC_GOLD);
	snd(5);
}

void scr_cashout(void) BANKED
{
	Vid_Clear(COL_FELT);
	Vid_TextC(128, 6, g.blind == BLIND_BOSS ? "Boss Blind defeated" : "Blind defeated", TC_GOLD);
	Vid_Panel(44, 24, 168, 140, COL_INK, COL_SLATE);
	ui.nCash = cashout_build(ui.cash, &ui.cashTotal);
	ui.cashShown = 0; ui.timer = FR(20);
	ui.phase = 0;
}

void upd_cashout(void) BANKED
{
	if (ui.cashShown < ui.nCash)
	{
		if (in.pressed & IN_OK) { while (ui.cashShown < ui.nCash) cash_row(ui.cashShown++); ui.timer = 0; }
		else if (ui.timer) ui.timer--;
		else { cash_row(ui.cashShown++); ui.timer = FR(16); }
		return;
	}
	if (!ui.phase)
	{
		ui.phase = 1;
		Vid_Fill(48, 136, 160, 1, COL_SLATE);
		Vid_Text(52, 140, "Total", TC_WHITE);
		Vid_Text(190, 140, "$", TC_GOLD); Vid_NumR(204, 140, ui.cashTotal, TC_GOLD);
		ui_add(W_OK, 68, 168, 120, 18);
		ui_set_focus(0);
		char b[16] = "Cash Out $"; u8 n = 10; i16 t = ui.cashTotal; char d[6]; u8 k = 0;
		do { d[k++] = (char)('0' + t % 10); t /= 10; } while (t);
		while (k) b[n++] = d[--k];
		b[n] = 0;
		ui_button(W_OK, b, COL_GREEN, TRUE);
		return;
	}
	ui_pointer_focus();
	bool go = (in.pressed & IN_OK) != 0;
	if (in.click) { u8 h = ui_hit(in.mx, in.my); go = h != 0xFF; }
	if (go)
	{
		g.money += ui.cashTotal;
		snd(6);
		if (g.blind == BLIND_BOSS && g.ante >= (g.endless ? END_ANTE : MAX_ANTE)) { ui_goto(SC_WIN); return; }
		shop_generate();
		ui_goto(SC_SHOP);
	}
}

//-----------------------------------------------------------------------------
// shop
//-----------------------------------------------------------------------------
#define SHOP_X(i)  (70 + (i) * 30)
#define SHOP_Y     52
#define PACK_X(i)  (70 + (i) * 30)
#define PACK_Y     110
#define VOUCH_X    176
#define VOUCH_Y    110

static const char* const k_packTag[5] = { "ARC", "CEL", "BUF", "STD", "SPC" };

static void shop_card(u8 i, u8 x, u8 y)
{
	u8 t = g.shopType[i], id = g.shopId[i];
	if (t == 1) Vid_Joker(id, x, y);
	else Vid_Card(t == 2 ? CELL_PLANET + id : CELL_TAROT + id, x, y);
}

static void draw_price(u8 x, u8 y, u8 price)
{
	Vid_Fill(x - 2, y, 28, 10, COL_FELT);
	if (!price) { Vid_Text(x + 2, y + 1, "Free", TC_GREEN); return; }
	Vid_Text(x + 3, y + 1, "$", TC_GOLD);
	Vid_Num(x + 9, y + 1, price, g.money < price ? TC_RED : TC_GOLD);
}

static void shop_item(u8 id)
{
	if (id == 0xFF) return;
	bool foc = ui.focus != 0xFF && ui.w[ui.focus].id == id;
	if (id >= W_JOKER && id < W_PLAY) { draw_jslot(id); return; }
	if (id >= W_SHOPCARD && id < W_SHOPCARD + SHOP_CARD_MAX)
	{
		u8 i = id - W_SHOPCARD, x = SHOP_X(i);
		if (g.shopType[i])
		{
			shop_card(i, x, SHOP_Y);
			if (foc) Vid_Frame(x, SHOP_Y, 24, 32, COL_GOLD);
			draw_price(x, SHOP_Y + 34, shop_cost(i));
		}
		else { Vid_Fill(x, SHOP_Y, 24, 44, COL_FELT); Vid_Frame(x, SHOP_Y, 24, 32, COL_SLATE); }
		return;
	}
	if (id >= W_PACK && id < W_PACK + 2)
	{
		u8 i = id - W_PACK, x = PACK_X(i), k = g.packType[i];
		if (k)
		{
			static const u8 col[5] = { COL_RED, COL_BLUE, COL_ORANGE, COL_GREEN, COL_PURPLE };
			Vid_Panel(x, PACK_Y, 24, 32, col[(k - 1) / 3], foc ? COL_GOLD : COL_INK);
			Vid_TextC(x + 12, PACK_Y + 6, k_packTag[(k - 1) / 3], TC_WHITE);
			Vid_TextC(x + 12, PACK_Y + 18, (k - 1) % 3 == 0 ? "" : ((k - 1) % 3 == 1 ? "JMB" : "MEGA"), TC_INK);
			draw_price(x, PACK_Y + 34, pack_price(i));
		}
		else { Vid_Fill(x, PACK_Y, 24, 44, COL_FELT); Vid_Frame(x, PACK_Y, 24, 32, COL_SLATE); }
		return;
	}
	switch (id)
	{
		case W_VOUCHER:
			if (g.voucher)
			{
				Vid_Card(g_Vouchers[g.voucher - 1].cell, VOUCH_X, VOUCH_Y);
				if (foc) Vid_Frame(VOUCH_X, VOUCH_Y, 24, 32, COL_GOLD);
				draw_price(VOUCH_X, VOUCH_Y + 34, voucher_price());
			}
			else { Vid_Fill(VOUCH_X, VOUCH_Y, 24, 44, COL_FELT); Vid_Frame(VOUCH_X, VOUCH_Y, 24, 32, COL_SLATE); }
			break;
		case W_NEXT: ui_button(W_NEXT, T_NEXT, COL_GREEN, TRUE); break;
		case W_REROLL:
		{
			char b[12] = "Reroll $"; u8 n = 8; if (g.rerollCost >= 10) b[n++] = '0' + g.rerollCost / 10; b[n++] = '0' + g.rerollCost % 10; b[n] = 0;
			ui_button(W_REROLL, b, COL_RED, TRUE);
			break;
		}
	}
}

void shop_focus(u8 o, u8 n) BANKED { shop_item(o); shop_item(n); }

static void draw_shop(void)
{
	Vid_Fill(AREA_X, 0, AREA_W, 174, COL_FELT);
	draw_joker_row(TRUE);
	Vid_Text(AREA_X + 6, 43, "Shop", TC_GOLD);
	for (u8 i = 0; i < g.shopN; i++) shop_item(W_SHOPCARD + i);
	Vid_Text(AREA_X + 6, 98, "Booster Packs", TC_SLATE);
	for (u8 i = 0; i < 2; i++) shop_item(W_PACK + i);
	Vid_Text(VOUCH_X - 4, 98, "Voucher", TC_SLATE);
	shop_item(W_VOUCHER);
	shop_item(W_NEXT); shop_item(W_REROLL);
	hud_mini();
	info_show(ui_focus_id());
	ui.dirty = 0;
}

static void shop_widgets(void)
{
	ui_clear_widgets();
	hud_mini();
	for (u8 i = 0; i < g.nJk; i++) ui_add(W_JOKER + i, JOKER_X(i), JOKER_Y, 24, 32);
	for (u8 i = 0; i < CONS_MAX; i++) if (g.cons[i]) ui_add(W_CONS + i, CONS_X(i), JOKER_Y, 24, 32);
	for (u8 i = 0; i < g.shopN; i++) ui_add(W_SHOPCARD + i, SHOP_X(i), SHOP_Y, 24, 44);
	for (u8 i = 0; i < 2; i++) ui_add(W_PACK + i, PACK_X(i), PACK_Y, 24, 44);
	if (g.voucher) ui_add(W_VOUCHER, VOUCH_X, VOUCH_Y, 24, 44);
	ui_add(W_NEXT, 66, 158, 88, BTN_H);
	ui_add(W_REROLL, 158, 158, 88, BTN_H);
	if (ui.itemKind) { ui_add(W_SELL, 0, 0, 38, 12); if (ui.itemKind == 2) ui_add(W_USE, 0, 0, 30, 12); }
	ui.defFocus = g.shopType[0] ? W_SHOPCARD : W_NEXT;
	ui.focus = 0xFF;
}

void scr_shop(void) BANKED
{
	Vid_Clear(COL_FELT);
	ui.itemKind = 0;
	shop_widgets();
	draw_shop();
}

static void shop_activate(u8 id)
{
	if (id >= W_JOKER && id < W_PLAY)
	{
		u8 it = joker_item_at(id), kind = it >> 4, idx = it & 15;
		if (ui.itemKind == kind && ui.itemIdx == idx) ui.itemKind = 0; else { ui.itemKind = kind; ui.itemIdx = idx; }
		u8 f = ui_focus_id(); shop_widgets(); ui.focus = ui_find(f); draw_shop();
		return;
	}
	if (id >= W_SHOPCARD && id < W_SHOPCARD + SHOP_CARD_MAX)
	{
		u8 i = id - W_SHOPCARD;
		if (!g.shopType[i]) return;
		if (shop_buy(i)) { snd(7); shop_widgets(); ui.focus = ui_find(id); draw_shop(); }
		else ui_msg(g.money - shop_cost(i) < debt_limit() ? M_NOMONEY : M_NOROOM);
		return;
	}
	if (id >= W_PACK && id < W_PACK + 2)
	{
		if (pack_open(id - W_PACK)) { snd(7); ui.packReturn = SC_SHOP; ui_goto(SC_PACK); }
		else ui_msg(g.packType[id - W_PACK] ? M_NOMONEY : M_SOLDOUT);
		return;
	}
	switch (id)
	{
		case W_VOUCHER:
			if (voucher_buy()) { snd(9); shop_widgets(); draw_shop(); }
			else ui_msg(M_NOMONEY);
			break;
		case W_REROLL:
			if (shop_reroll()) { snd(8); shop_widgets(); draw_shop(); }
			else ui_msg(M_NOMONEY);
			break;
		case W_NEXT: next_blind(); ui_goto(SC_BLIND); break;
		case W_INFO: ui_goto(SC_INFO); break;
		case W_SELL:
			if (ui.itemKind == 1) joker_sell(ui.itemIdx); else cons_sell(ui.itemIdx);
			snd(4); ui.itemKind = 0; shop_widgets(); draw_shop();
			break;
		case W_USE:
			if (cons_use(ui.itemIdx, 0)) { snd(3); ui.itemKind = 0; shop_widgets(); draw_shop(); }
			else ui_msg(M_CANTUSE);
			break;
	}
}

void upd_shop(void) BANKED
{
	ui_pointer_focus();
	u16 p = in.pressed;
	if (p & IN_LEFT) ui_nav(IN_LEFT);
	if (p & IN_RIGHT) ui_nav(IN_RIGHT);
	if (p & IN_UP) ui_nav(IN_UP);
	if (p & IN_DOWN) ui_nav(IN_DOWN);
	if (in.click) { u8 h = ui_hit(in.mx, in.my); if (h != 0xFF) shop_activate(ui.w[h].id); else if (ui.itemKind) { ui.itemKind = 0; shop_widgets(); draw_shop(); } }
	else if ((p & IN_OK) && ui.focus != 0xFF) shop_activate(ui.w[ui.focus].id);
	if (p & IN_NEXT) shop_activate(W_NEXT);
	if (p & IN_INFO) ui_goto(SC_INFO);
	if (p & IN_BACK && ui.itemKind) { ui.itemKind = 0; shop_widgets(); draw_shop(); }
	if (ui.msgTimer && --ui.msgTimer == 0) { ui.msg = 0; ui.dirty |= D_INFO; }
	ui_info_tick();
}

//-----------------------------------------------------------------------------
// booster pack
//-----------------------------------------------------------------------------
static u8 pack_x(u8 i) { return (u8)((AREA_X + 98 - (g_packN * 34 - 10) / 2 + i * 34) & ~1u); }

static void pack_item(u8 i)
{
	u8 x = pack_x(i);
	bool foc = ui.focus != 0xFF && ui.w[ui.focus].id == W_PACKCARD + i;
	if (!g_packType[i]) { Vid_Fill(x, 60, 24, 32, COL_FELT); Vid_Frame(x, 60, 24, 32, COL_SLATE); return; }
	if (g_packType[i] == 4)
	{
		Vid_PlayCard(g_packCard[i], FALSE, x, 60);
		if (foc) Vid_Frame(x, 60, 24, 32, COL_GOLD);
		return;
	}
	if (g_packType[i] == 1) Vid_Joker(g_packId[i], x, 60);
	else Vid_Card(g_packType[i] == 2 ? CELL_PLANET + g_packId[i] : (g_packType[i] == 5 ? CELL_SPECTRAL + g_packId[i] : CELL_TAROT + g_packId[i]), x, 60);
	if (foc) Vid_Frame(x, 60, 24, 32, COL_GOLD);
}

void pack_focus(u8 o, u8 n) BANKED
{
	for (u8 k = 0; k < 2; k++)
	{
		u8 id = k ? n : o;
		if (id >= W_PACKCARD && id < W_PACKCARD + PACK_CARD_MAX) pack_item(id - W_PACKCARD);
		else if (id == W_SKIP) ui_button(W_SKIP, T_SKIP, COL_RED, TRUE);
	}
}

static void draw_pack(void)
{
	Vid_Fill(AREA_X, 0, AREA_W, 174, COL_FELT);
	{
		static const char* const nm[5] = { "Arcana Pack", "Celestial Pack", "Buffoon Pack", "Standard Pack", "Spectral Pack" };
		Vid_TextC(AREA_X + 96, 14, nm[(g_packKind - 1) / 3], TC_GOLD);
		char b[16] = "Choose "; b[7] = '0' + g_packPick; b[8] = 0;
		Vid_TextC(AREA_X + 96, 28, b, TC_WHITE);
	}
	for (u8 i = 0; i < g_packN; i++) pack_item(i);
	ui_button(W_SKIP, T_SKIP, COL_RED, TRUE);
	hud_mini();
	info_show(ui_focus_id());
	ui.dirty = 0;
}

void scr_pack(void) BANKED
{
	Vid_Clear(COL_FELT);
	ui_clear_widgets();
	hud_mini();
	for (u8 i = 0; i < g_packN; i++) ui_add(W_PACKCARD + i, pack_x(i), 60, 24, 32);
	ui_add(W_SKIP, AREA_X + 56, 120, 80, 18);
	ui.defFocus = W_PACKCARD;
	ui.focus = ui_find(W_PACKCARD);
	draw_pack();
}

void upd_pack(void) BANKED
{
	ui_pointer_focus();
	u16 p = in.pressed;
	if (p & IN_LEFT) ui_nav(IN_LEFT);
	if (p & IN_RIGHT) ui_nav(IN_RIGHT);
	if (p & IN_UP) ui_nav(IN_UP);
	if (p & IN_DOWN) ui_nav(IN_DOWN);
	u8 act = 0xFF;
	if (in.click) { u8 h = ui_hit(in.mx, in.my); if (h != 0xFF) act = ui.w[h].id; }
	else if ((p & IN_OK) && ui.focus != 0xFF) act = ui.w[ui.focus].id;
	if (p & IN_BACK) act = W_SKIP;
	if (act >= W_PACKCARD && act < W_PACKCARD + PACK_CARD_MAX)
	{
		if (pack_choose(act - W_PACKCARD))
		{
			snd(9);
			if (g_packPick == 0) { ui_goto(ui.packReturn); return; }
			draw_pack();
		}
		else ui_msg(g_packType[act - W_PACKCARD] == 1 ? M_NOJOKER : (g_packType[act - W_PACKCARD] == 4 ? M_CANTUSE : M_NOCONS));
	}
	else if (act == W_SKIP) { ui_goto(ui.packReturn); return; }
	if (ui.msgTimer && --ui.msgTimer == 0) { ui.msg = 0; ui.dirty |= D_INFO; }
	ui_info_tick();
}
