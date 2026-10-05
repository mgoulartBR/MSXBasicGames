// UI core: widget list + focus navigation, HUD panel, info panel, joker/consumable row.
#include "ui.h"

UI ui;

//-----------------------------------------------------------------------------
// names
//-----------------------------------------------------------------------------
static const char* const k_rank[13] = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace" };
static const char* const k_suit[4] = { "Hearts", "Clubs", "Diamonds", "Spades" };

//-----------------------------------------------------------------------------
// widgets
//-----------------------------------------------------------------------------
void ui_clear_widgets(void) BANKED { ui.nw = 0; ui.focus = 0xFF; }

void ui_add(u8 id, u8 x, u8 y, u8 w, u8 h) BANKED
{
	if (ui.nw >= WMAX) return;
	Widget* k = &ui.w[ui.nw++];
	k->id = id; k->x = x; k->y = y; k->w = w; k->h = h;
}

u8 ui_find(u8 id) BANKED
{
	for (u8 i = 0; i < ui.nw; i++) if (ui.w[i].id == id) return i;
	return 0xFF;
}

void ui_ensure(u8 id, u8 x, u8 y, u8 w, u8 h) BANKED { if (ui_find(id) == 0xFF) ui_add(id, x, y, w, h); }

u8 ui_focus_id(void) BANKED { return ui.focus == 0xFF ? 0xFF : ui.w[ui.focus].id; }

void ui_set_focus(u8 idx) BANKED
{
	if (idx == ui.focus) return;
	ui.focus = idx;
	ui.dirty |= D_HAND | D_JOKERS | D_BUTTONS | D_INFO | D_PLAY;
}

// topmost widget under the pointer (later widgets are drawn on top)
u8 ui_hit(u8 x, u8 y) BANKED
{
	for (i8 i = (i8)ui.nw - 1; i >= 0; i--)
	{
		Widget* k = &ui.w[i];
		if (x >= k->x && x < (u8)(k->x + k->w) && y >= k->y && y < (u8)(k->y + k->h)) return (u8)i;
	}
	return 0xFF;
}

bool ui_pointer_focus(void) BANKED
{
	if (!in.mouse || !in.moved) return FALSE;
	u8 h = ui_hit(in.mx, in.my);
	if (h == ui.focus) return FALSE;
	ui_set_focus(h);
	return TRUE;
}

// keyboard / joystick navigation: nearest widget in the pressed direction
void ui_nav(u8 dir) BANKED
{
	if (!ui.nw) return;
	if (ui.focus == 0xFF) { ui_set_focus(0); return; }
	Widget* c = &ui.w[ui.focus];
	i16 cx = c->x + c->w / 2, cy = c->y + c->h / 2;
	u8 best = 0xFF; i16 bs = 32000;
	for (u8 i = 0; i < ui.nw; i++)
	{
		if (i == ui.focus) continue;
		Widget* k = &ui.w[i];
		i16 dx = (i16)(k->x + k->w / 2) - cx, dy = (i16)(k->y + k->h / 2) - cy;
		i16 prim, sec;
		switch (dir)
		{
			case IN_UP:    prim = -dy; sec = dx; break;
			case IN_DOWN:  prim = dy;  sec = dx; break;
			case IN_LEFT:  prim = -dx; sec = dy; break;
			default:       prim = dx;  sec = dy; break;
		}
		if (prim < 3) continue;
		if (sec < 0) sec = -sec;
		i16 sc = prim + sec * 3;
		if (sc < bs) { bs = sc; best = i; }
	}
	if (best != 0xFF) ui_set_focus(best);
}

void ui_msg(const char* m) BANKED { ui.msg = m; ui.msgTimer = FR(100); ui.dirty |= D_INFO; }

//-----------------------------------------------------------------------------
// buttons
//-----------------------------------------------------------------------------
void ui_focus_ring(u8 id) BANKED
{
	u8 i = ui_find(id);
	if (i == 0xFF) return;
	Widget* k = &ui.w[i];
	Vid_Frame(k->x - 1, k->y - 1, k->w + 2, k->h + 2, COL_GOLD);
}

void ui_button(u8 id, const char* label, u8 col, bool enabled) BANKED
{
	u8 i = ui_find(id);
	if (i == 0xFF) return;
	Widget* k = &ui.w[i];
	Vid_Panel(k->x, k->y, k->w, k->h, enabled ? col : COL_SLATE, COL_INK);
	Vid_TextC(k->x + k->w / 2, k->y + (k->h - 9) / 2, label, enabled ? TC_WHITE : TC_INK);
	if (i == ui.focus) ui_focus_ring(id);
}

