// Joker effects that fire outside the scoring pass: Blind selection, discards, sales, packs, round end.
// (The scoring phases live in score_jk.c; acquiring / removing Jokers in jokers.c.)
#include "bgame.h"

static Card random_deck_card(void)                  // a card that is still in the deck
{
	u8 n = 0, cand[DECK_MAX];
	for (u8 s = 0; s < g.nDeck; s++) if (g.loc[s] != LOC_GONE) cand[n++] = s;
	return n ? g.deck[cand[rndn(n)]] : CARD(0, 0);
}

static void new_tarot(void) { cons_add(CONS_TAROT(rndn(TAROT_COUNT))); }

bool deck_add(Card c, bool toHand) BANKED
{
	if (g.nDeck >= DECK_MAX) return FALSE;
	u8 slot = g.nDeck++;
	g.deck[slot] = c; g.dflag[slot] = 0; g.dbonus[slot] = 0; g.loc[slot] = LOC_PILE;
	if (toHand && g.nHand < HAND_MAX) { g.loc[slot] = LOC_HAND; g.hand[g.nHand++] = slot; }
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == JK_HOLOGRAM) g.jk[i].v += 25;
	return TRUE;
}

void card_destroyed(Card c) BANKED
{
	for (u8 i = 0; i < g.nJk; i++)
	{
		if (g.jk[i].id == JK_GLASS && C_ENH(c) == ENH_GLASS) g.jk[i].v += 75;
		if (g.jk[i].id == JK_CAINO && card_is_face(c)) g.jk[i].v += 100;
	}
}

void joker_blind_select(void) BANKED
{
	bool boss = g.blind == BLIND_BOSS;
	if (boss && joker_has(JK_CHICOT)) g.bossOff = 1;
	for (u8 i = 0; i < g.nJk; i++)
	{
		JokerInst* j = &g.jk[i];
		switch (j->id)
		{
			case JK_CEREMONIAL:
				if (i + 1 < g.nJk && !(g.jk[i + 1].flags & JF_ETERNAL)) { j->v += (i16)(2 * joker_sell_value(i + 1)); joker_remove(i + 1); }
				break;
			case JK_MARBLE: deck_add(C_SETENH(CARD(rndn(4), rndn(13)), ENH_STONE), FALSE); break;
			case JK_MADNESS:
				if (!boss)
				{
					j->v += 50;
					if (g.nJk > 1)
					{
						u8 r = rndn(g.nJk - 1);
						if (r >= i) r++;
						if (!(g.jk[r].flags & JF_ETERNAL)) { joker_remove(r); if (r < i) i--; }
					}
				}
				break;
			case JK_RIFF_RAFF: joker_random_add(1); joker_random_add(1); break;
			case JK_CARTOMANCER: new_tarot(); break;
			case JK_HIT_THE_ROAD: j->v = 100; break;
			case JK_ANCIENT: case JK_CASTLE: j->aux = C_SUIT(random_deck_card()); break;
			case JK_IDOL: j->aux = (u8)C_BASE(random_deck_card()); break;
			case JK_MAIL: j->aux = C_RANK(random_deck_card()); break;
		}
	}
}

void joker_hand_drawn(void) BANKED
{
	for (u8 i = 0; i < g.nJk; i++)
		if (g.jk[i].id == JK_CERTIFICATE)
		{
			Card c = CARD(rndn(4), rndn(13));
			deck_add(C_SETSEAL(c, 1 + rndn(SEAL_COUNT - 1)), TRUE);
		}
}

void joker_on_discard(Card c) BANKED
{
	if (C_ENH(c) == ENH_STONE) return;
	for (u8 i = 0; i < g.nJk; i++)
	{
		JokerInst* j = &g.jk[i];
		if (j->flags & JF_DEBUFF) continue;
		switch (j->id)
		{
			case JK_CASTLE: if (C_ENH(c) == ENH_WILD || C_SUIT(c) == j->aux) j->v += 3; break;
			case JK_MAIL: if (C_RANK(c) == j->aux) g.money += 5; break;
			case JK_HIT_THE_ROAD: if (C_RANK(c) == RANK_J) j->v += 50; break;
			case JK_YORICK: if (++j->aux >= 23) { j->aux = 0; j->v += 100; } break;
		}
	}
}

void joker_on_discard_hand(const u8* slots, u8 n) BANKED
{
	if (g.discardsUsed != 0) return;                                   // only the first discard of the round
	for (u8 i = 0; i < g.nJk; i++)
	{
		if (g.jk[i].flags & JF_DEBUFF) continue;
		if (g.jk[i].id == JK_TRADING && n == 1)
		{
			g.money += 3; card_destroyed(g.deck[slots[0]]); g.loc[slots[0]] = LOC_GONE;
		}
		else if (g.jk[i].id == JK_BURNT)
		{
			Card c[5]; HandEval he;
			for (u8 k = 0; k < n && k < 5; k++) c[k] = g.deck[slots[k]];
			poker_eval(c, n > 5 ? 5 : n, poker_rules(), &he);
			g.handLevel[he.type]++;
		}
	}
}

void joker_sold(const JokerInst* sold) BANKED
{
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == JK_CAMPFIRE) g.jk[i].v += 25;
	if (!sold) return;
	if (sold->id == JK_DIET_COLA) tag_gain(TG_DOUBLE);
	if (sold->id == JK_LUCHADOR && g.blind == BLIND_BOSS && g.state == ROUND_PLAYING) g.bossOff = 1;
	if (sold->id == JK_INVISIBLE && sold->v >= 2 && g.nJk && g.nJk < joker_slots())
	{
		g.jk[g.nJk] = g.jk[rndn(g.nJk)];
		g.nJk++;
	}
}

void joker_round_end2(void) BANKED
{
	for (u8 i = 0; i < g.nJk; i++)
	{
		JokerInst* j = &g.jk[i];
		switch (j->id)
		{
			case JK_INVISIBLE: if (j->v < 100) j->v++; break;
			case JK_CAMPFIRE: if (g.blind == BLIND_BOSS) j->v = 100; break;
			case JK_GIFT: for (u8 k = 0; k < g.nJk; k++) g.jk[k].sell++; break;
		}
	}
}

void pack_skipped(void) BANKED
{
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == JK_RED_CARD) g.jk[i].v += 3;
}

void pack_opened(void) BANKED
{
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == JK_HALLUCINATION && rnd_odds(2)) new_tarot();
}

void shop_leave(void) BANKED
{
	for (u8 i = 0; i < g.nJk; i++)
		if (g.jk[i].id == JK_PERKEO)
		{
			u8 have[CONS_MAX], n = 0;
			for (u8 k = 0; k < CONS_MAX; k++) if (g.cons[k]) have[n++] = g.cons[k];
			if (n) cons_add_ed(have[rndn(n)], TRUE);                        // the copy is Negative: it needs no free slot
		}
}
