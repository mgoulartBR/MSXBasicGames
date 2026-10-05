// Host simulation: random "bot" runs through the whole rules engine to catch crashes / invariant breaks.
// gcc -I include tests/host_sim.c src/game/*.c src/seg/seg_s20_b3.c
#include <stdio.h>
#include <stdlib.h>
#include "bgame.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { printf("INVARIANT FAILED: " __VA_ARGS__); printf("\n"); fails++; if (fails > 5) exit(1); } } while (0)

static void check_state(void)
{
	CHECK(g.nHand <= HAND_MAX, "nHand=%d", g.nHand);
	u8 seen[DECK_MAX] = { 0 };
	u8 inhand = 0, inpile = 0;
	for (u8 i = 0; i < g.nHand; i++) { CHECK(g.hand[i] < g.nDeck, "slot"); CHECK(!seen[g.hand[i]]++, "dup hand slot"); CHECK(g.loc[g.hand[i]] == LOC_HAND, "loc hand"); inhand++; }
	for (u8 i = 0; i < g.nPile; i++) { CHECK(!seen[g.pile[i]]++, "dup pile slot"); CHECK(g.loc[g.pile[i]] == LOC_PILE, "loc pile"); inpile++; }
	CHECK(g.nJk <= JOKER_MAX, "jokers");
}

int main(int argc, char** argv)
{
	int runs = argc > 1 ? atoi(argv[1]) : 200;
	long wins = 0, totalRounds = 0, maxAnte = 0, ante[10] = { 0 };
	for (int run = 0; run < runs; run++)
	{
		rng_seed((u16)(run * 7919 + 13));
		run_new();
		while (!run_won())
		{
			blind_start();
			check_state();
			int guard = 0;
			while (g.state == ROUND_PLAYING && guard++ < 100)
			{
				// bot: discard junk if possible, else play best 5 by simple heuristic: play a random 5
				u16 sel = 0; u8 n = g.nHand < 5 ? g.nHand : 5;
				for (u8 i = 0; i < n; i++) sel |= (u16)(1u << i);   // hand is sorted by rank: top-5 ranks
				if (g.discardsLeft && g.handsLeft > 1 && (rnd8() & 1)) { round_discard((u16)(1u << (g.nHand - 1))); check_state(); continue; }
				if (!round_can_play(sel)) break;
				ScoreOut o; round_play(sel, &o);
				CHECK(o.n <= EVENT_MAX, "events");
				round_resolve_play();
				check_state();
			}
			totalRounds++;
			if (g.state != ROUND_WON) break;
			round_end_effects();
			Cash rows[CASH_MAX]; i16 total; cashout_build(rows, &total);
			g.money += total;
			if (g.blind == BLIND_BOSS && g.ante == MAX_ANTE) { g.ante++; break; }
			shop_generate();
			// bot shopping: buy whatever is affordable (jokers first)
			for (u8 i = 0; i < g.shopN; i++) if (g.shopType[i]) shop_buy(i);
			for (u8 i = 0; i < CONS_MAX; i++) if (g.cons[i]) { u8 mn, mx; u8 c = g.cons[i]; if (!cons_needs_cards(c, &mn, &mx)) cons_use(i, 0); }
			next_blind();
		}
		if (run_won()) wins++;
		ante[g.ante > 9 ? 9 : g.ante]++;
		if (g.ante > maxAnte) maxAnte = g.ante;
	}
	printf("sim: %d runs, %ld rounds, %ld wins, max ante %ld\n", runs, totalRounds, wins, maxAnte);
	printf("died at ante: "); for (int a = 1; a <= 9; a++) printf("%d:%ld ", a, ante[a]); printf("\n");
	printf(fails ? "SIM FAILED (%d)\n" : "sim ok\n", fails);
	return fails != 0;
}