//-----------------------------------------------------------------------------
// HUD (left panel)
//-----------------------------------------------------------------------------
static i32 c_score; static i16 c_money; static u8 c_hands, c_disc, c_deck;

static void hud_fields(bool all)
{
	if (all || c_score != ui.shownScore)
	{
		Vid_Fill(4, 51, 54, 11, COL_INK);
		Vid_NumR(57, 52, ui.shownScore, TC_WHITE);
		c_score = ui.shownScore;
	}
	if (all || c_money != g.money)
	{
		Vid_Fill(4, 128, 54, 11, COL_INK);
		Vid_Text(6, 129, "$", TC_GOLD);
		Vid_NumR(57, 129, g.money, g.money < 0 ? TC_RED : TC_GOLD);
		c_money = g.money;
	}
	if (all || c_hands != g.handsLeft || c_disc != g.discardsLeft)
	{
		Vid_Fill(4, 104, 26, 13, COL_BLUE); Vid_NumR(27, 106, g.handsLeft, TC_WHITE);
		Vid_Fill(32, 104, 26, 13, COL_RED); Vid_NumR(55, 106, g.discardsLeft, TC_WHITE);
		c_hands = g.handsLeft; c_disc = g.discardsLeft;
	}
	if (all || c_deck != g.nPile)
	{
		Vid_Fill(4, 200, 56, 9, COL_INK);
		Vid_Text(6, 200, "Deck", TC_SLATE);
		Vid_NumR(57, 200, g.nPile, TC_WHITE);
		c_deck = g.nPile;
	}
}

void hud_hand(u8 type, u16 chips, u16 mult) BANKED
{
	Vid_Fill(4, 68, 54, 9, COL_SLATE);
	Vid_Fill(5, 78, 25, 11, COL_BLUE);
	Vid_Fill(36, 78, 21, 11, COL_RED);
	if (type != 0xFF)
	{
		Vid_Text(6, 68, g_Hands[type].name, TC_WHITE);
		Vid_NumR(29, 79, chips, TC_WHITE);
		Vid_NumR(56, 79, mult, TC_WHITE);
	}
	Vid_Text(31, 79, "x", TC_INK);
	ui.shownHand = type; ui.shownChips = chips; ui.shownMult = mult;
}

void hud_draw(void) BANKED
{
	Vid_Fill(0, 0, 62, 212, COL_INK);
	Vid_Fill(62, 0, 1, 212, COL_SLATE);
	// blind
	Vid_Panel(2, 2, 58, 36, COL_SLATE, COL_INK);
	u8 bi = g.blind == BLIND_SMALL ? g_BlindIcon[0] : (g.blind == BLIND_BIG ? g_BlindIcon[1] : g_Bosses[g.boss].icon);
	Vid_BlindIcon(bi, 5, 5);
	const char* bn = g.blind == BLIND_BOSS ? g_Bosses[g.boss].name : (g.blind == BLIND_SMALL ? "Small" : "Big");
	if (bn[0] == 'T' && bn[1] == 'h' && bn[2] == 'e' && bn[3] == ' ') bn += 4;
	Vid_TextN(24, 6, bn, 6, TC_WHITE);
	Vid_Num(24, 17, (i32)blind_target(), TC_RED);
	{ char d[9]; u8 r = blind_reward(), n = 0; while (n < r && n < 8) d[n++] = '$'; d[n] = 0; Vid_Text(6, 26, d, TC_GOLD); }
	// round score
	Vid_Panel(2, 40, 58, 24, COL_SLATE, COL_INK);
	Vid_Text(6, 41, "Round score", TC_WHITE);
	// hand preview
	Vid_Panel(2, 66, 58, 25, COL_SLATE, COL_INK);
	// hands / discards
	Vid_Panel(2, 93, 58, 29, COL_SLATE, COL_INK);
	Vid_Text(6, 94, "Hands", TC_WHITE); Vid_Text(34, 94, "Disc", TC_WHITE);
	// money
	Vid_Panel(2, 125, 58, 16, COL_SLATE, COL_INK);
	// ante / round
	Vid_Panel(2, 144, 58, 24, COL_SLATE, COL_INK);
	Vid_Text(6, 146, "Ante", TC_WHITE); Vid_NumR(41, 146, g.ante, TC_GOLD); Vid_Text(42, 146, "/8", TC_SLATE);
	Vid_Text(6, 157, g.blind == BLIND_SMALL ? "Small" : (g.blind == BLIND_BIG ? "Big" : "Boss"), TC_WHITE);
	// run info button
	ui_ensure(W_INFO, 2, 172, 58, 13);
	ui_button(W_INFO, T_RUNINFO, COL_ORANGE, TRUE);
	hud_fields(TRUE);
	hud_hand(ui.shownHand, ui.shownChips, ui.shownMult);
}

