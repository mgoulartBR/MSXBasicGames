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
void ui_clear_widgets(void) BANKED { ui.nw = 0; ui.focus = 0xFF; ui.defFocus = 0xFF; }

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
	u8 o = ui_focus_id();
	ui.focus = idx;
	scr_focus(o, ui_focus_id());
	ui.dirty |= D_INFO;
	ui.infoDelay = 2;
}

// repaint the info panel once the focus has settled for a couple of frames
void ui_info_tick(void) BANKED
{
	if (!(ui.dirty & D_INFO)) return;
	if (ui.infoDelay) { ui.infoDelay--; return; }
	ui.dirty &= (u8)~D_INFO;
	if (ui.msgTimer == 0) ui.msg = 0;
	info_show(ui_focus_id());
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
	if (ui.focus == 0xFF) { u8 d = ui_find(ui.defFocus); ui_set_focus(d == 0xFF ? 0 : d); return; }
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

void ui_msg(const char* m) BANKED { ui.msg = m; ui.msgTimer = FR(100); ui.dirty |= D_INFO; Snd_Play(SFX_ERROR); }

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
	if (i == ui.focus) Vid_Frame(k->x + 1, k->y + 1, k->w - 2, k->h - 2, COL_GOLD);     // inset ring: no leftovers when focus moves
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
	Vid_Fill(4, 78, 26, 11, COL_BLUE);
	Vid_Fill(36, 78, 22, 11, COL_RED);
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
	Vid_Fill(62, 0, 2, 212, COL_SLATE);
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
	Vid_Fill(62, 0, 2, 212, COL_SLATE);
	Vid_Panel(2, 4, 58, 16, COL_SLATE, COL_INK);
	Vid_Text(6, 8, "$", TC_GOLD); Vid_NumR(57, 8, g.money, g.money < 0 ? TC_RED : TC_GOLD);
	Vid_Panel(2, 24, 58, 26, COL_SLATE, COL_INK);
	Vid_Text(6, 26, "Ante", TC_WHITE); Vid_NumR(g.endless ? 37 : 41, 26, g.ante, TC_GOLD); Vid_Text(g.endless ? 38 : 42, 26, g.endless ? "/12" : "/8", TC_SLATE);
	Vid_Text(6, 37, g.blind == BLIND_SMALL ? "Small" : (g.blind == BLIND_BIG ? "Big" : "Boss"), TC_WHITE);
	if (g.nTags)
	{
		Vid_Text(6, 56, "Tags", TC_SLATE);
		for (u8 i = 0; i < g.nTags; i++) Vid_TagIcon(g.tags[i], (u8)(4 + (i % 3) * 19), (u8)(67 + (i / 3) * 18));
	}
	ui_ensure(W_INFO, 2, 172, 58, 13);
	ui_button(W_INFO, T_RUNINFO, COL_ORANGE, TRUE);
}

//-----------------------------------------------------------------------------
// info panel (bottom right)
//-----------------------------------------------------------------------------
static void info_clear(void)
{
	Vid_Panel(AREA_X + 2, INFO_Y, 188, 35, COL_INK, COL_SLATE);
}

// "Cost $n" / "Sell $n" right aligned in the info panel
static void info_price(const char* label, u8 v)
{
	u8 w = Vid_NumW(v);
	Vid_Num(250 - w, INFO_Y + 3, v, TC_GOLD);
	Vid_Text(250 - w - 6, INFO_Y + 3, "$", TC_GOLD);
	Vid_Text(250 - w - 6 - Vid_TextW(label) - 3, INFO_Y + 3, label, TC_SLATE);
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
	if (C_ENH(c)) Vid_Text(AREA_X + 6, INFO_Y + 25, g_EnhText[C_ENH(c)], TC_GOLD);
	if (C_ED(c)) Vid_Text(250 - Vid_TextW(g_EdName[C_ED(c)]), INFO_Y + 3, g_EdName[C_ED(c)], TC_BLUE);
	if (C_SEAL(c))
	{
		if (!C_ENH(c)) Vid_Text(AREA_X + 6, INFO_Y + 25, g_SealName[C_SEAL(c)], TC_RED);
		else Vid_Text(250 - Vid_TextW(g_SealShort[C_SEAL(c)]), INFO_Y + 14, g_SealShort[C_SEAL(c)], TC_RED);
	}
}

