// Debug-build helpers: key shortcuts + RAM beacon for the openMSX tests. Included only when DEBUG_KEYS is set.
#include "ui.h"
#include "platform/save.h"
#include "msxgl.h"
extern bool s_hasSave;

// Debug build only (scripts/build.sh debug): 1 win round, 2 random joker, 3 +$50, 4 planet+tarot, 5 lose round
// RAM beacon read by the openMSX test scripts (tests/tcl/asserts.tcl) through `peek`
volatile u8 g_beacon[48];
volatile u8 g_perf2[12];      // worst duration (frames) of each screen constructor
static void bset(u8 i, u8 v) { g_beacon[i] = v; }   // one store per call: SDCC 4.6.0 mis-compiles chained stores of u32 fields
static void beacon(void)
{
	u32 sc = g.score;
	bset(0, ui.screen); bset(1, ui.phase); bset(2, g.ante); bset(3, g.blind);
	bset(4, (u8)(g.money & 0xFF)); bset(5, (u8)((u16)g.money >> 8));
	bset(6, g.handsLeft); bset(7, g.discardsLeft); bset(8, g.nHand); bset(9, g.nJk); bset(10, g.state);
	bset(11, (u8)(sc & 0xFF)); sc >>= 8; bset(12, (u8)(sc & 0xFF)); sc >>= 8; bset(13, (u8)(sc & 0xFF)); sc >>= 8; bset(14, (u8)sc);
	bset(15, (u8)ui.frame); bset(16, (u8)(ui.sel & 0xFF)); bset(17, g.nPile); bset(18, in.mouse); bset(19, g.boss); bset(20, g.nTags); bset(21, g.skips); bset(22, g.tagSmall); bset(23, g.tagBig); bset(24, g.deckId); bset(25, g.stake); bset(26, g_deckSel); bset(27, g_stakeSel); bset(28, Save_Available()); bset(29, s_hasSave); bset(30, g.discardsUsed); bset(31, g.nJk);
	bset(32, joker_slots()); bset(33, cons_slots()); bset(34, g.consNeg); bset(35, g.nTags); bset(36, jpitch()); bset(37, g.vouchers & 0xFF); bset(38, (u8)(g.vouchers >> 8)); bset(39, ui_focus_id());
	{ u8 nc = 0; for (u8 i = 0; i < CONS_MAX; i++) nc += g.cons[i] != 0; bset(40, nc); }
}

void debug_keys(void) BANKED
{
	beacon();
	static u8 held;
	u8 now = (u8)~Keyboard_Read(0), down = (u8)(now & ~held);
	held = now;
	if ((down & 0x02) && ui.screen == SC_ROUND && ui.phase == PH_INPUT) { g.score = g.target; g.state = ROUND_WON; ui.phase = PH_BANNER; ui.timer = 2; }
	if (down & 0x04) joker_add(rndn(JOKER_COUNT));
	if (down & 0x08) g.money += 50;
	if (down & 0x01) cons_add(CONS_SPECTRAL(rndn(SPECTRAL_COUNT)));       // 0 = random Spectral card
	if (down & 0x10) { cons_add(CONS_PLANET(rndn(HAND_COUNT))); cons_add(CONS_TAROT(rndn(TAROT_COUNT))); }
	if ((down & 0x20) && ui.screen == SC_ROUND) { g.state = ROUND_LOST; ui_goto(SC_OVER); }
	{                                                           // 8 = jump to the Ante 8 boss blind (endless-mode test)
		static u8 held1;
		u8 n1 = (u8)~Keyboard_Read(1), d1 = (u8)(n1 & ~held1); held1 = n1;
		if ((d1 & 0x04) && ui.screen == SC_ROUND) { static u8 hi; static const u8 ids[5] = { 149, 148, 147, 146, 145 }; joker_add(ids[hi++ % 5]); rnd_rebuild(); ui.dirty |= D_JOKERS; }   // - = the newest Jokers (art streamed from the last segments)
		if (d1 & 0x10) { tag_gain(TG_DOUBLE); ui_goto(ui.screen); }        // \ = a Double Tag (the screen is redrawn to show it)
		if ((d1 & 0x08) && ui.screen == SC_ROUND)              // = : every Joker becomes Negative, plus a Negative Planet in the consumable row
		{
			for (u8 i = 0; i < g.nJk; i++) g.jk[i].ed = ED_NEG;
			cons_add_ed(CONS_PLANET(rndn(HAND_FIVE)), TRUE);
			rnd_rebuild();
			ui.dirty |= D_JOKERS;
		}
		if ((d1 & 0x02) && ui.screen == SC_ROUND)              // 9 = random enhancements / editions / seals on the hand
		{
			for (u8 i = 0; i < g.nHand; i++)
			{
				Card* d = &g.deck[g.hand[i]];
				*d = C_SETENH(*d, rndn(ENH_COUNT)); *d = C_SETED(*d, rndn(2) ? rndn(4) : 0); *d = C_SETSEAL(*d, rndn(2) ? rndn(SEAL_COUNT) : 0);
			}
			ui.hnc = 0; ui.dirty |= D_ALL;
		}
		if ((d1 & 0x01) && ui.screen == SC_BLIND) { g.ante = 8; g.blind = BLIND_BOSS; g.boss = BS_FINAL_VESSEL; ui_goto(SC_BLIND); }
	}
	if (down & 0x1F) { ui.hnc = 0; ui.dirty |= D_ALL | D_JOKERS; }
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