void hud_update(void) BANKED { hud_fields(FALSE); }

void hud_mini(void) BANKED
{
	Vid_Fill(0, 0, 62, 212, COL_INK);
	Vid_Fill(62, 0, 1, 212, COL_SLATE);
	Vid_Panel(2, 4, 58, 16, COL_SLATE, COL_INK);
	Vid_Text(6, 8, "$", TC_GOLD); Vid_NumR(57, 8, g.money, g.money < 0 ? TC_RED : TC_GOLD);
	Vid_Panel(2, 24, 58, 26, COL_SLATE, COL_INK);
	Vid_Text(6, 26, "Ante", TC_WHITE); Vid_NumR(41, 26, g.ante > MAX_ANTE ? MAX_ANTE : g.ante, TC_GOLD); Vid_Text(42, 26, "/8", TC_SLATE);
	Vid_Text(6, 37, g.blind == BLIND_SMALL ? "Small" : (g.blind == BLIND_BIG ? "Big" : "Boss"), TC_WHITE);
	ui_ensure(W_INFO, 2, 172, 58, 13);
	ui_button(W_INFO, T_RUNINFO, COL_ORANGE, TRUE);
}

//-----------------------------------------------------------------------------
// info panel (bottom right)
//-----------------------------------------------------------------------------
static void info_clear(void)
{
	Vid_Panel(AREA_X + 1, INFO_Y, 190, 35, COL_INK, COL_SLATE);
}

static void info_card(Card c)
{
	char b[24]; u8 n = 0;
	const char* r = k_rank[C_RANK(c)];
	while (*r) b[n++] = *r++;
	b[n++] = ' '; b[n++] = 'o'; b[n++] = 'f'; b[n++] = ' ';
	const char* s = k_suit[C_SUIT(c)];
	while (*s) b[n++] = *s++;
	b[n] = 0;
	Vid_Text(AREA_X + 6, INFO_Y + 3, b, TC_WHITE);
	Vid_Text(AREA_X + 6, INFO_Y + 14, "+", TC_BLUE);
	Vid_Num(AREA_X + 12, INFO_Y + 14, C_NOMINAL(c), TC_BLUE);
	Vid_Text(AREA_X + 12 + Vid_NumW(C_NOMINAL(c)) + 3, INFO_Y + 14, "chips when scored", TC_SLATE);
}

static void info_jokerdef(const JokerDef* d, i8 sell, u8 price)
{
	Vid_Text(AREA_X + 6, INFO_Y + 3, d->name, TC_GOLD);
	if (sell >= 0) { Vid_Text(AREA_X + 150, INFO_Y + 3, "Sell $", TC_SLATE); Vid_Num(AREA_X + 150 + 32, INFO_Y + 3, sell, TC_GOLD); }
	else if (price) { Vid_Text(AREA_X + 160, INFO_Y + 3, "Cost $", TC_SLATE); Vid_Num(AREA_X + 160 + 32, INFO_Y + 3, price, TC_GOLD); }
	Vid_Wrap(AREA_X + 6, INFO_Y + 14, d->desc, 180, TC_WHITE, 2);
}

static void info_planet(u8 h, bool owned)
{
	(void)owned;
	Vid_Text(AREA_X + 6, INFO_Y + 3, g_Hands[h].name, TC_BLUE);
	Vid_Text(AREA_X + 6, INFO_Y + 14, "Level up:", TC_SLATE);
	Vid_Text(AREA_X + 52, INFO_Y + 14, "+", TC_BLUE); Vid_Num(AREA_X + 57, INFO_Y + 14, g_Hands[h].lvlChips, TC_BLUE);
	Vid_Text(AREA_X + 80, INFO_Y + 14, "chips", TC_BLUE);
	Vid_Text(AREA_X + 52, INFO_Y + 24, "+", TC_RED); Vid_Num(AREA_X + 57, INFO_Y + 24, g_Hands[h].lvlMult, TC_RED);
	Vid_Text(AREA_X + 80, INFO_Y + 24, "mult", TC_RED);
	Vid_Text(AREA_X + 130, INFO_Y + 14, "Lv", TC_SLATE); Vid_Num(AREA_X + 144, INFO_Y + 14, g.handLevel[h], TC_WHITE);
}

