// Host-side regression test for the poker evaluator: gcc -I include -I src/gen tests/host_poker.c src/game/poker.c ...
#include <stdio.h>
#include <string.h>
#include "bgame.h"
static int fails = 0;
static void chk(const char* name, const Card* c, u8 n, u8 rules, u8 type, u8 mask)
{
	HandEval e; poker_eval(c, n, rules, &e);
	if (e.type != type || e.mask != mask) { printf("FAIL %-28s got type %d mask %02X, want %d %02X\n", name, e.type, e.mask, type, mask); fails++; }
	else printf("ok   %-28s type %d mask %02X\n", name, e.type, e.mask);
}
#define H SUIT_H
#define C SUIT_C
#define D SUIT_D
#define S SUIT_S
int main(void)
{
	{ Card h[] = { CARD(H,RANK_A), CARD(S,5), CARD(D,9) }; chk("high card A", h, 3, 0, HAND_HIGH_CARD, 1); }
	{ Card h[] = { CARD(H,RANK_A), CARD(S,RANK_A), CARD(D,9) }; chk("pair of aces", h, 3, 0, HAND_PAIR, 3); }
	{ Card h[] = { CARD(H,3), CARD(S,3), CARD(D,5), CARD(C,5), CARD(H,9) }; chk("two pair", h, 5, 0, HAND_TWO_PAIR, 0x0F); }
	{ Card h[] = { CARD(H,3), CARD(S,3), CARD(D,3), CARD(C,5), CARD(H,9) }; chk("trips", h, 5, 0, HAND_THREE, 7); }
	{ Card h[] = { CARD(H,3), CARD(S,4), CARD(D,5), CARD(C,6), CARD(H,7) }; chk("straight 5-9", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,RANK_A), CARD(S,0), CARD(D,1), CARD(C,2), CARD(H,3) }; chk("wheel A-5", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,8), CARD(S,9), CARD(D,10), CARD(C,11), CARD(H,12) }; chk("broadway", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,10), CARD(S,11), CARD(D,12), CARD(C,0), CARD(H,1) }; chk("no wrap Q K A 2 3", h, 5, 0, HAND_HIGH_CARD, 4); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), CARD(H,10) }; chk("flush", h, 5, 0, HAND_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,3), CARD(H,4), CARD(H,5), CARD(H,6) }; chk("straight flush", h, 5, 0, HAND_STRAIGHT_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,2), CARD(H,6) }; chk("quads", h, 5, 0, HAND_FOUR, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,6), CARD(H,6) }; chk("full house", h, 5, 0, HAND_FULL_HOUSE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,2), CARD(H,2) }; chk("five of a kind", h, 5, 0, HAND_FIVE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,2) }; chk("flush five", h, 5, 0, HAND_FLUSH_FIVE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,6), CARD(H,6) }; chk("flush house", h, 5, 0, HAND_FLUSH_HOUSE, 0x1F); }
	// Four Fingers
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), CARD(S,10) }; chk("4F flush (4 hearts)", h, 5, PR_FOUR_FINGERS, HAND_FLUSH, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,3), CARD(D,4), CARD(C,5), CARD(H,10) }; chk("4F straight", h, 5, PR_FOUR_FINGERS, HAND_STRAIGHT, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,3), CARD(D,4), CARD(C,5) }; chk("4F straight w/ 4 cards", h, 4, PR_FOUR_FINGERS, HAND_STRAIGHT, 0x0F); }
	// Shortcut
	{ Card h[] = { CARD(H,2), CARD(S,4), CARD(D,6), CARD(C,8), CARD(H,10) }; chk("shortcut 2-4-6-8-10", h, 5, PR_SHORTCUT, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,4), CARD(D,6), CARD(C,8), CARD(H,10) }; chk("no shortcut: high card", h, 5, 0, HAND_HIGH_CARD, 0x10); }
	// Smeared: hearts+diamonds are one suit
	{ Card h[] = { CARD(H,2), CARD(D,4), CARD(H,6), CARD(D,8), CARD(H,10) }; chk("smeared flush", h, 5, PR_SMEARED, HAND_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(D,4), CARD(H,6), CARD(D,8), CARD(H,10) }; chk("no smeared: high card", h, 5, 0, HAND_HIGH_CARD, 0x10); }
	// flush needs exactly the played cards: 4 cards is no flush without Four Fingers
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8) }; chk("4 hearts no flush", h, 4, 0, HAND_HIGH_CARD, 0x08); }
	printf(fails ? "\n%d FAILED\n" : "\nall poker tests passed\n", fails);
	return fails != 0;
}
