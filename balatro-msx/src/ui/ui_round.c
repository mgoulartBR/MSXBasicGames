// The round screen: hand, jokers, play/discard, scoring animation.
#include "ui.h"


static u8 hstep, hx0;
static u8 pop_x, pop_y, pop_w;       // last popup rectangle (0 = none)

//-----------------------------------------------------------------------------
// geometry
//-----------------------------------------------------------------------------
static void hand_geom(void)
{
	u8 n = g.nHand;
	if (n <= 1) { hstep = 0; hx0 = AREA_X + 2 + 82; return; }
	u16 st = (188 - 24) / (n - 1);
	if (st > 26) st = 26;
	hstep = (u8)st;
	hx0 = (u8)(AREA_X + 2 + (188 - (24 + hstep * (n - 1))) / 2);
}
static u8 hand_x(u8 i) { return (u8)(hx0 + i * hstep); }
static u8 played_x(u8 i) { u8 n = g.nPlayed; return (u8)(AREA_X + 98 - (n * 28 - 4) / 2 + i * 28); }

static u8 popcnt16(u16 m) { u8 n = 0; while (m) { n += (u8)(m & 1); m >>= 1; } return n; }

//-----------------------------------------------------------------------------
// widgets
//-----------------------------------------------------------------------------
static void rebuild_widgets(void)
{
	ui_clear_widgets();
	hand_geom();
	for (u8 i = 0; i < g.nHand; i++)
		ui_add(W_HAND + i, hand_x(i), HAND_Y - HAND_RAISE, (u8)(i + 1 < g.nHand && hstep < 24 ? hstep : 24), 32 + HAND_RAISE);
	for (u8 i = 0; i < g.nJk; i++) ui_add(W_JOKER + i, JOKER_X(i), JOKER_Y, 24, 32);
	for (u8 i = 0; i < CONS_MAX; i++) if (g.cons[i]) ui_add(W_CONS + i, CONS_X(i), JOKER_Y, 24, 32);
	ui_add(W_PLAY, 66, BTN_Y, 44, BTN_H);
	ui_add(W_DISCARD, 112, BTN_Y, 44, BTN_H);
	ui_add(W_SORT_RANK, 158, BTN_Y, 44, BTN_H);
	ui_add(W_SORT_SUIT, 204, BTN_Y, 44, BTN_H);
	ui_add(W_INFO, 2, 172, 58, 13);
	ui.defFocus = W_HAND;
	if (ui.itemKind)
	{
		ui_add(W_SELL, 0, 0, 38, 12);
		if (ui.itemKind == 2) ui_add(W_USE, 0, 0, 30, 12);
	}
}

//-----------------------------------------------------------------------------
// drawing
//-----------------------------------------------------------------------------
static void draw_hand(void)
{
	Vid_Fill(AREA_X + 1, HAND_Y - HAND_RAISE - 4, 191, 32 + HAND_RAISE + 8, COL_FELT);
	u8 fi = ui_focus_id(), foc = 0xFF;
	for (u8 i = 0; i < g.nHand; i++)
	{
		u8 slot = g.hand[i];
		u8 y = (ui.sel & (1u << i)) ? HAND_Y - HAND_RAISE : HAND_Y;
		if (fi == W_HAND + i) { foc = i; continue; }          // focused card is drawn last (on top)
		Vid_Card(C_CELL(g.deck[slot]), hand_x(i), y);
		if (card_is_debuffed(slot)) { Vid_Frame(hand_x(i), y, 24, 32, COL_RED); }
	}
	if (foc != 0xFF)
	{
		u8 y = (ui.sel & (1u << foc)) ? HAND_Y - HAND_RAISE : HAND_Y;
		y -= 2;
		Vid_Card(C_CELL(g.deck[g.hand[foc]]), hand_x(foc), y);
		Vid_Frame(hand_x(foc) - 1, y - 1, 26, 34, COL_GOLD);
	}
}

static void draw_buttons(void)
{
	Vid_Fill(AREA_X + 1, BTN_Y - 3, 191, BTN_H + 6, COL_FELT);
	bool canPlay = ui.phase == PH_INPUT && round_can_play(ui.sel);
	bool canDisc = ui.phase == PH_INPUT && g.discardsLeft > 0 && ui.sel != 0;
	ui_button(W_PLAY, T_PLAY, COL_BLUE, canPlay);
	ui_button(W_DISCARD, T_DISCARD, COL_RED, canDisc);
	ui_button(W_SORT_RANK, T_RANK, COL_ORANGE, TRUE);
	ui_button(W_SORT_SUIT, T_SUIT, COL_ORANGE, TRUE);
}