static void info_cons(u8 c, u8 price)
{
	if (CONS_IS_PLANET(c)) { info_planet(c - 1, TRUE); }
	else
	{
		const TarotDef* t = &g_Tarots[c - 0x20];
		Vid_Text(AREA_X + 6, INFO_Y + 3, t->name, TC_BLUE);
		Vid_Wrap(AREA_X + 6, INFO_Y + 14, t->desc, 180, TC_WHITE, 2);
	}
	if (price) { Vid_Text(AREA_X + 160, INFO_Y + 3, "Cost $", TC_SLATE); Vid_Num(AREA_X + 192, INFO_Y + 3, price, TC_GOLD); }
}

static void info_text(const char* a, const char* b)
{
	Vid_Text(AREA_X + 6, INFO_Y + 3, a, TC_WHITE);
	if (b) Vid_Wrap(AREA_X + 6, INFO_Y + 14, b, 180, TC_SLATE, 2);
}

void info_blind(void) BANKED
{
	if (g.blind == BLIND_BOSS)
	{
		Vid_Text(AREA_X + 6, INFO_Y + 3, g_Bosses[g.boss].name, TC_RED);
		Vid_Wrap(AREA_X + 6, INFO_Y + 14, g_Bosses[g.boss].desc, 180, TC_WHITE, 2);
	}
	else info_text(g.blind == BLIND_SMALL ? "Small Blind" : "Big Blind", "Reach the target score before you run out of hands.");
}

void info_show(u8 id) BANKED
{
	info_clear();
	if (ui.msgTimer && ui.msg) { Vid_TextC(AREA_X + 96, INFO_Y + 13, ui.msg, TC_GOLD); return; }
	if (id == 0xFF) { if (ui.screen == SC_ROUND) info_blind(); return; }
	if (id < W_JOKER)
	{
		u8 i = id - W_HAND;
		if (i < g.nHand) info_card(g.deck[g.hand[i]]);
	}
	else if (id < W_CONS) { u8 i = id - W_JOKER; if (i < g.nJk) info_jokerdef(&g_Jokers[g.jk[i].id], (i8)joker_sell_value(i), 0); else info_text("Empty Joker slot", 0); }
	else if (id < W_PLAY) { u8 i = id - W_CONS; if (g.cons[i]) info_cons(g.cons[i], 0); else info_text("Empty consumable slot", 0); }
	else switch (id)
	{
		case W_PLAY:      info_text("Play Hand", "Score the selected cards (1-5)."); break;
		case W_DISCARD:   info_text("Discard", "Throw away the selected cards and draw new ones."); break;
		case W_SORT_RANK: info_text("Sort by Rank", "Highest rank on the left."); break;
		case W_SORT_SUIT: info_text("Sort by Suit", "Group cards by suit."); break;
		case W_INFO:      info_text("Run Info", "Poker hand levels and statistics."); break;
		case W_SELL:      info_text("Sell", "Sell this card for money."); break;
		case W_USE:       info_text("Use", "Use this consumable now."); break;
		case W_REROLL:    info_text("Reroll", "Get a new set of shop cards. The price grows each time."); break;
		case W_NEXT:      info_text("Next Round", "Leave the shop and pick the next Blind."); break;
		default:
			if (id >= W_SHOPCARD && id < W_SHOPCARD + SHOP_CARD_MAX)
			{
				u8 i = id - W_SHOPCARD;
				if (g.shopType[i] == 1) info_jokerdef(&g_Jokers[g.shopId[i]], -1, g_Jokers[g.shopId[i]].cost);
				else if (g.shopType[i] == 2) { info_planet(g.shopId[i], FALSE); Vid_Text(AREA_X + 160, INFO_Y + 3, "Cost $3", TC_GOLD); }
				else if (g.shopType[i] == 3) info_cons(CONS_TAROT(g.shopId[i]), 3);
				else info_text("Sold out", 0);
			}
			else if (id >= W_PACK && id < W_PACK + 2)
			{
				u8 k = g.packType[id - W_PACK];
				if (k)
				{
					static const char* const nm[3] = { "Arcana Pack", "Celestial Pack", "Buffoon Pack" };
					static const char* const ds[3] = { "Choose from random Tarot cards.", "Choose from random Planet cards.", "Choose from random Jokers." };
					static const char* const sz[3] = { "", "Jumbo ", "Mega " };
					char b[28]; u8 n = 0; const char* p = sz[(k - 1) % 3];
					while (*p) b[n++] = *p++;
					p = nm[(k - 1) / 3]; while (*p) b[n++] = *p++;
					b[n] = 0;
					Vid_Text(AREA_X + 6, INFO_Y + 3, b, TC_BLUE);
					Vid_Text(AREA_X + 160, INFO_Y + 3, "Cost $", TC_SLATE); Vid_Num(AREA_X + 192, INFO_Y + 3, pack_cost(k), TC_GOLD);
					Vid_Wrap(AREA_X + 6, INFO_Y + 14, ds[(k - 1) / 3], 180, TC_WHITE, 2);
				}
				else info_text("Sold out", 0);
			}
			else if (id >= W_PACKCARD && id < W_PACKCARD + PACK_CARD_MAX)
			{
				u8 i = id - W_PACKCARD;
				if (g_packType[i] == 1) info_jokerdef(&g_Jokers[g_packId[i]], -1, 0);
				else if (g_packType[i] == 2) info_planet(g_packId[i], FALSE);
				else if (g_packType[i] == 3) info_cons(CONS_TAROT(g_packId[i]), 0);
			}
	}
}

