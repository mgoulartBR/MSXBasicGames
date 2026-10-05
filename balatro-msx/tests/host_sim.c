// Host simulation: a heuristic "bot" plays whole runs through the rules engine to catch crashes / invariant
// breaks and to sanity-check progression (not a balance tool).
// gcc -I include -I src tests/host_sim.c src/game/*.c src/seg/seg_s20_b3.c
#include <stdio.h>
#include <stdlib.h>
#include "bgame.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { printf("INVARIANT FAILED: " __VA_ARGS__); printf("\n"); fails++; if (fails > 5) exit(1); } } while (0)

static void check_state(void)
{
	CHECK(g.nHand <= HAND_MAX, "nHand=%d", g.nHand);
	u8 seen[DECK_MAX] = { 0 };
	for (u8 i = 0; i < g.nHand; i++) { CHECK(g.hand[i] < g.nDeck, "slot"); CHECK(!seen[g.hand[i]]++, "dup hand slot"); CHECK(g.loc[g.hand[i]] == LOC_HAND, "loc hand"); }
	for (u8 i = 0; i < g.nPile; i++) { CHECK(!seen[g.pile[i]]++, "dup pile slot"); CHECK(g.loc[g.pile[i]] == LOC_PILE, "loc pile"); }
	CHECK(g.nJk <= JOKER_MAX, "jokers");
	CHECK(g.handLevel[0] >= 1, "level");
}

// best subset of the hand by (hand type, summed nominal)
static u16 best_play(u8 rules, u8* typeOut)
{
	u16 best = 0; long bestScore = -1; u8 bestType = 0;
	for (u16 m = 1; m < (1u << g.nHand); m++)
	{
		u8 n = 0; Card c[5]; int ok = 1;
		for (u8 i = 0; i < g.nHand; i++) if (m & (1u << i)) { if (n >= 5) { ok = 0; break; } c[n++] = g.deck[g.hand[i]]; }
		if (!ok) continue;
		HandEval he; poker_eval(c, n, rules, &he);
		long sc = (long)(hand_chips(he.type) * hand_mult(he.type)) * 100;
		for (u8 i = 0; i < n; i++) if (he.mask & (1 << i)) sc += C_NOMINAL(c[i]) * hand_mult(he.type);
		if (sc > bestScore) { bestScore = sc; best = m; bestType = he.type; }
	}
	*typeOut = bestType;
	return best;
}

// force every boss and every joker through a few hands (coverage of the effect code paths)
static void coverage(void)
{
	long hands = 0;
	for (int b = 0; b < BOSS_COUNT; b++)
	{
		for (int j = -1; j < JOKER_COUNT; j += 3)
		{
			rng_seed((u16)(b * 131 + j * 17 + 5));
			run_new();
			g.ante = g_Bosses[b].reward == 8 ? MAX_ANTE : (g_Bosses[b].minAnte > 1 ? g_Bosses[b].minAnte : 2);
			g.blind = BLIND_BOSS; g.boss = (u8)b;
			g.money = 30;
			for (int k = 0; k < 3; k++) if (j >= 0) joker_add((u8)((j + k) % JOKER_COUNT));
			for (u8 t = 0; t < HAND_COUNT; t++) g.handPlays[t] = (u16)(t + 1);   // Ox: some hand is the most played
			blind_start();
			g.target = 1000000UL;
			for (int h = 0; h < 6 && g.state == ROUND_PLAYING; h++)
			{
				u8 type; u16 sel = best_play(poker_rules(), &type);
				if (g.forced != 0xFF) for (u8 i = 0; i < g.nHand; i++) if (g.hand[i] == g.forced) sel |= (u16)(1u << i);
				if (!round_can_play(sel)) sel = 1;
				if (h == 2 && g.discardsLeft) { round_discard(1); check_state(); }
				ScoreOut o; round_play(sel, &o); round_resolve_play(); check_state(); hands++;
				if (g.state == ROUND_PLAYING && g.nJk) { joker_sell(0); }
			}
			if (g.state == ROUND_WON || g.state == ROUND_LOST) { round_end_effects(); Cash rows[CASH_MAX]; i16 t; cashout_build(rows, &t); }
		}
	}
	printf("coverage: %ld hands played with every boss and every joker\n", hands);
}

