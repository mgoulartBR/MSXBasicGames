// Host-side scoring checks against hand-computed values (rules taken from the original).
#include <stdio.h>
#include "bgame.h"
static int fails = 0;
static void expect(const char* name, u32 got, u32 want)
{
	if (got != want) { printf("FAIL %-40s got %lu want %lu\n", name, (unsigned long)got, (unsigned long)want); fails++; }
	else printf("ok   %-40s %lu\n", name, (unsigned long)got);
}
// put the given cards in hand (deck slots 0..n-1 overwritten) and play them all
static u32 play(const Card* cards, u8 n, const Card* held, u8 nheld)
{
	run_new(); g.blind = BLIND_SMALL;
	blind_start();
	g.nHand = 0;
	for (u8 i = 0; i < n + nheld; i++)
	{
		g.deck[i] = i < n ? cards[i] : held[i - n];
		g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND;
	}
	u16 sel = (u16)((1u << n) - 1);
	ScoreOut o; round_play(sel, &o);
	return o.total;
}
int main(void)
{
	rng_seed(1);
	Card pairA[] = { CARD(SUIT_H, RANK_A), CARD(SUIT_S, RANK_A) };
	expect("pair of aces: (10+11+11)x2", play(pairA, 2, 0, 0), 64);

	Card hi[] = { CARD(SUIT_H, RANK_K) };
	expect("high card K: (5+10)x1", play(hi, 1, 0, 0), 15);

	Card fl[] = { CARD(SUIT_H, 0), CARD(SUIT_H, 2), CARD(SUIT_H, 4), CARD(SUIT_H, 6), CARD(SUIT_H, 8) };
	expect("flush 2,4,6,8,10: (35+2+4+6+8+10)x4", play(fl, 5, 0, 0), (35 + 2 + 4 + 6 + 8 + 10) * 4);

	// Joker (+4 mult)
	run_new(); joker_add(JK_JOKER);
	{ Card c[] = { CARD(SUIT_H, RANK_A), CARD(SUIT_S, RANK_A) };
	  blind_start(); g.nHand = 0; for (u8 i = 0; i < 2; i++) { g.deck[i] = c[i]; g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND; }
	  ScoreOut o; round_play(3, &o); expect("pair aces + Joker: 32 x 6", o.total, 192); }

	// Jolly (+8 mult on pair) + Greedy (+3 per diamond scored)
	run_new(); joker_add(JK_JOLLY); joker_add(JK_GREEDY_JOKER);
	{ Card c[] = { CARD(SUIT_D, 8), CARD(SUIT_S, 8) };
	  blind_start(); g.nHand = 0; for (u8 i = 0; i < 2; i++) { g.deck[i] = c[i]; g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND; }
	  ScoreOut o; round_play(3, &o); expect("pair of 10s + Jolly + Greedy: 30 x (2+8+3)", o.total, 30 * 13); }

	// Cavendish X3 after Joker: (chips)x((2+4)*3)
	run_new(); joker_add(JK_JOKER); joker_add(JK_CAVENDISH);
	{ Card c[] = { CARD(SUIT_C, 3), CARD(SUIT_H, 3) };
	  blind_start(); g.nHand = 0; for (u8 i = 0; i < 2; i++) { g.deck[i] = c[i]; g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND; }
	  ScoreOut o; round_play(3, &o); expect("pair of 5s + Joker + Cavendish: 20 x 18", o.total, 20 * 18); }

	// Photograph: first face card X2 (played order), Baron held X1.5
	run_new(); joker_add(JK_PHOTOGRAPH);
	{ Card c[] = { CARD(SUIT_C, RANK_K), CARD(SUIT_H, RANK_K) };
	  blind_start(); g.nHand = 0; for (u8 i = 0; i < 2; i++) { g.deck[i] = c[i]; g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND; }
	  ScoreOut o; round_play(3, &o); expect("pair of kings + Photograph: 30 x 4", o.total, 120); }

	// Joker order matters: X3 first, then +4 => (2*3)+4 = 10, vs +4 first => (2+4)*3 = 18
	run_new(); joker_add(JK_CAVENDISH); joker_add(JK_JOKER);
	{ Card c[] = { CARD(SUIT_C, 3), CARD(SUIT_H, 3) };
	  blind_start(); g.nHand = 0; for (u8 i = 0; i < 2; i++) { g.deck[i] = c[i]; g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND; }
	  ScoreOut o; round_play(3, &o); expect("order: Cavendish then Joker: 20 x 10", o.total, 200); }

	// planet levels: Pair level 2 = chips 10+15, mult 2+1
	run_new(); planet_use(HAND_PAIR);
	{ Card c[] = { CARD(SUIT_C, 3), CARD(SUIT_H, 3) };
	  blind_start(); g.nHand = 0; for (u8 i = 0; i < 2; i++) { g.deck[i] = c[i]; g.hand[g.nHand++] = i; g.loc[i] = LOC_HAND; }
	  ScoreOut o; round_play(3, &o); expect("pair lvl2 of 5s: (25+10) x 3", o.total, 105); }

	// blind targets (ante amounts 300, 800, ...)
	run_new(); g.blind = BLIND_SMALL; expect("ante1 small target", blind_target(), 300);
	g.blind = BLIND_BIG; expect("ante1 big target", blind_target(), 450);
	g.ante = 8; g.blind = BLIND_SMALL; expect("ante8 small target", blind_target(), 50000);

	printf(fails ? "\n%d FAILED\n" : "\nall scoring tests passed\n", fails);
	return fails != 0;
}
