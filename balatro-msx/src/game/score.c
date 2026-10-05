// Playing a hand: poker evaluation, boss rules and card/edition effects (the per-joker phases live in score_jk.c).
#include "score_priv.h"

u8 poker_rules(void) BANKED
{
	u8 r = 0;
	if (joker_has(JK_FOUR_FINGERS)) r |= PR_FOUR_FINGERS;
	if (joker_has(JK_SHORTCUT))     r |= PR_SHORTCUT;
	if (joker_has(JK_SMEARED))      r |= PR_SMEARED;
	return r;
}

bool card_is_face(Card c) BANKED
{
	u8 r = C_RANK(c);
	if (C_ENH(c) == ENH_STONE) return FALSE;
	return (r >= RANK_J && r <= RANK_K) || joker_has(JK_PAREIDOLIA);
}

bool bossActive(u8 b) BANKED { return g.blind == BLIND_BOSS && g.boss == b && !g.bossOff; }

// Boss debuffs on playing cards (Blind:debuff_card)
bool card_is_debuffed(u8 slot) BANKED
{
	if (g.blind != BLIND_BOSS || g.bossOff) return FALSE;
	Card c = g.deck[slot];
	switch (g.boss)
	{
		case BS_CLUB:   return suit_is(c, SUIT_C);
		case BS_GOAD:   return suit_is(c, SUIT_S);
		case BS_WINDOW: return suit_is(c, SUIT_D);
		case BS_HEAD:   return suit_is(c, SUIT_H);
		case BS_PLANT:  return card_is_face(c);
		case BS_PILLAR: return (g.dflag[slot] & DF_PILLAR) != 0;
		case BS_FINAL_LEAF: return TRUE;
	}
	return FALSE;
}

u32 hand_chips(u8 t) BANKED { return (u32)g_Hands[t].baseChips + (u32)g_Hands[t].lvlChips * (g.handLevel[t] - 1); }
u16 hand_mult(u8 t) BANKED { return (u16)(g_Hands[t].baseMult + g_Hands[t].lvlMult * (g.handLevel[t] - 1)); }