static void draw_played(u8 hil)
{
	Vid_Fill(AREA_X + 1, PLAY_Y - 6, 191, 32 + 10, COL_FELT);
	for (u8 i = 0; i < g.nPlayed; i++)
	{
		bool scoring = (ui.so.mask >> i) & 1;
		u8 y = scoring ? PLAY_Y - 4 : PLAY_Y + 4;
		Vid_Card(C_CELL(g.deck[g.played[i]]), played_x(i), y);
		if (hil == i) Vid_Frame(played_x(i) - 1, y - 1, 26, 34, COL_GOLD);
	}
}

static void draw_all(void)
{
	Vid_Fill(AREA_X + 1, 0, 191, 174, COL_FELT);
	draw_joker_row(FALSE);
	draw_hand();
	draw_buttons();
	info_show(ui_focus_id());
	hud_update();
}

static void preview_hand(void)
{
	if (!ui.sel) { hud_hand(0xFF, 0, 0); return; }
	Card c[PLAY_MAX]; u8 n = 0;
	for (u8 i = 0; i < g.nHand && n < PLAY_MAX; i++) if (ui.sel & (1u << i)) c[n++] = g.deck[g.hand[i]];
	HandEval he; poker_eval(c, n, poker_rules(), &he);
	hud_hand(he.type, (u16)hand_chips(he.type), hand_mult(he.type));
}

static void clear_popup(void)
{
	if (pop_w) { Vid_Fill(pop_x, pop_y, pop_w, 11, COL_FELT); pop_w = 0; }
}

static void popup(u8 cx, u8 y, const char* s, u8 tc)
{
	clear_popup();
	u8 w = Vid_TextW(s) + 6;
	u8 x = cx > w / 2 + AREA_X ? cx - w / 2 : AREA_X + 1;
	if (x + w > 254) x = 254 - w;
	Vid_Panel(x, y, w, 11, COL_INK, COL_SLATE);
	Vid_Text(x + 3, y + 1, s, tc);
	pop_x = x; pop_y = y; pop_w = w;
}

static void fmt_signed(char* b, char sign, i32 v)
{
	u8 n = 0; char t[11]; u8 k = 0;
	b[n++] = sign;
	if (v < 0) { b[n++] = '-'; v = -v; }
	do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v);
	while (k) b[n++] = t[--k];
	b[n] = 0;
}
static void fmt_x(char* b, i16 x100)
{
	u8 n = 0;
	b[n++] = 'X';
	b[n++] = (char)('0' + x100 / 100);
	if (x100 % 100)
	{
		b[n++] = '.';
		b[n++] = (char)('0' + (x100 % 100) / 10);
		if (x100 % 10) b[n++] = (char)('0' + x100 % 10);
	}
	b[n] = 0;
}

//-----------------------------------------------------------------------------
// events -> animation
//-----------------------------------------------------------------------------
static void play_event(const Ev* e)
{
	char b[14];
	u8 ax = AREA_X + 96, ay = 98;
	u8 hil = 0xFF;
	if (e->src < 0x20) { ax = played_x(e->src) + 12; ay = PLAY_Y + 36; hil = e->src; }
	else if (e->src >= 0x40 && e->src != SRC_NONE) { ax = JOKER_X(e->src & 0x3F) + 12; ay = 38; }
	switch (e->kind)
	{
		case EV_BASE: hud_hand(ui.so.type, e->chips, e->mult); draw_played(0xFF); return;
		case EV_CARD: draw_played(hil); fmt_signed(b, '+', e->val); popup(ax, ay, b, TC_BLUE); break;
		case EV_CHIPS: fmt_signed(b, '+', e->val); popup(ax, ay, b, TC_BLUE); break;
		case EV_MULT:  fmt_signed(b, '+', e->val); popup(ax, ay, b, TC_RED); break;
		case EV_XMULT: fmt_x(b, e->val); popup(ax, ay, b, TC_RED); break;
		case EV_MONEY: fmt_signed(b, '$', e->val); popup(ax, ay, b, e->val < 0 ? TC_RED : TC_GOLD); break;
		case EV_TEXT:  popup(ax, ay, g_TextMsg[e->val], TC_WHITE); break;
		case EV_DEBUFF: if (hil != 0xFF) draw_played(hil); popup(ax, ay, "Debuffed", TC_RED); break;
	}
	hud_hand(ui.so.type, e->chips, e->mult);
	hud_update();
	snd_event(e->kind, ui.evi);
}

