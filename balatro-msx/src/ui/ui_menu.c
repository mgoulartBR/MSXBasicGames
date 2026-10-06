// Title, blind selection, run info, game over / win, screen dispatcher.
#include "ui.h"
#include "platform/save.h"
#ifdef DEBUG_KEYS
#include "msxgl.h"
#endif

static u16 s_savedSum;                      // checksum of the run as last written to the cartridge SRAM
bool s_hasSave;
static SaveInfo s_info;

static const char* const k_state[3] = { "Defeated", "Select", "Upcoming" };   /* (only [0] and [2] are drawn locally) */

//-----------------------------------------------------------------------------
// dispatcher
//-----------------------------------------------------------------------------
void ui_init(void) BANKED
{
	{ u8* p = (u8*)&ui; for (u16 i = 0; i < sizeof(UI); i++) p[i] = 0; }      // RAM is not cleared at boot
#ifdef DEBUG_KEYS
	{ extern volatile u8 g_perf2[12]; extern volatile u16 g_slowFill; for (u8 i = 0; i < 12; i++) g_perf2[i] = 0; g_slowFill = 0; }
#endif
	ui.hz = Vid_Hz();
	ui.focus = 0xFF;
	g_deckSel = 0; g_stakeSel = 0;                                     // RAM is not cleared at boot
	ui_goto(SC_TITLE);
}

void scr_focus(u8 o, u8 n) BANKED
{
	switch (ui.screen)
	{
		case SC_ROUND: rnd_focus(o, n); break;
		case SC_SHOP:  shop_focus(o, n); break;
		case SC_PACK:  pack_focus(o, n); break;
		case SC_BLIND: blind_focus(o, n); break;
		case SC_DECK:  deck_focus(o, n); break;
		case SC_TITLE: title_focus(o, n); break;
	}
}

void ui_goto(u8 sc) BANKED
{
	Vid_Display(FALSE);                 // draw the new screen blanked: faster and no half-painted frames
	ui.prevScreen = ui.screen;
	ui.screen = sc;
	ui.msgTimer = 0; ui.msg = 0; ui.dirty = 0;
	ui.itemKind = 0;
	ui.focus = 0xFF;
	ui_clear_widgets();
#ifdef DEBUG_KEYS
	u16 t0 = *(volatile u16*)0xFC9E;
#endif
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
		case SC_DECK:    scr_deck(); break;
	}
	Vid_Display(TRUE);
#ifdef DEBUG_KEYS
	{ extern volatile u8 g_perf2[12]; u8 d = (u8)(*(volatile u16*)0xFC9E - t0); if (d > g_perf2[sc]) g_perf2[sc] = d; }
#endif
}

#ifdef DEBUG_KEYS
void debug_keys(void) BANKED;      // ui_debug.c (own mapper segment: the debug build would not fit segment 30)
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
		case SC_DECK:    upd_deck(); break;
	}
	if (!(ui.frame & 31) && Save_Available() && (ui.screen == SC_BLIND || ui.screen == SC_SHOP || (ui.screen == SC_ROUND && ui.phase == PH_INPUT && g.state == ROUND_PLAYING)))
	{
		u16 c = Save_Sum();                                     // autosave when the run changed and the screen is idle
		if (c != s_savedSum) { Save_Write(ui.screen); s_savedSum = c; }
	}
	Vid_Cursor(in.mx, in.my, in.mouse);
}

//-----------------------------------------------------------------------------
// title
//-----------------------------------------------------------------------------
void scr_title(void) BANKED
{
	Vid_Clear(COL_FELT);
	Vid_Logo(52, 34);
	// a fanned hand as decoration
	static const u8 deco[5] = { 12, 25, 38, 51, 11 };   // A of hearts, clubs, diamonds, spades + K of hearts (atlas card indexes)
	for (u8 i = 0; i < 5; i++) Vid_Card(CELL_CARD + deco[i], 66 + i * 26, 124 - (i == 2 ? 6 : 0));
	s_hasSave = Save_Peek(&s_info);
	if (s_hasSave)                                       // a run is waiting: Continue / New Run
	{
		ui_add(W_CONTINUE, 70, 159, 116, 15);
		ui_add(W_NEWRUN, 70, 177, 116, 15);
		ui_set_focus(ui_find(W_CONTINUE));
		ui_button(W_CONTINUE, T_CONTINUE, COL_GREEN, TRUE);
		ui_button(W_NEWRUN, T_NEWRUN, COL_ORANGE, TRUE);
		Vid_Text(40, 196, "Ante", TC_SLATE); Vid_Num(64, 196, s_info.ante, TC_GOLD);
		Vid_Text(82, 196, "$", TC_GOLD); Vid_Num(88, 196, s_info.money, TC_GOLD);
		Vid_Text(116, 196, g_DeckName[s_info.deck], TC_WHITE);
	}
	else
	{
		Vid_TextC(128, 170, "MSX2 port", TC_GOLD);
		Vid_TextC(128, 196, Save_Available() ? "Mouse  Joystick  Keyboard   (autosave on)" : "Mouse  Joystick  Keyboard", TC_SLATE);
	}
	ui.timer = 0;
}

