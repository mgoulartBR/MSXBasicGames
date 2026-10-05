// Title, blind selection, run info, game over / win, screen dispatcher.
#include "ui.h"
#ifdef DEBUG_KEYS
#include "msxgl.h"
#endif

static const char* const k_state[3] = { "Defeated", "Select", "Upcoming" };   /* (only [0] and [2] are drawn locally) */

//-----------------------------------------------------------------------------
// dispatcher
//-----------------------------------------------------------------------------
void ui_init(void) BANKED
{
	ui.hz = Vid_Hz();
	ui.focus = 0xFF;
	ui_goto(SC_TITLE);
}

void ui_goto(u8 sc) BANKED
{
	ui.prevScreen = ui.screen;
	ui.screen = sc;
	ui.msgTimer = 0; ui.msg = 0; ui.dirty = 0;
	ui.itemKind = 0;
	ui.focus = 0xFF;
	ui_clear_widgets();
	switch (sc)
	{
		case SC_TITLE:   scr_title(); break;
		case SC_BLIND:   scr_blind(); break;
		case SC_ROUND:   scr_round(); break;
		case SC_CASHOUT: scr_cashout(); break;
		case SC_SHOP:    scr_shop(); break;
		case SC_PACK:    scr_pack(); break;
		case SC_INFO:    scr_info(); break;
		case SC_OVER:    scr_over(); break;
		case SC_WIN:     scr_win(); break;
	}
}

#ifdef DEBUG_KEYS
// Debug build only (scripts/build.sh debug): 1 win round, 2 random joker, 3 +$50, 4 planet+tarot, 5 lose round
static void debug_keys(void)
{
	static u8 held;
	u8 now = (u8)~Keyboard_Read(0), down = (u8)(now & ~held);
	held = now;
	if ((down & 0x02) && ui.screen == SC_ROUND && ui.phase == PH_INPUT) { g.score = g.target; g.state = ROUND_WON; ui.phase = PH_BANNER; ui.timer = 2; }
	if (down & 0x04) joker_add(rndn(JOKER_COUNT));
	if (down & 0x08) g.money += 50;
	if (down & 0x10) { cons_add(CONS_PLANET(rndn(HAND_COUNT))); cons_add(CONS_TAROT(rndn(TAROT_COUNT))); }
	if ((down & 0x20) && ui.screen == SC_ROUND) { g.state = ROUND_LOST; ui_goto(SC_OVER); }
	if (down & 0x1E) ui.dirty |= D_ALL;
	// 6 = teleport the pointer to the centre of the next widget (exercises hover/hit-testing), 7 = left click there,
	// 8 = right click.  The real MSX mouse protocol is tested separately with xdotool (tests/mouse_test.sh).
	{
		static u8 wi;
		if ((down & 0x40) && ui.nw) { wi = (u8)((wi + 1) % ui.nw); in.mx = ui.w[wi].x + ui.w[wi].w / 2; in.my = ui.w[wi].y + ui.w[wi].h / 2; in.moved = TRUE; in.mouse = TRUE; }
		if (down & 0x80) { in.click = TRUE; in.mouse = TRUE; in.pressed |= IN_OK; }
	}
	if (in.moved || in.click)                                  // pointer position read-out for the mouse tests
	{
		Vid_Fill(0, 204, 62, 8, COL_INK);
		Vid_Num(2, 203, in.mx, TC_GOLD); Vid_Num(24, 203, in.my, TC_GOLD);
		if (in.click) Vid_Text(46, 203, "C", TC_RED);
	}
}
#endif

void ui_update(void) BANKED
{
	ui.frame++;
#ifdef DEBUG_KEYS
	debug_keys();
#endif
	switch (ui.screen)
	{
		case SC_TITLE:   upd_title(); break;
		case SC_BLIND:   upd_blind(); break;
		case SC_ROUND:   upd_round(); break;
		case SC_CASHOUT: upd_cashout(); break;
		case SC_SHOP:    upd_shop(); break;
		case SC_PACK:    upd_pack(); break;
		case SC_INFO:    upd_info(); break;
		case SC_OVER:
		case SC_WIN:     upd_over(); break;
	}
	Vid_Cursor(in.mx, in.my, in.mouse);
}