//-----------------------------------------------------------------------------
// actions
//-----------------------------------------------------------------------------
static void after_hand_change(void)
{
	ui.sel = 0;
	if (g.forced != 0xFF) for (u8 i = 0; i < g.nHand; i++) if (g.hand[i] == g.forced) ui.sel |= (u16)(1u << i);
	rebuild_widgets();
	ui.focus = 0xFF;
	ui.dirty |= D_ALL;
	preview_hand();
}

static void toggle_card(u8 i)
{
	if (i >= g.nHand) return;
	u16 b = (u16)(1u << i);
	if (ui.sel & b)
	{
		if (g.forced == g.hand[i]) return;                 // Cerulean Bell
		ui.sel &= (u16)~b;
	}
	else
	{
		if (popcnt16(ui.sel) >= PLAY_MAX) return;
		ui.sel |= b;
	}
	snd(1);
	preview_hand();
	ui.dirty |= D_HAND | D_BUTTONS;
}

static void end_of_round(void)
{
	if (g.state == ROUND_WON)
	{
		ui.phase = PH_BANNER; ui.timer = FR(70);
		Vid_Panel(AREA_X + 40, 90, 112, 30, COL_INK, COL_GOLD);
		Vid_TextC(AREA_X + 96, 98, g.blind == BLIND_BOSS ? "Boss defeated!" : "Blind defeated!", TC_GOLD);
		snd(SFX_WIN);
	}
	else if (g.state == ROUND_LOST) { snd(SFX_LOSE); ui_goto(SC_OVER); }
}

static void start_play(void)
{
	if (!round_can_play(ui.sel)) { return; }
	round_play(ui.sel, &ui.so);
	ui.sel = 0;
	preview_hand();
	rebuild_widgets();
	ui.phase = PH_SCORING; ui.evi = 0; ui.timer = FR(6);
	pop_w = 0;
	ui.dirty = D_HAND | D_BUTTONS | D_JOKERS;
	Vid_Fill(AREA_X + 1, 38, 191, 76, COL_FELT);
	draw_joker_row(FALSE);
	draw_hand();
	draw_buttons();
	draw_played(0xFF);
	hud_hand(ui.so.type, 0, 0);
}

static void do_discard(void)
{
	if (g.discardsLeft == 0 || !ui.sel) return;
	if (!round_discard(ui.sel)) return;
	snd(2);
	after_hand_change();
	if (g.state != ROUND_PLAYING) end_of_round();
}

static void use_item(void)
{
	if (ui.itemKind != 2) return;
	u8 slot = ui.itemIdx;
	u8 c = g.cons[slot], mn, mx;
	if (cons_needs_cards(c, &mn, &mx))
	{
		u8 n = popcnt16(ui.sel);
		if (n < mn || n > mx) { ui_msg(M_NEEDCARDS); return; }
	}
	if (!cons_use(slot, ui.sel)) { ui_msg(M_CANTUSE); return; }
	snd(3);
	ui.itemKind = 0;
	after_hand_change();
}

static void sell_item(void)
{
	if (ui.itemKind == 1) { joker_sell(ui.itemIdx); }
	else if (ui.itemKind == 2) cons_sell(ui.itemIdx);
	snd(4);
	ui.itemKind = 0;
	rebuild_widgets();
	ui.focus = 0xFF;
	ui.dirty |= D_ALL;
}

static void activate(u8 id)
{
	if (id < W_HAND + HAND_MAX) { toggle_card(id - W_HAND); return; }
	if (id >= W_JOKER && id < W_PLAY)
	{
		u8 it = joker_item_at(id);
		u8 kind = it >> 4, idx = it & 15;
		if (ui.itemKind == kind && ui.itemIdx == idx) ui.itemKind = 0; else { ui.itemKind = kind; ui.itemIdx = idx; }
		u8 f = ui_focus_id();
		rebuild_widgets();
		ui.focus = ui_find(f);
		ui.dirty |= D_JOKERS;
		return;
	}
	switch (id)
	{
		case W_PLAY: start_play(); break;
		case W_DISCARD: do_discard(); break;
		case W_SORT_RANK: hand_sort(0); after_hand_change(); break;
		case W_SORT_SUIT: hand_sort(1); after_hand_change(); break;
		case W_INFO: ui_goto(SC_INFO); break;
		case W_SELL: sell_item(); break;
		case W_USE: use_item(); break;
	}
}

