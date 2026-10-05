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