void title_focus(u8 o, u8 n) BANKED
{
	if (!s_hasSave) return;
	for (u8 k = 0; k < 2; k++)
	{
		u8 id = k ? n : o;
		if (id == W_CONTINUE || id == W_NEWRUN) ui_button_ring(id);
	}
}

void upd_title(void) BANKED
{
	ui.timer++;
	if (s_hasSave)
	{
		ui_pointer_focus();
		if (in.pressed & (IN_UP | IN_DOWN)) ui_nav((in.pressed & IN_UP) ? IN_UP : IN_DOWN);
		u8 act = 0xFF;
		if (in.click) { u8 h = ui_hit(in.mx, in.my); if (h != 0xFF) act = ui.w[h].id; }
		else if ((in.pressed & (IN_OK | IN_PLAY)) && ui.focus != 0xFF) act = ui.w[ui.focus].id;
		if (act == W_NEWRUN) ui_goto(SC_DECK);
		else if (act == W_CONTINUE)
		{
			u8 sc;
			if (Save_Load(&sc))
			{
				rng_seed((u16)(ui.frame * 31 + in.mx + in.my * 3));
				ui.shownHand = 0xFF; ui.packReturn = SC_SHOP;
				s_savedSum = Save_Sum();
				ui_goto(sc);
			}
		}
		return;
	}
	if ((ui.timer & 31) == 0 || ui.timer == 1)
	{
		bool on = (ui.timer & 32) == 0;
		Vid_Fill(40, 154, 176, 11, COL_FELT);
		if (on) Vid_TextC(128, 155, in.mouse ? "Click or press SPACE" : "Press SPACE", TC_WHITE);
	}
	if (in.pressed & (IN_OK | IN_PLAY)) ui_goto(SC_DECK);
}

//-----------------------------------------------------------------------------
// blind selection
//-----------------------------------------------------------------------------
void draw_tag(u8 tag, u8 x, u8 y) BANKED { Vid_TagIcon(tag, x, y); }

static void blind_info(void)
{
	Vid_Panel(AREA_X + 2, INFO_Y, 188, 35, COL_INK, COL_SLATE);
	u8 f = ui_focus_id();
	if (f == W_SKIPBLIND)
	{
		u8 t = (g.blind == BLIND_SMALL ? g.tagSmall : g.tagBig) - 1;
		Vid_Text(AREA_X + 6, INFO_Y + 3, g_Tags[t].name, TC_GOLD);
		Vid_Text(AREA_X + 6 + Vid_TextW(g_Tags[t].name) + 4, INFO_Y + 3, "Tag", TC_SLATE);
		Vid_WrapDesc(g_Tags[t].desc, AREA_X + 6, INFO_Y + 14, 180, TC_WHITE, 2);
	}
	else if (g.blind == BLIND_BOSS)
	{
		Vid_Text(AREA_X + 6, INFO_Y + 3, g_Bosses[g.boss].name, TC_RED);
		Vid_WrapDesc(g_Bosses[g.boss].desc, AREA_X + 6, INFO_Y + 14, 180, TC_WHITE, 2);
	}
	else
	{
		Vid_Text(AREA_X + 6, INFO_Y + 3, g.blind == BLIND_SMALL ? "Small Blind" : "Big Blind", TC_WHITE);
		Vid_Wrap(AREA_X + 6, INFO_Y + 14, "Select it to play, or skip it to get its Tag.", 180, TC_SLATE, 2);
	}
}