//-----------------------------------------------------------------------------
// joker + consumable row (also used by the shop)
//-----------------------------------------------------------------------------
void draw_joker_row(bool shop) BANKED
{
	Vid_Fill(AREA_X + 1, 0, 191, 38, COL_FELT);
	for (u8 i = 0; i < JOKER_MAX; i++)
	{
		u8 x = JOKER_X(i), y = JOKER_Y;
		if (i < g.nJk)
		{
			bool sel = ui.itemKind == 1 && ui.itemIdx == i;
			if (sel) y = JOKER_Y + 3;
			Vid_Card(g_Jokers[g.jk[i].id].cell, x, y);
			if (g.jk[i].flags & JF_DEBUFF) Vid_Frame(x, y, 24, 32, COL_RED);
		}
		else Vid_Frame(x, y, 24, 32, COL_SLATE);
	}
	for (u8 i = 0; i < CONS_MAX; i++)
	{
		u8 x = CONS_X(i), y = JOKER_Y;
		if (g.cons[i])
		{
			if (ui.itemKind == 2 && ui.itemIdx == i) y = JOKER_Y + 3;
			Vid_Card(CONS_IS_PLANET(g.cons[i]) ? CELL_PLANET + g.cons[i] - 1 : CELL_TAROT + g.cons[i] - 0x20, x, y);
		}
		else Vid_Frame(x, y, 24, 32, COL_SLATE);
	}
	{ char s[6] = "0/5"; s[0] = '0' + g.nJk; Vid_Text(JOKER_X(0), 36, s, TC_SLATE); }
	{ char s[6] = "0/2"; s[0] = '0' + (g.cons[0] != 0) + (g.cons[1] != 0); Vid_Text(CONS_X(0), 36, s, TC_SLATE); }
	(void)shop;
	// focus ring + action buttons
	for (u8 i = 0; i < JOKER_MAX + CONS_MAX; i++)
	{
		u8 id = i < JOKER_MAX ? W_JOKER + i : W_CONS + (i - JOKER_MAX);
		if (ui_find(id) != 0xFF && ui_find(id) == ui.focus) ui_focus_ring(id);
	}
	if (ui.itemKind)
	{
		u8 x = ui.itemKind == 1 ? JOKER_X(ui.itemIdx) : CONS_X(ui.itemIdx);
		u8 sx = x > 150 ? 150 : x;
		char b[12] = "Sell $";
		u8 val = ui.itemKind == 1 ? joker_sell_value(ui.itemIdx) : 1;
		u8 n = 6; if (val >= 10) b[n++] = '0' + val / 10; b[n++] = '0' + val % 10; b[n] = 0;
		u8 si = ui_find(W_SELL), ui2 = ui_find(W_USE);
		if (si != 0xFF) { ui.w[si].x = sx; ui.w[si].y = 38; ui.w[si].w = 38; ui.w[si].h = 12; ui_button(W_SELL, b, COL_ORANGE, TRUE); }
		if (ui2 != 0xFF && ui.itemKind == 2) { ui.w[ui2].x = sx + 40; ui.w[ui2].y = 38; ui.w[ui2].w = 30; ui.w[ui2].h = 12; ui_button(W_USE, T_USE, COL_GREEN, TRUE); }
	}
}

// joker/consumable under a widget id: returns kind in high nibble (1 joker, 2 cons) and index
u8 joker_item_at(u8 id) BANKED
{
	if (id >= W_JOKER && id < W_JOKER + JOKER_MAX) return (u8)(0x10 | (id - W_JOKER));
	if (id >= W_CONS && id < W_CONS + CONS_MAX) return (u8)(0x20 | (id - W_CONS));
	return 0;
}

void snd(u8 id) BANKED { (void)id; }
