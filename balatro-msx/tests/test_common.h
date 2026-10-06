// Helpers shared by the regression case files (host build and Z80 self-test).
#pragma once
#include "bgame.h"

extern void test_report(const char* name, u32 got, u32 want);
#ifdef SELFTEST
#define EXPECT(name, got, want) test_report(0, (u32)(got), (u32)(want))          /* names are dropped in the Z80 ROM: they would not fit */
#else
#define EXPECT(name, got, want) test_report(name, (u32)(got), (u32)(want))
#endif

#define H SUIT_H
#define C SUIT_C
#define D SUIT_D
#define S SUIT_S

static void chk_hand_(const char* name, const Card* c, u8 n, u8 rules, u8 type, u8 mask)
{
	HandEval e; poker_eval(c, n, rules, &e);
	EXPECT(name, (u16)e.type << 8 | e.mask, (u16)type << 8 | mask);
}
#ifdef SELFTEST
#define chk_hand(name, ...) chk_hand_(0, __VA_ARGS__)
#else
#define chk_hand(name, ...) chk_hand_(name, __VA_ARGS__)
#endif

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
static void fresh(void) { rng_seed(1); g_deckSel = 0; g_stakeSel = 0; run_new(); g.blind = BLIND_SMALL; }
static void fresh_with(u8 deck, u8 stake) { rng_seed(1); g_deckSel = deck; g_stakeSel = stake; run_new(); g.blind = BLIND_SMALL; g_deckSel = 0; g_stakeSel = 0; }

