// Shared regression cases: compiled natively for the host (tests/host_main.c) AND for the Z80 (selftest ROM, built with
// SDCC), so code-generation bugs in the target compiler cannot hide behind a passing host run.
#include "bgame.h"

extern void test_report(const char* name, u32 got, u32 want);
#define EXPECT(name, got, want) test_report(name, (u32)(got), (u32)(want))

#define H SUIT_H
#define C SUIT_C
#define D SUIT_D
#define S SUIT_S

static void chk_hand(const char* name, const Card* c, u8 n, u8 rules, u8 type, u8 mask)
{
	HandEval e; poker_eval(c, n, rules, &e);
	EXPECT(name, (u16)e.type << 8 | e.mask, (u16)type << 8 | mask);
}

static void load_hand(const Card* cards, u8 n)
{
	blind_start();
	g.nHand = 0;
	for (u8 i = 0; i < n; i++) { g.deck[i] = cards[i]; g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND; }
}
static u32 score_of(const Card* cards, u8 n)
{
	load_hand(cards, n);
	ScoreOut o; round_play((u16)((1u << n) - 1), &o);
	return o.total;
}
static void fresh(void) { rng_seed(1); run_new(); g.blind = BLIND_SMALL; }

void run_poker_cases(void)
{
	{ Card h[] = { CARD(H,RANK_A), CARD(S,5), CARD(D,9) }; chk_hand("poker: high card A", h, 3, 0, HAND_HIGH_CARD, 1); }
	{ Card h[] = { CARD(H,RANK_A), CARD(S,RANK_A), CARD(D,9) }; chk_hand("poker: pair of aces", h, 3, 0, HAND_PAIR, 3); }
	{ Card h[] = { CARD(H,3), CARD(S,3), CARD(D,5), CARD(C,5), CARD(H,9) }; chk_hand("poker: two pair", h, 5, 0, HAND_TWO_PAIR, 0x0F); }
	{ Card h[] = { CARD(H,3), CARD(S,3), CARD(D,3), CARD(C,5), CARD(H,9) }; chk_hand("poker: trips", h, 5, 0, HAND_THREE, 7); }
	{ Card h[] = { CARD(H,3), CARD(S,4), CARD(D,5), CARD(C,6), CARD(H,7) }; chk_hand("poker: straight 5-9", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,RANK_A), CARD(S,0), CARD(D,1), CARD(C,2), CARD(H,3) }; chk_hand("poker: wheel A-5", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,8), CARD(S,9), CARD(D,10), CARD(C,11), CARD(H,12) }; chk_hand("poker: broadway", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,10), CARD(S,11), CARD(D,12), CARD(C,0), CARD(H,1) }; chk_hand("poker: no wrap Q K A 2 3", h, 5, 0, HAND_HIGH_CARD, 4); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), CARD(H,10) }; chk_hand("poker: flush", h, 5, 0, HAND_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,3), CARD(H,4), CARD(H,5), CARD(H,6) }; chk_hand("poker: straight flush", h, 5, 0, HAND_STRAIGHT_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,2), CARD(H,6) }; chk_hand("poker: quads", h, 5, 0, HAND_FOUR, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,6), CARD(H,6) }; chk_hand("poker: full house", h, 5, 0, HAND_FULL_HOUSE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,2), CARD(H,2) }; chk_hand("poker: five of a kind", h, 5, 0, HAND_FIVE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,2) }; chk_hand("poker: flush five", h, 5, 0, HAND_FLUSH_FIVE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,6), CARD(H,6) }; chk_hand("poker: flush house", h, 5, 0, HAND_FLUSH_HOUSE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), CARD(S,10) }; chk_hand("poker: four fingers flush", h, 5, PR_FOUR_FINGERS, HAND_FLUSH, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,3), CARD(D,4), CARD(C,5), CARD(H,10) }; chk_hand("poker: four fingers straight", h, 5, PR_FOUR_FINGERS, HAND_STRAIGHT, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,4), CARD(D,6), CARD(C,8), CARD(H,10) }; chk_hand("poker: shortcut straight", h, 5, PR_SHORTCUT, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,4), CARD(D,6), CARD(C,8), CARD(H,10) }; chk_hand("poker: no shortcut", h, 5, 0, HAND_HIGH_CARD, 0x10); }
	{ Card h[] = { CARD(H,2), CARD(D,4), CARD(H,6), CARD(D,8), CARD(H,10) }; chk_hand("poker: smeared flush", h, 5, PR_SMEARED, HAND_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8) }; chk_hand("poker: 4 hearts is no flush", h, 4, 0, HAND_HIGH_CARD, 0x08); }
}

