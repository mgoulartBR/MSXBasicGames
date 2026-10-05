// Poker hand evaluation. Faithful port of evaluate_poker_hand / get_flush / get_straight /
// get_X_same from the original (functions/misc_functions.lua), including Four Fingers,
// Shortcut and Smeared Joker behaviour.
#include "bgame.h"

static u8 suit_class(u8 s, u8 rules) { return (rules & PR_SMEARED) ? (u8)(s & 1) : s; }

// Flush: >= need cards of one suit (order of the original: Spades, Hearts, Clubs, Diamonds)
static u8 find_flush(const Card* c, u8 n, u8 rules, u8 need)
{
	static const u8 order[4] = { SUIT_S, SUIT_H, SUIT_C, SUIT_D };
	if (n > 5 || n < need) return 0;
	for (u8 k = 0; k < 4; k++)
	{
		u8 sc = suit_class(order[k], rules), m = 0, cnt = 0;
		for (u8 i = 0; i < n; i++)
			if (suit_class(C_SUIT(c[i]), rules) == sc) { m |= (u8)(1 << i); cnt++; }
		if (cnt >= need) return m;
	}
	return 0;
}

// Straight: run of `need` consecutive ranks (A low or high); Shortcut allows one gap
static u8 find_straight(const u8* rm, u8 n, u8 rules, u8 need)
{
	if (n > 5 || n < need) return 0;
	u8 t = 0, len = 0, straight = 0, skipped = 0;
	bool can_skip = (rules & PR_SHORTCUT) != 0;
	for (u8 j = 1; j <= 14; j++)
	{
		u8 ri = (u8)((j == 1 ? 14 : j) - 2);
		if (rm[ri]) { len++; skipped = 0; t |= rm[ri]; }
		else if (can_skip && !skipped && j != 14) { skipped = 1; }
		else
		{
			len = 0; skipped = 0;
			if (!straight) t = 0;
			if (straight) break;
		}
		if (len >= need) straight = 1;
	}
	return straight ? t : 0;
}

void poker_eval(const Card* c, u8 n, u8 rules, HandEval* out)
{
	u8 cnt[13], rm[13];
	for (u8 r = 0; r < 13; r++) { cnt[r] = 0; rm[r] = 0; }
	for (u8 i = 0; i < n; i++) { u8 r = C_RANK(c[i]); cnt[r]++; rm[r] |= (u8)(1 << i); }

	u8 n2 = 0, n3 = 0, n4 = 0, n5 = 0, m2a = 0, m2b = 0, m3 = 0, m4 = 0, m5 = 0;
	for (i8 r = 12; r >= 0; r--)            // highest rank first, like the original
	{
		switch (cnt[r])
		{
			case 2: if (n2++ == 0) m2a = rm[r]; else m2b = rm[r]; break;
			case 3: n3++; m3 = rm[r]; break;
			case 4: n4++; m4 = rm[r]; break;
			case 5: n5++; m5 = rm[r]; break;
		}
	}
	u8 need = (rules & PR_FOUR_FINGERS) ? 4 : 5;
	u8 fl = find_flush(c, n, rules, need);
	u8 st = find_straight(rm, n, rules, need);

	u16 ct = 0;
	if (n) ct |= 1 << HAND_HIGH_CARD;
	if (n2 || n3 || n4 || n5) ct |= 1 << HAND_PAIR;
	if (n2 == 2 || (n3 == 1 && n2 == 1)) ct |= 1 << HAND_TWO_PAIR;
	if (n3 || n4 || n5) ct |= 1 << HAND_THREE;
	if (st) ct |= 1 << HAND_STRAIGHT;
	if (fl) ct |= 1 << HAND_FLUSH;
	if (n3 && n2) ct |= 1 << HAND_FULL_HOUSE;
	if (n4 || n5) ct |= 1 << HAND_FOUR;
	if (fl && st) ct |= 1 << HAND_STRAIGHT_FLUSH;
	if (n5) ct |= 1 << HAND_FIVE;
	if (n3 && n2 && fl) ct |= 1 << HAND_FLUSH_HOUSE;
	if (n5 && fl) ct |= 1 << HAND_FLUSH_FIVE;
	out->contains = ct;

	u8 type, mask;
	if (n5 && fl)              { type = HAND_FLUSH_FIVE;  mask = m5; }
	else if (n3 && n2 && fl)   { type = HAND_FLUSH_HOUSE; mask = (u8)(m3 | m2a); }
	else if (n5)               { type = HAND_FIVE;        mask = m5; }
	else if (fl && st)         { type = HAND_STRAIGHT_FLUSH; mask = (u8)(fl | st); }
	else if (n4)               { type = HAND_FOUR;        mask = m4; }
	else if (n3 && n2)         { type = HAND_FULL_HOUSE;  mask = (u8)(m3 | m2a); }
	else if (fl)               { type = HAND_FLUSH;       mask = fl; }
	else if (st)               { type = HAND_STRAIGHT;    mask = st; }
	else if (n3)               { type = HAND_THREE;       mask = m3; }
	else if (n2 == 2)          { type = HAND_TWO_PAIR;    mask = (u8)(m2a | m2b); }
	else if (n2)               { type = HAND_PAIR;        mask = m2a; }
	else
	{
		u8 best = 0;
		for (u8 i = 1; i < n; i++) if (C_RANK(c[i]) > C_RANK(c[best])) best = i;
		type = HAND_HIGH_CARD; mask = n ? (u8)(1 << best) : 0;
	}
	out->type = type;
	out->mask = mask;
}