//-----------------------------------------------------------------------------
// screen
//-----------------------------------------------------------------------------
void scr_round(void) BANKED
{
	Vid_Clear(COL_FELT);
	ui.itemKind = 0; ui.sel = 0; ui.phase = PH_INPUT; ui.shownScore = g.score; ui.shownHand = 0xFF; ui.shownChips = 0; ui.shownMult = 0;
	rebuild_widgets();
	hud_draw();
	after_hand_change();
	draw_all();
}

void upd_round(void) BANKED
{
	switch (ui.phase)
	{
		case PH_INPUT:
		{
			if (ui_pointer_focus()) {}
			u16 p = in.pressed;
			u8 fi = ui_focus_id();
			if (p & (IN_LEFT | IN_RIGHT))
			{
				// inside the hand the arrows move card by card
				if (fi < W_JOKER && g.nHand) { i8 d = (p & IN_LEFT) ? -1 : 1; i8 k = (i8)((fi == 0xFF ? 0 : (i8)fi) + d); if (k < 0) k = 0; if (k >= (i8)g.nHand) k = (i8)g.nHand - 1; ui_set_focus(ui_find((u8)(W_HAND + k))); }
				else ui_nav((p & IN_LEFT) ? IN_LEFT : IN_RIGHT);
			}
			if (p & IN_UP) ui_nav(IN_UP);
			if (p & IN_DOWN) ui_nav(IN_DOWN);
			if (in.click) { u8 h = ui_hit(in.mx, in.my); if (h != 0xFF) activate(ui.w[h].id); else if (ui.itemKind) { ui.itemKind = 0; rebuild_widgets(); ui.dirty |= D_JOKERS; } }
			else if ((p & IN_OK) && fi != 0xFF) activate(fi);
			if (p & IN_PLAY) start_play();
			if (p & IN_DISCARD) do_discard();
			if (p & IN_SORT) { hand_sort(g.sortMode ^ 1); after_hand_change(); }
			if (p & IN_INFO) ui_goto(SC_INFO);
			if (p & IN_BACK)
			{
				if (ui.itemKind) { ui.itemKind = 0; rebuild_widgets(); ui.dirty |= D_JOKERS; }
				else if (ui.sel && g.forced == 0xFF) { ui.sel = 0; preview_hand(); ui.dirty |= D_HAND | D_BUTTONS; }
			}
			if (ui.msgTimer && --ui.msgTimer == 0) ui.dirty |= D_INFO;
			break;
		}
		case PH_SCORING:
			if (in.pressed & IN_OK) ui.timer = 0;
			if (ui.timer) { ui.timer--; break; }
			if (ui.evi < ui.so.n)
			{
				play_event(&ui.so.ev[ui.evi]);
				ui.timer = FR(ui.so.ev[ui.evi].kind == EV_BASE ? 22 : 11);
				ui.evi++;
			}
			else
			{
				clear_popup();
				ui.phase = PH_TALLY; ui.timer = 0; ui.step = 0;
				ui.shownScore = (i32)g.score;
			}
			break;
		case PH_TALLY:
		{
			i32 target = (i32)(g.score + ui.so.total);
			i32 d = target - ui.shownScore;
			if (d > 0 && !(in.pressed & IN_OK))
			{
				i32 st = (i32)(ui.so.total / FR(30)) + 1;
				ui.shownScore += st > d ? d : st;
				hud_update();
				if (ui.shownScore < target) break;
			}
			ui.shownScore = target;
			hud_update();
			round_resolve_play();
			ui.shownScore = (i32)g.score;
			if (g.state == ROUND_PLAYING)
			{
				ui.phase = PH_INPUT;
				Vid_Fill(AREA_X + 1, 38, 191, 76, COL_FELT);
				hud_hand(0xFF, 0, 0);
				draw_joker_row(FALSE);
				after_hand_change();
				draw_all();
				hud_update();
			}
			else { hud_update(); draw_all(); end_of_round(); }
			break;
		}
		case PH_BANNER:
			if (ui.timer) ui.timer--;
			if (!ui.timer || (in.pressed & IN_OK))
			{
				round_end_effects();
				ui_goto(SC_CASHOUT);
			}
			break;
	}
	if (ui.dirty && ui.phase == PH_INPUT)
	{
		if (ui.dirty & D_JOKERS) draw_joker_row(FALSE);
		if (ui.dirty & D_HAND) draw_hand();
		if (ui.dirty & D_BUTTONS) draw_buttons();
		if (ui.dirty & D_INFO) { if (ui.msgTimer == 0) ui.msg = 0; info_show(ui_focus_id()); }
		ui.dirty = 0;
		hud_update();
	}
}