void run_score_cases(void)
{
	{ fresh(); Card c[] = { CARD(H,RANK_A), CARD(S,RANK_A) }; EXPECT("score: pair of aces (10+11+11)x2", score_of(c, 2), 64); }
	{ fresh(); Card c[] = { CARD(H,RANK_K) }; EXPECT("score: high card K (5+10)x1", score_of(c, 1), 15); }
	{ fresh(); Card c[] = { CARD(H,0), CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8) }; EXPECT("score: flush (35+30)x4", score_of(c, 5), 260); }
	{ fresh(); joker_add(JK_JOKER); Card c[] = { CARD(H,RANK_A), CARD(S,RANK_A) }; EXPECT("score: pair aces + Joker 32x6", score_of(c, 2), 192); }
	{ fresh(); joker_add(JK_JOLLY); joker_add(JK_GREEDY_JOKER); Card c[] = { CARD(D,8), CARD(S,8) }; EXPECT("score: Jolly + Greedy 30x13", score_of(c, 2), 390); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_CAVENDISH); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("score: Joker then Cavendish 20x18", score_of(c, 2), 360); }
	{ fresh(); joker_add(JK_PHOTOGRAPH); Card c[] = { CARD(C,RANK_K), CARD(H,RANK_K) }; EXPECT("score: Photograph 30x4", score_of(c, 2), 120); }
	{ fresh(); joker_add(JK_CAVENDISH); joker_add(JK_JOKER); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("score: order Cavendish then Joker 20x10", score_of(c, 2), 200); }
	{ fresh(); planet_use(HAND_PAIR); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("score: pair level 2 (25+10)x3", score_of(c, 2), 105); }
	{ fresh(); joker_add(JK_SCHOLAR); Card c[] = { CARD(C,RANK_A), CARD(H,RANK_A) }; EXPECT("score: Scholar on two aces (10+22+40)x(2+8)", score_of(c, 2), 720); }
	{ fresh(); joker_add(JK_HALF); Card c[] = { CARD(C,RANK_Q) }; EXPECT("score: Half Joker high card (5+10)x21", score_of(c, 1), 315); }
	{ fresh(); joker_add(JK_BULL); g.money = 10; Card c[] = { CARD(C,RANK_Q) }; EXPECT("score: Bull $10 (5+10+20)x1", score_of(c, 1), 35); }
	{ fresh(); g.blind = BLIND_SMALL; EXPECT("blind: ante 1 small target", blind_target(), 300);
	  g.blind = BLIND_BIG; EXPECT("blind: ante 1 big target", blind_target(), 450);
	  g.ante = 8; g.blind = BLIND_SMALL; EXPECT("blind: ante 8 small target", blind_target(), 50000);
	  g.ante = 9; EXPECT("blind: endless ante 9 small target", blind_target() > 50000, 1); }
	{ fresh(); blind_start(); g.score = g.target; g.money = 23; Cash rows[CASH_MAX]; i16 t; cashout_build(rows, &t);
	  EXPECT("cashout: $3 blind + 4 hands + $4 interest (23/5)", t, 3 + 4 + 4); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_BONUS), CARD(S,RANK_A) }; EXPECT("card: Bonus +30 chips (10+11+11+30)x2", score_of(c, 2), 124); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_MULT), CARD(S,RANK_A) }; EXPECT("card: Mult +4 (32)x6", score_of(c, 2), 192); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_GLASS), CARD(S,RANK_A) }; EXPECT("card: Glass x2 (32)x4", score_of(c, 2), 128); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_STONE) }; EXPECT("card: Stone alone: (5+0+50)x1", score_of(c, 1), 55); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_STONE), CARD(S,RANK_A), CARD(D,RANK_A) }; EXPECT("card: Stone scores beside a pair (10+11+11+50)x2", score_of(c, 3), 164); }
	{ fresh(); Card c[] = { C_SETED(CARD(H,RANK_A), ED_FOIL), CARD(S,RANK_A) }; EXPECT("card: Foil +50 chips (82)x2", score_of(c, 2), 164); }
	{ fresh(); Card c[] = { C_SETED(CARD(H,RANK_A), ED_HOLO), CARD(S,RANK_A) }; EXPECT("card: Holo +10 mult (32)x12", score_of(c, 2), 384); }
	{ fresh(); Card c[] = { C_SETED(CARD(H,RANK_A), ED_POLY), CARD(S,RANK_A) }; EXPECT("card: Polychrome x1.5 (32)x3", score_of(c, 2), 96); }
	{ fresh(); Card c[] = { C_SETSEAL(CARD(H,RANK_A), SEAL_RED), CARD(S,RANK_A) }; EXPECT("card: Red Seal retriggers (10+11+11+11)x2", score_of(c, 2), 86); }
	{ fresh(); g.money = 0; Card c[] = { C_SETSEAL(CARD(H,RANK_A), SEAL_GOLD), CARD(S,RANK_A) }; score_of(c, 2); EXPECT("card: Gold Seal pays $3", g.money, 3); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), C_SETENH(CARD(S,10), ENH_WILD) }; chk_hand("poker: Wild card completes a flush", h, 5, 0, HAND_FLUSH, 0x1F); }
	{ fresh(); Card c[] = { CARD(H,RANK_A), CARD(S,RANK_A) }; load_hand(c, 2); g.deck[g.hand[0]] = CARD(H,RANK_A); g.nHand = 1;
	  g.deck[5] = C_SETENH(CARD(D,3), ENH_STEEL); g.hand[g.nHand++] = 5; g.loc[5] = LOC_HAND; ScoreOut o; round_play(1, &o);
	  EXPECT("card: Steel held x1.5 on high card (5+11)x1.5", o.total, 24); }
	{ fresh(); blind_start(); g.cons[0] = CONS_TAROT(TR_MAGICIAN); u8 slot0 = g.hand[0], slot1 = g.hand[1];
	  EXPECT("tarot: Magician applies Lucky to 2 cards", cons_use(0, 3) && C_ENH(g.deck[slot0]) == ENH_LUCKY && C_ENH(g.deck[slot1]) == ENH_LUCKY, 1);
	  g.cons[0] = CONS_TAROT(TR_TOWER); EXPECT("tarot: Tower needs exactly 1 card", cons_use(0, 3), 0); }
	{ fresh(); blind_start(); g.cons[0] = CONS_TAROT(TR_STRENGTH); u8 sl = g.hand[0]; g.deck[sl] = C_SETENH(CARD(H,3), ENH_GOLD);
	  cons_use(0, 1); EXPECT("tarot: Strength keeps the enhancement", C_ENH(g.deck[sl]) == ENH_GOLD && C_RANK(g.deck[sl]) == 4, 1); }
	{ fresh(); g.nDeck = 52; pack_open_free(PACK_KIND(3, 0)); u8 n0 = g.nDeck;
	  EXPECT("pack: Standard pack adds the chosen card to the deck", pack_choose(0) && g.nDeck == n0 + 1, 1); }
	{ fresh(); blind_start(); g.score = g.target; for (u8 i = 0; i < 2; i++) g.deck[g.hand[i]] = C_SETENH(g.deck[g.hand[i]], ENH_GOLD);
	  Cash rows[CASH_MAX]; i16 t; u8 n = cashout_build(rows, &t); u8 found = 0; for (u8 i = 0; i < n; i++) if (rows[i].kind == 5) found = (u8)rows[i].amount;
	  EXPECT("cashout: Gold Cards held pay $3 each", found, 6); }
	{ fresh(); joker_add(JK_JOKER); g.jk[0].ed = ED_POLY; Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("joker edition: Polychrome (20)x(2+4)x1.5", score_of(c, 2), 180); }
	{ fresh(); blind_start(); g.nHand = 1; g.hand[0] = 0; g.deck[0] = C_SETSEAL(CARD(H,4), SEAL_BLUE); g.loc[0] = LOC_HAND; g.lastHandType = HAND_PAIR; g.nHand = 1;
	  g.cons[0] = g.cons[1] = 0; round_end_effects(); EXPECT("seal: Blue Seal held creates the Planet of the last hand", g.cons[0], CONS_PLANET(HAND_PAIR)); }
	{ fresh(); blind_start(); u8 d0 = g.nHand; g.cons[0] = CONS_SPECTRAL(SP_FAMILIAR); u8 face = 0;
	  EXPECT("spectral: Familiar uses no selection", cons_use(0, 0), 1);
	  for (u8 i = 0; i < g.nHand; i++) face += (C_ENH(g.deck[g.hand[i]]) != 0);
	  EXPECT("spectral: Familiar: hand +2 net, enhanced face cards added", g.nHand == d0 + 2 && face >= 3, 1); }
	{ fresh(); blind_start(); g.cons[0] = CONS_SPECTRAL(SP_TALISMAN); u8 sl = g.hand[2];
	  EXPECT("spectral: Talisman needs one card", cons_use(0, 0), 0);
	  EXPECT("spectral: Talisman gives a Gold Seal", cons_use(0, 4) && C_SEAL(g.deck[sl]) == SEAL_GOLD, 1); }
	{ fresh(); blind_start(); g.cons[0] = CONS_SPECTRAL(SP_SIGIL); cons_use(0, 0); u8 su = C_SUIT(g.deck[g.hand[0]]), same = 1;
	  for (u8 i = 0; i < g.nHand; i++) if (C_SUIT(g.deck[g.hand[i]]) != su) same = 0;
	  EXPECT("spectral: Sigil makes the whole hand one suit", same, 1); }
	{ fresh(); blind_start(); g.cons[0] = CONS_SPECTRAL(SP_IMMOLATE); u8 d0 = g.nHand; i16 m0 = g.money; cons_use(0, 0);
	  EXPECT("spectral: Immolate destroys 5 cards, pays $20", g.nHand == d0 - 5 && g.money == m0 + 20, 1); }
	{ fresh(); g.cons[0] = CONS_SPECTRAL(SP_BLACK_HOLE); cons_use(0, 0); EXPECT("spectral: Black Hole levels every hand", g.handLevel[HAND_PAIR] == 2 && g.handLevel[HAND_FLUSH_FIVE] == 2, 1); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_GREEDY_JOKER); joker_add(JK_JOLLY); g.cons[0] = CONS_SPECTRAL(SP_ANKH);
	  EXPECT("spectral: Ankh keeps one Joker and copies it", cons_use(0, 0) && g.nJk == 2 && g.jk[0].id == g.jk[1].id, 1); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_JOLLY); g.cons[0] = CONS_SPECTRAL(SP_HEX);
	  EXPECT("spectral: Hex makes one Polychrome Joker, destroys the rest", cons_use(0, 0) && g.nJk == 1 && g.jk[0].ed == ED_POLY, 1); }
	{ fresh(); g.money = 40; g.cons[0] = CONS_SPECTRAL(SP_WRAITH); EXPECT("spectral: Wraith creates a Joker and zeroes money", cons_use(0, 0) && g.nJk == 1 && g.money == 0, 1); }
	{ fresh(); g.nDeck = 52; pack_open_free(PACK_KIND(4, 0)); EXPECT("pack: Spectral pack offers 2 cards", g_packN, 2);
	  EXPECT("pack: choosing a Spectral card puts it in a consumable slot", pack_choose(0) && g.cons[0] >= 0x40, 1); }
	{ fresh(); joker_add(JK_HACK); Card c[] = { CARD(H,3), CARD(S,3) }; EXPECT("joker: Hack retriggers 5s (10+5x4)x2", score_of(c, 2), 60); }
	{ fresh(); joker_add(JK_SOCK_AND_BUSKIN); Card c[] = { CARD(H,RANK_K), CARD(S,RANK_K) }; EXPECT("joker: Sock and Buskin retriggers face cards (10+10x4)x2", score_of(c, 2), 100); }
	{ fresh(); joker_add(JK_MIME); Card c[] = { CARD(H,RANK_A) }; load_hand(c, 1); g.deck[5] = C_SETENH(CARD(D,3), ENH_STEEL); g.hand[g.nHand++] = 5; g.loc[5] = LOC_HAND;
	  ScoreOut o; round_play(1, &o); EXPECT("joker: Mime retriggers held Steel (5+11)x2.25", o.total, 36); }
	{ fresh(); joker_add(JK_BLUEPRINT); joker_add(JK_JOKER); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("joker: Blueprint copies the Joker on its right (20)x(2+4+4)", score_of(c, 2), 200); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_BRAINSTORM); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("joker: Brainstorm copies the leftmost Joker (20)x(2+4+4)", score_of(c, 2), 200); }
	{ fresh(); joker_add(JK_TRIBOULET); Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Triboulet X2 on a King (5+10)x2", score_of(c, 1), 30); }
	{ fresh(); joker_add(JK_THROWBACK); g.skips = 2; Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Throwback X1.5 after 2 skips", score_of(c, 1), 22); }
	{ fresh(); joker_add(JK_STONE); g.deck[1] = C_SETENH(g.deck[1], ENH_STONE); g.deck[2] = C_SETENH(g.deck[2], ENH_STONE); Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Stone Joker +50 chips for 2 stones (5+10+50)x1", score_of(c, 1), 65); }
	{ fresh(); joker_add(JK_STEEL_JOKER); g.deck[1] = C_SETENH(g.deck[1], ENH_STEEL); g.deck[2] = C_SETENH(g.deck[2], ENH_STEEL); Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Steel Joker X1.4 for 2 steel cards", score_of(c, 1), 21); }
	{ fresh(); joker_add(JK_EROSION); g.loc[2] = g.loc[3] = LOC_GONE; Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Erosion +8 Mult with 2 cards missing (15)x9", score_of(c, 1), 135); }
	{ fresh(); joker_add(JK_OOPS); u8 all = 1; for (u8 i = 0; i < 60; i++) all &= rnd_odds(2); EXPECT("joker: Oops All 6s makes 1-in-2 certain", all, 1); }
	{ fresh(); joker_add(JK_MIDAS_MASK); Card c[] = { CARD(C,RANK_K), CARD(H,RANK_K) }; score_of(c, 2); EXPECT("joker: Midas Mask turns played face cards Gold", C_ENH(g.deck[0]) == ENH_GOLD && C_ENH(g.deck[1]) == ENH_GOLD, 1); }
	{ fresh(); joker_add(JK_LOYALTY_CARD); u8 x6 = 0; for (u8 i = 0; i < 6; i++) { Card c[] = { CARD(C,RANK_K) }; if (score_of(c, 1) == 60) x6 = (u8)(i + 1); } EXPECT("joker: Loyalty Card X4 on every 6th hand", x6, 6); }
	{ fresh(); joker_add(JK_SELZER); Card c[] = { CARD(H,3), CARD(S,3) }; EXPECT("joker: Seltzer retriggers every card (10+5x4)x2", score_of(c, 2), 60); for (u8 i = 0; i < 9; i++) score_of(c, 2); EXPECT("joker: Seltzer is eaten after 10 hands", (g.jk[0].flags & 0x80) != 0 || g.nJk == 0, 1); }
	{ fresh(); joker_add(JK_DNA); Card c[] = { CARD(C,RANK_K) }; load_hand(c, 1); u8 n0 = g.nDeck; g.handsPlayed = 0; ScoreOut o; round_play(1, &o); EXPECT("joker: DNA copies a lone first-hand card into the deck", g.nDeck, n0 + 1); }
	{ fresh(); joker_add(JK_MARBLE); u8 n0 = g.nDeck; blind_start(); u8 st = 0; for (u8 i = 0; i < g.nDeck; i++) st += C_ENH(g.deck[i]) == ENH_STONE;
	  EXPECT("joker: Marble adds a Stone card when the Blind starts", g.nDeck == n0 + 1 && st == 1, 1); }
	{ fresh(); joker_add(JK_RIFF_RAFF); blind_start(); EXPECT("joker: Riff-raff creates 2 Common Jokers", g.nJk, 3); }
	{ fresh(); joker_add(JK_CEREMONIAL); joker_add(JK_JOKER); u8 sv = joker_sell_value(1); blind_start(); EXPECT("joker: Ceremonial Dagger eats the Joker on its right for 2x its sell value", g.nJk == 1 && g.jk[0].v == 2 * sv, 1); }
	{ fresh(); joker_add(JK_MADNESS); joker_add(JK_JOKER); blind_start(); EXPECT("joker: Madness gains X0.5 and destroys another Joker", g.nJk == 1 && g.jk[0].id == JK_MADNESS && g.jk[0].v == 150, 1); }
	{ fresh(); joker_add(JK_HIT_THE_ROAD); blind_start(); g.deck[g.hand[0]] = CARD(H,RANK_J); g.deck[g.hand[1]] = CARD(S,RANK_J); round_discard(3); EXPECT("joker: Hit the Road X0.5 per discarded Jack", g.jk[0].v, 200); }
	{ fresh(); joker_add(JK_MAIL); blind_start(); g.jk[0].aux = RANK_Q; g.deck[g.hand[0]] = CARD(H,RANK_Q); i16 m0 = g.money; round_discard(1); EXPECT("joker: Mail-In Rebate pays $5 for the target rank", g.money, m0 + 5); }
	{ fresh(); joker_add(JK_CASTLE); blind_start(); g.jk[0].aux = SUIT_S; g.deck[g.hand[0]] = CARD(S,4); g.deck[g.hand[1]] = CARD(H,4); round_discard(3); EXPECT("joker: Castle +3 chips per discarded card of the target suit", g.jk[0].v, 3); }
	{ fresh(); joker_add(JK_TRADING); blind_start(); u8 sl = g.hand[0]; i16 m0 = g.money; round_discard(1); EXPECT("joker: Trading Card destroys a lone first discard for $3", g.loc[sl] == LOC_GONE && g.money == m0 + 3, 1); }
	{ fresh(); joker_add(JK_FLASH); g.money = 50; shop_generate(); shop_reroll(); EXPECT("joker: Flash Card +2 Mult per reroll", g.jk[0].v, 2); }
	{ fresh(); joker_add(JK_CHAOS); g.money = 50; shop_generate(); i16 m0 = g.money; shop_reroll(); u8 free1 = g.money == m0; shop_reroll(); EXPECT("joker: Chaos the Clown first reroll is free", free1 && g.money < m0, 1); }
	{ fresh(); joker_add(JK_LUCHADOR); g.blind = BLIND_BOSS; g.boss = BS_CLUB; blind_start(); joker_sell(0); EXPECT("joker: Luchador disables the Boss Blind when sold", g.bossOff, 1); }
	{ fresh(); joker_add(JK_CHICOT); g.blind = BLIND_BOSS; g.boss = BS_WATER; blind_start(); EXPECT("joker: Chicot disables the Boss Blind", g.bossOff == 1 && g.discardsLeft == START_DISCARDS, 1); }
	{ fresh(); joker_add(JK_INVISIBLE); joker_add(JK_JOKER); g.jk[0].v = 2; joker_sell(0); EXPECT("joker: Invisible Joker duplicates a Joker when sold after 2 rounds", g.nJk, 2); }
	{ fresh(); joker_add(JK_HOLOGRAM); deck_add(CARD(H,3), FALSE); deck_add(CARD(H,4), FALSE); EXPECT("joker: Hologram X0.25 per card added", g.jk[0].v, 150); }
	{ fresh(); joker_add(JK_GLASS); card_destroyed(C_SETENH(CARD(H,3), ENH_GLASS)); EXPECT("joker: Glass Joker gains X0.75 per shattered Glass Card", g.jk[0].v, 175); }
	{ fresh(); joker_add(JK_ASTRONOMER); g.shopType[0] = 2; g.shopId[0] = 0; EXPECT("joker: Astronomer makes Planets free", shop_cost(0), 0); }
	{ fresh(); joker_add(JK_RED_CARD); g_packN = 1; pack_skipped(); EXPECT("joker: Red Card +3 Mult per skipped pack", g.jk[0].v, 3); }
	{ fresh(); joker_add(JK_OBELISK); Card c[] = { CARD(C,RANK_K) }; score_of(c, 1); score_of(c, 1); EXPECT("joker: Obelisk resets while playing the most played hand", g.jk[0].v, 100); }
	{ fresh(); joker_add(JK_VAGABOND); g.money = 3; g.cons[0] = g.cons[1] = 0; Card c[] = { CARD(C,RANK_K) }; score_of(c, 1); EXPECT("joker: Vagabond creates a Tarot when poor", g.cons[0] != 0, 1); }
	{ fresh(); g.cons[0] = CONS_SPECTRAL(SP_SOUL); EXPECT("spectral: The Soul creates a Legendary Joker", cons_use(0, 0) && g.nJk == 1 && g_Jokers[g.jk[0].id].rarity == 4, 1); }
	{ fresh(); g.blind = BLIND_BOSS; g.boss = BS_HOUSE; blind_start(); u8 fd = 0;
	  for (u8 i = 0; i < g.nHand; i++) fd += (g.dflag[g.hand[i]] & DF_FD) != 0;
	  EXPECT("boss: House draws the first hand face down", fd == g.nHand && fd > 0, 1);
	  ScoreOut o; round_play(1, &o);
	  EXPECT("boss: played cards turn face up", g.dflag[g.played[0]] & DF_FD, 0); }
	{ fresh(); g.blind = BLIND_BOSS; g.boss = BS_MARK; blind_start(); u8 bad = 0;
	  for (u8 i = 0; i < g.nHand; i++) bad += ((g.dflag[g.hand[i]] & DF_FD) != 0) != card_is_face(g.deck[g.hand[i]]);
	  EXPECT("boss: Mark hides exactly the face cards", bad, 0); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_GREEDY_JOKER); joker_add(JK_JOLLY); g.blind = BLIND_BOSS; g.boss = BS_FINAL_ACORN; blind_start();
	  u8 sum = 0; for (u8 i = 0; i < g.nJk; i++) sum += g.jk[i].id;
	  EXPECT("boss: Amber Acorn keeps the joker set", g.nJk == 3 ? sum : 0, JK_JOKER + JK_GREEDY_JOKER + JK_JOLLY); }
}

void run_all_cases(void) BANKED { run_poker_cases(); run_score_cases(); }