//-----------------------------------------------------------------------------
// title
//-----------------------------------------------------------------------------
void scr_title(void) BANKED
{
	Vid_Clear(COL_FELT);
	Vid_Logo(52, 14);
	// a fanned hand as decoration
	static const u8 deco[5] = { 12, 25, 38, 51, 11 };   // A of hearts, clubs, diamonds, spades + K of hearts (atlas card indexes)
	for (u8 i = 0; i < 5; i++) Vid_Card(CELL_CARD + deco[i], 66 + i * 26, 124 - (i == 2 ? 6 : 0));
	Vid_TextC(128, 170, "MSX2 port", TC_GOLD);
	Vid_TextC(128, 196, "Mouse  Joystick  Keyboard", TC_SLATE);
	ui.timer = 0;
}

void upd_title(void) BANKED
{
	ui.timer++;
	if ((ui.timer & 31) == 0 || ui.timer == 1)
	{
		bool on = (ui.timer & 32) == 0;
		Vid_Fill(40, 154, 176, 11, COL_FELT);
		if (on) Vid_TextC(128, 155, in.mouse ? "Click or press SPACE" : "Press SPACE", TC_WHITE);
	}
	if (in.pressed & (IN_OK | IN_PLAY))
	{
		rng_seed((u16)(ui.frame * 31 + ui.timer * 7 + in.mx + in.my * 3));
		run_new();
		ui.shownHand = 0xFF;
		ui_goto(SC_BLIND);
	}
}

//-----------------------------------------------------------------------------
// blind selection
//-----------------------------------------------------------------------------
void scr_blind(void) BANKED
{
	Vid_Clear(COL_FELT);
	hud_mini();
	// the three blinds of this ante
	for (u8 b = 0; b < 3; b++)
	{
		u8 x = AREA_X + 2 + b * 64, st = b < g.blind ? 0 : (b == g.blind ? 1 : 2);
		u8 col = st == 0 ? COL_SLATE : (st == 1 ? COL_BLUE : COL_SLATE);
		Vid_Panel(x, 8, 60, 160, COL_INK, st == 1 ? COL_GOLD : COL_SLATE);
		Vid_Fill(x + 2, 10, 56, 13, col);
		const char* nm = b == 0 ? "Small Blind" : (b == 1 ? "Big Blind" : g_Bosses[g.boss].name);
		Vid_TextC(x + 30, 12, nm, TC_WHITE);
		u8 bi = b == 0 ? g_BlindIcon[0] : (b == 1 ? g_BlindIcon[1] : g_Bosses[g.boss].icon);
		Vid_BlindIcon(bi, x + 22, 28);
		Vid_TextC(x + 30, 50, "Goal", TC_SLATE);
		u8 sv = g.blind; g.blind = b;
		u32 tgt = blind_target(); u8 rw = blind_reward();
		g.blind = sv;
		char d[9]; u8 n = 0; while (n < rw && n < 8) d[n++] = '$'; d[n] = 0;
		Vid_NumR(x + 30 + Vid_NumW((i32)tgt) / 2, 61, (i32)tgt, TC_RED);
		Vid_TextC(x + 30, 75, "Reward", TC_SLATE);
		Vid_TextC(x + 30, 85, d, TC_GOLD);
		if (b == 2) Vid_Wrap(x + 4, 100, g_Bosses[g.boss].desc, 52, TC_WHITE, 5);
		if (st == 1) { ui_add(W_BLIND, x + 6, 148, 48, 14); ui_button(W_BLIND, T_SELECT, COL_GREEN, TRUE); }
		else Vid_TextC(x + 30, 151, k_state[st], st == 0 ? TC_GREEN : TC_SLATE);
	}
	Vid_Text(AREA_X + 6, 176, "Choose your next Blind", TC_SLATE);
	ui_set_focus(ui_find(W_BLIND));
}

void upd_blind(void) BANKED
{
	ui_pointer_focus();
	if (in.pressed & (IN_UP | IN_DOWN | IN_LEFT | IN_RIGHT)) ui_nav(IN_DOWN);
	if (ui.dirty) { ui_button(W_BLIND, T_SELECT, COL_GREEN, TRUE); ui.dirty = 0; }
	bool go = FALSE;
	if (in.click) { u8 h = ui_hit(in.mx, in.my); go = h != 0xFF && ui.w[h].id == W_BLIND; }
	else if (in.pressed & IN_OK) go = TRUE;
	if (in.pressed & IN_INFO) { ui_goto(SC_INFO); return; }
	if (in.click) { u8 h = ui_hit(in.mx, in.my); if (h != 0xFF && ui.w[h].id == W_INFO) { ui_goto(SC_INFO); return; } }
	if (go)
	{
		blind_start();
		ui_goto(SC_ROUND);
	}
}