static void info_jokerdef(const JokerDef* d, i8 sell, u8 price)
{
	Vid_Text(AREA_X + 6, INFO_Y + 3, d->name, TC_GOLD);
	if (sell >= 0) info_price("Sell", (u8)sell);
	else if (price) info_price("Cost", price);
	Vid_WrapDesc(d->desc, AREA_X + 6, INFO_Y + 14, 180, TC_WHITE, 2);
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
		const TarotDef* t = CONS_IS_SPECTRAL(c) ? &g_Spectrals[c - 0x40] : &g_Tarots[c - 0x20];
		Vid_Text(AREA_X + 6, INFO_Y + 3, t->name, TC_BLUE);
		Vid_WrapDesc(t->desc, AREA_X + 6, INFO_Y + 14, 180, TC_WHITE, 2);
	}
	if (price) info_price("Cost", price);
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
		Vid_WrapDesc(g_Bosses[g.boss].desc, AREA_X + 6, INFO_Y + 14, 180, TC_WHITE, 2);
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
		if (i < g.nHand) { if (g.dflag[g.hand[i]] & DF_FD) info_text("Face down card", 0); else info_card(g.deck[g.hand[i]]); }
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
				else if (g.shopType[i] == 2) { info_planet(g.shopId[i], FALSE); info_price("Cost", 3); }
				else if (g.shopType[i] == 3) info_cons(CONS_TAROT(g.shopId[i]), 3);
				else info_text("Sold out", 0);
			}
			else if (id == W_VOUCHER)
			{
				if (g.voucher)
				{
					Vid_Text(AREA_X + 6, INFO_Y + 3, g_Vouchers[g.voucher - 1].name, TC_GOLD);
					info_price("Cost", voucher_price());
					Vid_WrapDesc(g_Vouchers[g.voucher - 1].desc, AREA_X + 6, INFO_Y + 14, 180, TC_WHITE, 2);
				}
			}
			else if (id >= W_PACK && id < W_PACK + 2)
			{
				u8 k = g.packType[id - W_PACK];
				if (k)
				{
					static const char* const nm[5] = { "Arcana Pack", "Celestial Pack", "Buffoon Pack", "Standard Pack", "Spectral Pack" };
					static const char* const ds[5] = { "Choose from random Tarot cards.", "Choose from random Planet cards.", "Choose from random Jokers.", "Choose playing cards to add to your deck.", "Choose from random Spectral cards." };
					static const char* const sz[3] = { "", "Jumbo ", "Mega " };
					char b[28]; u8 n = 0; const char* p = sz[(k - 1) % 3];
					while (*p) b[n++] = *p++;
					p = nm[(k - 1) / 3]; while (*p) b[n++] = *p++;
					b[n] = 0;
					Vid_Text(AREA_X + 6, INFO_Y + 3, b, TC_BLUE);
					info_price("Cost", pack_price(id - W_PACK));
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
				else if (g_packType[i] == 4) info_card(g_packCard[i]);
				else if (g_packType[i] == 5) info_cons(CONS_SPECTRAL(g_packId[i]), 0);
			}
	}
}

//-----------------------------------------------------------------------------
// joker + consumable row (also used by the shop)
//-----------------------------------------------------------------------------
static u8 jslot_x(u8 id) { return id < W_CONS ? JOKER_X(id - W_JOKER) : CONS_X(id - W_CONS); }

void draw_jslot(u8 id) BANKED
{
	bool isJoker = id < W_CONS;
	u8 idx = isJoker ? id - W_JOKER : id - W_CONS;
	u8 x = jslot_x(id), y = JOKER_Y;
	bool present = isJoker ? idx < g.nJk : g.cons[idx] != 0;
	if (present && ui.itemKind == (isJoker ? 1 : 2) && ui.itemIdx == idx) y = JOKER_Y + 3;     // selected: lowered
	Vid_Fill(x, JOKER_Y, 24, 3, COL_FELT);
	if (!present) { Vid_Fill(x, JOKER_Y, 24, 32, COL_FELT); Vid_Frame(x, JOKER_Y, 24, 32, COL_SLATE); return; }
	if (isJoker) { Vid_Joker(g.jk[idx].id, x, y); Vid_EdStripe(g.jk[idx].ed, x, y); }
	else Vid_Card(CONS_IS_PLANET(g.cons[idx]) ? CELL_PLANET + g.cons[idx] - 1 : (CONS_IS_SPECTRAL(g.cons[idx]) ? CELL_SPECTRAL + g.cons[idx] - 0x40 : CELL_TAROT + g.cons[idx] - 0x20), x, y);
	if (isJoker && (g.jk[idx].flags & JF_DEBUFF)) Vid_Frame(x, y, 24, 32, COL_RED);
	if (ui_find(id) != 0xFF && ui_find(id) == ui.focus) Vid_Frame(x, y, 24, 32, COL_GOLD);     // ring inside the card border
}

void draw_joker_row(bool shop) BANKED
{
	Vid_Fill(AREA_X, 0, AREA_W, 52, COL_FELT);
	for (u8 i = 0; i < JOKER_MAX; i++) draw_jslot(W_JOKER + i);
	for (u8 i = 0; i < CONS_MAX; i++) draw_jslot(W_CONS + i);
	{ char s[6] = "0/5"; s[0] = '0' + g.nJk; Vid_Text(JOKER_X(0), 36, s, TC_SLATE); }
	{ char s[6] = "0/2"; s[0] = '0' + (g.cons[0] != 0) + (g.cons[1] != 0); Vid_Text(CONS_X(0), 36, s, TC_SLATE); }
	(void)shop;
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

void snd(u8 id) BANKED { Snd_Play(id); }

// scoring event sounds: chips/mult blips climb in pitch with the event number, like the original
void snd_event(u8 kind, u8 n) BANKED
{
	switch (kind)
	{
		case EV_CARD: case EV_CHIPS: Snd_PlayPitch(SFX_CHIPS, n); break;
		case EV_MULT: Snd_PlayPitch(SFX_MULT, n); break;
		case EV_XMULT: Snd_PlayPitch(SFX_XMULT, n); break;
		case EV_MONEY: Snd_Play(SFX_COIN); break;
		case EV_DEBUFF: Snd_Play(SFX_ERROR); break;
	}
}