void scr_blind(void) BANKED
{
	Vid_Clear(COL_FELT);
	ui_clear_widgets();
	hud_mini();
	// the three blinds of this ante
	for (u8 b = 0; b < 3; b++)
	{
		u8 x = AREA_X + 2 + b * 64, st = b < g.blind ? 0 : (b == g.blind ? 1 : 2);
		u8 col = st == 0 ? COL_SLATE : (st == 1 ? COL_BLUE : COL_SLATE);
		Vid_Panel(x, 8, 60, 162, COL_INK, st == 1 ? COL_GOLD : COL_SLATE);
		Vid_Fill(x + 2, 10, 56, 13, col);
		const char* nm = b == 0 ? "Small Blind" : (b == 1 ? "Big Blind" : g_Bosses[g.boss].name);
		Vid_TextC(x + 30, 12, nm, TC_WHITE);
		u8 bi = b == 0 ? g_BlindIcon[0] : (b == 1 ? g_BlindIcon[1] : g_Bosses[g.boss].icon);
		Vid_BlindIcon(bi, x + 22, 26);
		Vid_TextC(x + 30, 45, "Goal", TC_SLATE);
		u8 sv = g.blind; g.blind = b;
		u32 tgt = blind_target(); u8 rw = blind_reward();
		g.blind = sv;
		char d[9]; u8 n = 0; while (n < rw && n < 8) d[n++] = '$'; d[n] = 0;
		Vid_NumR(x + 30 + Vid_NumW((i32)tgt) / 2, 55, (i32)tgt, TC_RED);
		Vid_TextC(x + 30, 67, "Reward", TC_SLATE);
		Vid_TextC(x + 30, 77, d, TC_GOLD);
		if (b == 2) Vid_WrapDesc(g_Bosses[g.boss].desc, x + 4, 94, 52, TC_WHITE, 6);
		else
		{
			u8 tag = b == 0 ? g.tagSmall : g.tagBig;
			if (st != 0 && tag)
			{
				Vid_TextC(x + 30, 92, "Skip Tag", TC_SLATE);
				Vid_TagIcon(tag - 1, x + 22, 103);
				Vid_TextC(x + 30, 122, g_Tags[tag - 1].name, TC_GOLD);
			}
		}
		if (st == 1)
		{
			ui_add(W_BLIND, x + 6, 134, 48, 14);
			ui_button(W_BLIND, T_SELECT, COL_GREEN, TRUE);
			if (blind_can_skip()) { ui_add(W_SKIPBLIND, x + 6, 151, 48, 14); ui_button(W_SKIPBLIND, T_SKIPBLIND, COL_ORANGE, TRUE); }
		}
		else Vid_TextC(x + 30, 153, k_state[st], st == 0 ? TC_GREEN : TC_SLATE);
	}
	ui.defFocus = W_BLIND;
	ui.focus = ui_find(W_BLIND);
	blind_info();
	// tags that fire when the next Blind choice appears (Charm/Meteor/Buffoon: a free pack, Boss: new boss)
	{
		u8 pk = tags_choice_effects();
		if (pk) { pack_open_free(pk); ui.packReturn = SC_BLIND; ui_goto(SC_PACK); return; }
	}
}

void blind_focus(u8 o, u8 n) BANKED
{
	(void)o; (void)n;
	ui_button_ring(W_BLIND);
	ui_button_ring(W_SKIPBLIND);
}

void upd_blind(void) BANKED
{
	ui_pointer_focus();
	u16 p = in.pressed;
	if (p & (IN_UP | IN_DOWN)) ui_nav((p & IN_UP) ? IN_UP : IN_DOWN);
	if ((ui.dirty & D_INFO) && !ui.infoDelay) { ui.dirty = 0; blind_info(); } else if (ui.dirty & D_INFO) ui.infoDelay--;
	u8 act = 0xFF;
	if (in.click) { u8 h = ui_hit(in.mx, in.my); if (h != 0xFF) act = ui.w[h].id; }
	else if ((p & IN_OK) && ui.focus != 0xFF) act = ui.w[ui.focus].id;
	if (p & IN_INFO) act = W_INFO;
	if (act == W_INFO) { ui_goto(SC_INFO); return; }
	if (act == W_BLIND) { blind_start(); ui_goto(SC_ROUND); }
	else if (act == W_SKIPBLIND) { Snd_Play(SFX_BUY); blind_skip(); ui_goto(SC_BLIND); }
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
	Save_Erase(); s_savedSum = 0;                        // the run is over
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
	Save_Erase(); s_savedSum = 0;
	Vid_Clear(COL_FELT);
	Vid_TextC(128, 40, "YOU WIN!", TC_GOLD);
	Vid_TextC(128, 64, g.endless ? "You beat the Ante 12 Boss Blind." : "You beat the Ante 8 Boss Blind.", TC_WHITE);
	Vid_Text(76, 100, "Money", TC_SLATE); Vid_Num(160, 100, g.money, TC_GOLD);
	Vid_Text(76, 112, "Jokers", TC_SLATE); Vid_Num(160, 112, g.nJk, TC_WHITE);
	for (u8 i = 0; i < g.nJk; i++) Vid_Joker(g.jk[i].id, 62 + i * 28, 134);
	ui.timer = 0;
}

void upd_over(void) BANKED
{
	ui.timer++;
	if ((ui.timer & 31) == 0 || ui.timer == 1)
	{
		Vid_Fill(40, 174, 176, 11, COL_FELT);
		if (!(ui.timer & 32)) Vid_TextC(128, 175, ui.screen == SC_WIN && !g.endless ? "SPACE / click: endless to Ante 12" : "Press SPACE or click", TC_WHITE);
	}
	if (ui.screen == SC_WIN && !g.endless)
	{
		if ((ui.timer & 31) == 0 || ui.timer == 1)
		{
			Vid_Fill(40, 186, 176, 11, COL_FELT);
			Vid_TextC(128, 187, "ESC / right click: title", TC_SLATE);
		}
		if (ui.timer > 20 && (in.pressed & IN_OK)) { g.endless = 1; shop_generate(); ui_goto(SC_SHOP); return; }
		if (ui.timer > 20 && (in.pressed & IN_BACK)) ui_goto(SC_TITLE);
		return;
	}
	if (ui.timer > 20 && (in.pressed & (IN_OK | IN_BACK))) ui_goto(SC_TITLE);
}