int main(int argc, char** argv)
{
	coverage();
	int runs = argc > 1 ? atoi(argv[1]) : 200;
	long wins = 0, rounds = 0, anteHist[12] = { 0 }, bossSeen[BOSS_COUNT] = { 0 }, jokerSeen[JOKER_COUNT] = { 0 };
	for (int run = 0; run < runs; run++)
	{
		rng_seed((u16)(run * 7919 + 13));
		run_new();
		int dead = 0;
		while (!run_won() && !dead)
		{
			// skip tags: sometimes skip the Small / Big blind (exercises every tag effect)
			if (blind_can_skip() && (rnd8() & 3) == 0)
			{
				blind_skip();
				u8 pk = tags_choice_effects();
				if (pk && pack_open_free(pk)) { for (u8 k = 0; k < g_packN && g_packPick; k++) if (g_packType[k]) pack_choose(k); }
				continue;
			}
			{ u8 pk = tags_choice_effects(); if (pk && pack_open_free(pk)) { for (u8 k = 0; k < g_packN && g_packPick; k++) if (g_packType[k]) pack_choose(k); } }
			blind_start();
			if (g.blind == BLIND_BOSS) bossSeen[g.boss]++;
			check_state();
			int guard = 0;
			while (g.state == ROUND_PLAYING && guard++ < 60)
			{
				u8 type; u16 sel = best_play(poker_rules(), &type);
				if (g.discardsLeft && g.handsLeft > 1 && type <= HAND_PAIR && g.score < g.target / 2)
				{
					// discard everything that is not part of the best (pair) hand, up to 5 cards
					u16 d = (u16)(((1u << g.nHand) - 1) & ~sel); u8 cnt = 0; u16 dd = 0;
					for (u8 i = 0; i < g.nHand && cnt < 5; i++) if (d & (1u << i)) { dd |= (u16)(1u << i); cnt++; }
					if (dd && round_discard(dd)) { check_state(); continue; }
				}
				if (g.forced != 0xFF) for (u8 i = 0; i < g.nHand; i++) if (g.hand[i] == g.forced) sel |= (u16)(1u << i);
				if (!round_can_play(sel)) { sel = 1; }
				ScoreOut o; round_play(sel, &o);
				CHECK(o.n <= EVENT_MAX, "events");
				round_resolve_play();
				check_state();
			}
			rounds++;
			if (g.state != ROUND_WON) { dead = 1; break; }
			round_end_effects();
			Cash rows[CASH_MAX]; i16 total; cashout_build(rows, &total);
			g.money += total;
			if (g.blind == BLIND_BOSS && g.ante == MAX_ANTE) { g.ante++; break; }
			shop_generate();
			if (g.voucher && g.money >= 14) voucher_buy();
			if (g.money > 3 && (rnd8() & 7) == 0) shop_reroll();
			for (u8 i = 0; i < g.shopN; i++) if (g.shopType[i] == 1) shop_buy(i);
			for (u8 i = 0; i < g.shopN; i++) if (g.shopType[i]) shop_buy(i);
			for (u8 i = 0; i < 2; i++) if (g.money >= 8 && g.packType[i] && pack_open(i)) { for (u8 k = 0; k < g_packN; k++) if (g_packType[k]) { if (pack_choose(k) && g_packPick == 0) break; } }
			for (u8 i = 0; i < CONS_MAX; i++) if (g.cons[i]) { u8 mn, mx; if (!cons_needs_cards(g.cons[i], &mn, &mx)) cons_use(i, 0); }
			next_blind();
		}
		for (u8 i = 0; i < g.nJk; i++) jokerSeen[g.jk[i].id]++;
		if (run_won()) wins++;
		anteHist[g.ante > 11 ? 11 : g.ante]++;
	}
	printf("sim: %d runs, %ld rounds played, %ld wins\n", runs, rounds, wins);
	long vs = 0; (void)vs;
	printf("final ante histogram: "); for (int a = 1; a <= 9; a++) printf("%d:%ld ", a, anteHist[a]); printf("\n");
	int uncovered = 0; for (int b = 0; b < BOSS_COUNT; b++) if (!bossSeen[b]) { printf("boss never met: %s\n", g_Bosses[b].name); uncovered++; }
	int jk = 0; for (int j = 0; j < JOKER_COUNT; j++) if (jokerSeen[j]) jk++;
	printf("bosses met: %d/%d, distinct jokers owned at the end of a run: %d/%d\n", BOSS_COUNT - uncovered, BOSS_COUNT, jk, JOKER_COUNT);
	printf(fails ? "SIM FAILED (%d)\n" : "sim ok\n", fails);
	return fails != 0;
}
