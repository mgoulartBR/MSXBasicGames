// Spectral cards (split out of shop.c: that mapper segment is full).
#include "bgame.h"

enum { ST_NONE, ST_JOKER, ST_PLANET, ST_TAROT, ST_CARD, ST_SPECTRAL };

static void hand_remove(u8 idx)
{
	for (u8 i = idx; i + 1 < g.nHand; i++) g.hand[i] = g.hand[i + 1];
	g.nHand--;
}

static void hand_remove(u8 idx);
static void destroy_random_in_hand(void)
{
	if (!g.nHand) return;
	u8 i = rndn(g.nHand);
	card_destroyed(g.deck[g.hand[i]]); g.loc[g.hand[i]] = LOC_GONE; hand_remove(i);
}
static void add_to_hand(Card c) { deck_add(c, TRUE); }
static Card enhanced(u8 rank) { return C_SETENH(CARD(rndn(4), rank), 1 + rndn(ENH_COUNT - 1)); }

// Spectral cards
bool spectral_use(u8 slot, u8 t, u16 sel) BANKED
{
	u8 first = 0xFF;
	for (u8 i = 0; i < g.nHand; i++) if (sel & (1u << i)) { first = i; break; }
	switch (t)
	{
		case SP_WRAITH:
		{
			if (g.nJk >= joker_slots()) return FALSE;
			u8 none[(JOKER_COUNT + 7) / 8] = { 0 };
			i16 j = random_joker_r(none, 3);
			if (j < 0) return FALSE;
			cons_remove(slot); joker_add((u8)j); g.money = 0;
			return TRUE;
		}
		case SP_ANKH: case SP_HEX:
		{
			if (!g.nJk) return FALSE;
			cons_remove(slot);
			JokerInst keep = g.jk[rndn(g.nJk)], eternal[JOKER_MAX]; u8 ne = 0;
			for (u8 i = 0; i < g.nJk; i++) if ((g.jk[i].flags & JF_ETERNAL) && ne < JOKER_MAX) eternal[ne++] = g.jk[i];      // eternal Jokers survive
			g.nJk = 0;
			g.jk[g.nJk++] = keep;
			if (t == SP_HEX) g.jk[0].ed = ED_POLY;
			for (u8 i = 0; i < ne && g.nJk < joker_slots(); i++) if (eternal[i].id != keep.id || eternal[i].flags != keep.flags) g.jk[g.nJk++] = eternal[i];
			if (t == SP_ANKH && g.nJk < joker_slots()) { g.jk[g.nJk] = keep; g.jk[g.nJk].flags &= (u8)~JF_ETERNAL; g.nJk++; }
			joker_recalc_modifiers();
			return TRUE;
		}
		case SP_SOUL:
			if (!joker_random_add(4)) return FALSE;
			cons_remove(slot);
			return TRUE;
		case SP_ECTOPLASM:
		{
			u8 cand[JOKER_MAX], n = 0;
			for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].ed == ED_NONE) cand[n++] = i;
			if (!n) return FALSE;
			cons_remove(slot);
			g.jk[cand[rndn(n)]].ed = ED_NEG;
			if (g.handSizeBase > 1) g.handSizeBase--;
			return TRUE;
		}
		case SP_BLACK_HOLE:
			cons_remove(slot);
			for (u8 h = 0; h < HAND_COUNT; h++) g.handLevel[h]++;
			return TRUE;
	}
	if (!g.nHand) return FALSE;                  // every other Spectral works on the hand
	cons_remove(slot);
	switch (t)
	{
		case SP_FAMILIAR: destroy_random_in_hand(); for (u8 k = 0; k < 3; k++) add_to_hand(enhanced(RANK_J + rndn(3))); break;
		case SP_GRIM: destroy_random_in_hand(); for (u8 k = 0; k < 2; k++) add_to_hand(enhanced(RANK_A)); break;
		case SP_INCANTATION: destroy_random_in_hand(); for (u8 k = 0; k < 4; k++) add_to_hand(enhanced(rndn(9))); break;
		case SP_TALISMAN: case SP_DEJA_VU: case SP_TRANCE: case SP_MEDIUM:
		{
			Card* d = &g.deck[g.hand[first]];
			*d = C_SETSEAL(*d, t == SP_TALISMAN ? SEAL_GOLD : t == SP_DEJA_VU ? SEAL_RED : t == SP_TRANCE ? SEAL_BLUE : SEAL_PURPLE);
			break;
		}
		case SP_AURA: { Card* d = &g.deck[g.hand[first]]; *d = C_SETED(*d, 1 + rndn(3)); break; }
		case SP_SIGIL: { u8 su = rndn(4); for (u8 i = 0; i < g.nHand; i++) g.deck[g.hand[i]] = C_SETSUIT(g.deck[g.hand[i]], su); break; }
		case SP_OUIJA:
		{
			u8 r = rndn(13);
			for (u8 i = 0; i < g.nHand; i++) g.deck[g.hand[i]] = C_SETRANK(g.deck[g.hand[i]], r);
			if (g.handSizeBase > 1) g.handSizeBase--;
			break;
		}
		case SP_IMMOLATE: for (u8 k = 0; k < 5; k++) destroy_random_in_hand(); g.money += 20; break;
		case SP_CRYPTID: { Card c = g.deck[g.hand[first]]; add_to_hand(c); add_to_hand(c); break; }
	}
	hand_sort(g.sortMode);
	return TRUE;
}