//-----------------------------------------------------------------------------
// run info: poker hand levels
//-----------------------------------------------------------------------------
void scr_info(void) BANKED
{
	Vid_Clear(COL_FELT);
	Vid_TextC(128, 4, "Poker Hands", TC_GOLD);
	Vid_Text(8, 16, "Hand", TC_SLATE); Vid_Text(110, 16, "Lv", TC_SLATE); Vid_Text(134, 16, "Chips", TC_BLUE);
	Vid_Text(176, 16, "Mult", TC_RED); Vid_Text(214, 16, "Played", TC_SLATE);
	for (u8 i = 0; i < HAND_COUNT; i++)
	{
		u8 t = HAND_COUNT - 1 - i, y = 28 + i * 12;
		bool secret = t >= HAND_FIVE && g.handPlays[t] == 0;
		Vid_Fill(4, y - 1, 248, 11, i & 1 ? COL_FELT : COL_INK);
		Vid_Text(8, y, secret ? "???" : g_Hands[t].name, secret ? TC_SLATE : TC_WHITE);
		if (secret) continue;
		Vid_NumR(122, y, g.handLevel[t], TC_WHITE);
		Vid_NumR(160, y, (i32)hand_chips(t), TC_BLUE);
		Vid_NumR(198, y, hand_mult(t), TC_RED);
		Vid_NumR(244, y, g.handPlays[t], TC_GOLD);
	}
	ui_add(W_BACK, 88, 184, 80, 16);
	ui_button(W_BACK, T_BACK, COL_GREEN, TRUE);
	ui_set_focus(0);
}

void upd_info(void) BANKED
{
	if (in.pressed & (IN_OK | IN_BACK | IN_INFO) || in.click)
	{
		ui_goto(ui.prevScreen);
	}
}

//-----------------------------------------------------------------------------
// game over / win
//-----------------------------------------------------------------------------
void scr_over(void) BANKED
{
	Vid_Clear(COL_FELT);
	Vid_TextC(128, 40, "GAME OVER", TC_RED);
	Vid_TextC(128, 64, "You did not reach the Blind's score.", TC_WHITE);
	Vid_Text(76, 88, "Ante reached", TC_SLATE); Vid_Num(160, 88, g.ante, TC_GOLD);
	Vid_Text(76, 100, "Round score", TC_SLATE); Vid_Num(160, 100, (i32)g.score, TC_WHITE);
	Vid_Text(76, 112, "Blind target", TC_SLATE); Vid_Num(160, 112, (i32)g.target, TC_RED);
	Vid_Text(76, 124, "Money", TC_SLATE); Vid_Num(160, 124, g.money, TC_GOLD);
	ui.timer = 0;
}

void scr_win(void) BANKED
{
	Vid_Clear(COL_FELT);
	Vid_TextC(128, 40, "YOU WIN!", TC_GOLD);
	Vid_TextC(128, 64, "You beat the Ante 8 Boss Blind.", TC_WHITE);
	Vid_Text(76, 100, "Money", TC_SLATE); Vid_Num(160, 100, g.money, TC_GOLD);
	Vid_Text(76, 112, "Jokers", TC_SLATE); Vid_Num(160, 112, g.nJk, TC_WHITE);
	for (u8 i = 0; i < g.nJk; i++) Vid_Card(g_Jokers[g.jk[i].id].cell, 62 + i * 28, 134);
	ui.timer = 0;
}

void upd_over(void) BANKED
{
	ui.timer++;
	if ((ui.timer & 31) == 0 || ui.timer == 1)
	{
		Vid_Fill(40, 174, 176, 11, COL_FELT);
		if (!(ui.timer & 32)) Vid_TextC(128, 175, "Press SPACE or click", TC_WHITE);
	}
	if (ui.timer > 20 && (in.pressed & (IN_OK | IN_BACK))) ui_goto(SC_TITLE);
}