// ---------------------------------------------------------------------------
// round_play: removes the highlighted cards from the hand and scores them
// ---------------------------------------------------------------------------
void round_play(u16 sel, ScoreOut* o) BANKED
{
	SC s; s.o = o; s.chips = 0; s.mult = 0;
	o->n = 0; o->debuffed = 0; o->total = 0;

	Card pc[PLAY_MAX]; u8 np = 0, nh = 0, nhand[HAND_MAX];
	for (u8 i = 0; i < g.nHand; i++)
	{
		if ((sel & (1u << i)) && np < PLAY_MAX)
		{
			u8 slot = g.hand[i];
			g.played[np] = slot; pc[np] = g.deck[slot]; np++;
			g.loc[slot] = LOC_PLAY; g.dflag[slot] &= (u8)~DF_FD;       // played cards turn face up
		}
		else nhand[nh++] = g.hand[i];
	}
	for (u8 i = 0; i < nh; i++) g.hand[i] = nhand[i];
	g.nHand = nh; g.nPlayed = np;
	g.handsLeft--; g.handsPlayed++; g.handsPlayedRun++;

	HandEval he;
	{                                                             // Stone Cards take no part in the hand type but always score
		Card ec[PLAY_MAX]; u8 emap[PLAY_MAX], ne = 0, stones = 0, m = 0;
		for (u8 k = 0; k < np; k++)
			if (C_ENH(pc[k]) == ENH_STONE) stones |= (u8)(1 << k); else { ec[ne] = pc[k]; emap[ne++] = k; }
		poker_eval(ec, ne, poker_rules(), &he);
		for (u8 i = 0; i < ne; i++) if (he.mask & (1 << i)) m |= (u8)(1 << emap[i]);
		he.mask = (u8)(m | stones);
	}
	u8 type = he.type, mask = he.mask;
	u16 ct = he.contains;
	if (joker_has(JK_SPLASH)) mask = (u8)((1 << np) - 1);
	o->type = type; o->mask = mask;

	g.handPlays[type]++; g.lastHandType = type;
	if (g.playedCnt[type] < 255) g.playedCnt[type]++;

	// Blind: press_play (The Hook, The Tooth)
	if (bossActive(BS_HOOK))
	{
		for (u8 k = 0; k < 2 && g.nHand > 0; k++)
		{
			u8 r = rndn(g.nHand);
			g.loc[g.hand[r]] = LOC_DISCARD;
			for (u8 i = r; i + 1 < g.nHand; i++) g.hand[i] = g.hand[i + 1];
			g.nHand--;
		}
	}
	// Blind: debuff_hand
	bool dbf = FALSE;
	if (g.blind == BLIND_BOSS && !g.bossOff)
	{
		if (g.boss == BS_PSYCHIC && np < 5) dbf = TRUE;
		if (g.boss == BS_EYE) { if (g.eyeMask & (1u << type)) dbf = TRUE; else g.eyeMask |= (u16)(1u << type); }
		if (g.boss == BS_MOUTH) { if (g.mouthHand != 0xFF && g.mouthHand != type) dbf = TRUE; else g.mouthHand = type; }
		if (g.boss == BS_ARM && g.handLevel[type] > 1) { g.handLevel[type]--; dbf = FALSE; }
		if (g.boss == BS_OX && type == g.mostPlayed) { g.money = 0; push(&s, EV_MONEY, SRC_NONE, 0); }
		if (g.boss == BS_TOOTH) { g.money -= np; push(&s, EV_MONEY, SRC_NONE, (i16)-np); }
	}
	if (dbf)
	{
		o->debuffed = 1;
		s.chips = 0; s.mult = 0; push(&s, EV_DEBUFF, SRC_NONE, 0);
		return;
	}

	// level-based start values; jokers' before phase may level the hand up (Space Joker)
	for (u8 i = 0; i < g.nJk; i++) if (!(g.jk[i].flags & JF_DEBUFF)) joker_before(&s, i, ct, type, np, pc, mask);
	s.chips = hand_chips(type);
	s.mult = (u32)hand_mult(type) * 100;
	if (bossActive(BS_FLINT))
	{
		s.chips = (s.chips + 1) / 2;
		s.mult = (s.mult / 100 + 1) / 2 * 100; if (s.mult < 100) s.mult = 100;
	}
	push(&s, EV_BASE, SRC_NONE, type);

	// first scoring face card (Photograph)
	u8 firstFace = 0xFF;
	for (u8 k = 0; k < np; k++) if ((mask & (1 << k)) && card_is_face(pc[k])) { firstFace = k; break; }

	// scored cards
	for (u8 k = 0; k < np; k++)
	{
		if (!(mask & (1 << k))) continue;
		if (card_is_debuffed(g.played[k])) { push(&s, EV_DEBUFF, SRC_PLAY(k), 0); continue; }
		for (u8 rep = 0; rep < 1 + (C_SEAL(pc[k]) == SEAL_RED); rep++)       // Red Seal: the card scores twice
		{
			Card c = pc[k]; u8 enh = C_ENH(c), ed = C_ED(c), src = SRC_PLAY(k);
			u8 nom = (enh == ENH_STONE) ? 0 : (u8)C_NOMINAL(c);
			s.chips += nom;
			push(&s, EV_CARD, src, nom);
			if (rep) text(&s, src, TX_AGAIN);
			switch (enh)
			{
				case ENH_BONUS: add_chips(&s, src, 30); break;
				case ENH_STONE: add_chips(&s, src, 50); break;
				case ENH_MULT:  add_mult(&s, src, 4); break;
				case ENH_GLASS: x_mult(&s, src, 200); break;
				case ENH_LUCKY:
					if (rnd_odds(5)) add_mult(&s, src, 20);
					if (rnd_odds(15)) add_money(&s, src, 20);
					break;
			}
			if (C_SEAL(c) == SEAL_GOLD) add_money(&s, src, 3);
			if (ed == ED_FOIL) add_chips(&s, src, 50);
			else if (ed == ED_HOLO) add_mult(&s, src, 10);
			else if (ed == ED_POLY) x_mult(&s, src, 150);
			for (u8 i = 0; i < g.nJk; i++)
				if (!(g.jk[i].flags & JF_DEBUFF)) joker_on_card(&s, i, c, k == firstFace);
		}
		if (C_ENH(pc[k]) == ENH_GLASS && rnd_odds(4)) g.dflag[g.played[k]] |= DF_BREAK;   // shatters after scoring
	}

	// cards held in hand
	u8 lowest = 0xFF, lowId = 15;
	for (u8 h = 0; h < g.nHand; h++)
	{
		u8 id = C_ID(g.deck[g.hand[h]]);
		if (lowId >= id) { lowId = id; lowest = h; }
	}
	for (u8 h = 0; h < g.nHand; h++)
	{
		if (C_ENH(g.deck[g.hand[h]]) == ENH_STEEL && !card_is_debuffed(g.hand[h])) x_mult(&s, SRC_HELD(h), 150);
		for (u8 i = 0; i < g.nJk; i++)
			if (!(g.jk[i].flags & JF_DEBUFF)) joker_on_held(&s, i, h, lowest);
	}

	// jokers, left to right
	for (u8 i = 0; i < g.nJk; i++)
		if (!(g.jk[i].flags & JF_DEBUFF))
		{
			u8 ed = g.jk[i].ed;                                   // joker editions: Foil/Holo before the effect, Polychrome after
			if (ed == ED_FOIL) add_chips(&s, SRC_JOKER(i), 50);
			else if (ed == ED_HOLO) add_mult(&s, SRC_JOKER(i), 10);
			joker_main(&s, i, ct, type, np, pc, mask);
			if (ed == ED_POLY) x_mult(&s, SRC_JOKER(i), 150);
		}

	// result: chips x mult (split to stay in 32 bits)
	u32 m = s.mult / 100, f = s.mult % 100;
	o->total = s.chips * m + s.chips * f / 100;
	g.lastTotal = o->total;

	// after-scoring joker phase
	for (u8 i = 0; i < g.nJk; i++)
	{
		if (g.jk[i].id == JK_ICE_CREAM)
		{
			g.jk[i].v -= 5;
			if (g.jk[i].v <= 0) { text(&s, SRC_JOKER(i), TX_EATEN); g.jk[i].flags |= 0x80; }   // melted at resolve
		}
	}
}
