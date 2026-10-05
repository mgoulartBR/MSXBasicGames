// Playing a hand: poker evaluation, boss rules and card/edition effects (the per-joker phases live in score_jk.c).
#include "score_priv.h"

// Blueprint / Brainstorm: index of the Joker whose ability the Joker at i uses (0xFF = no target)
static u8 eff_index(u8 i)
{
	for (u8 guard = 0; guard < 8; guard++)
	{
		u8 id = g.jk[i].id;
		if (id == JK_BLUEPRINT) { if (i + 1 >= g.nJk) return 0xFF; i++; }
		else if (id == JK_BRAINSTORM) { if (i == 0) return 0xFF; i = 0; }
		else return (g.jk[i].flags & JF_DEBUFF) ? 0xFF : i;
	}
	return 0xFF;
}

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

// extra repetitions of a scoring card granted by Jokers (Hack, Hanging Chad, Sock and Buskin, Seltzer, Dusk; Blueprint copies them)
static u8 joker_retriggers(u8 k, Card c, u8 firstScoring)
{
	u8 n = 0, id = C_ID(c);
	bool face = card_is_face(c);
	if (C_ENH(c) == ENH_STONE) id = 0;
	for (u8 i = 0; i < g.nJk; i++)
	{
		u8 t = eff_index(i);
		if (t == 0xFF) continue;
		switch (g.jk[t].id)
		{
			case JK_HACK: if (id >= 2 && id <= 5) n++; break;
			case JK_HANGING_CHAD: if (k == firstScoring) n += 2; break;
			case JK_SOCK_AND_BUSKIN: if (face) n++; break;
			case JK_SELZER: n++; break;
			case JK_DUSK: if (g.handsLeft == 0) n++; break;
		}
	}
	return n;
}

static void lucky_cat(void) { for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == JK_LUCKY_CAT) g.jk[i].v += 25; }

static void matador(SC* s)                                      // Boss Blind ability triggered
{
	for (u8 i = 0; i < g.nJk; i++)
		if (eff_index(i) != 0xFF && g.jk[eff_index(i)].id == JK_MATADOR) add_money(s, SRC_JOKER(i), 8);
}

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
		matador(&s);
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
		if (g.boss == BS_ARM && g.handLevel[type] > 1) { g.handLevel[type]--; dbf = FALSE; matador(&s); }
		if (g.boss == BS_OX && type == g.mostPlayed) { g.money = 0; push(&s, EV_MONEY, SRC_NONE, 0); matador(&s); }
		if (g.boss == BS_TOOTH) { g.money -= np; push(&s, EV_MONEY, SRC_NONE, (i16)-np); matador(&s); }
	}
	if (dbf)
	{
		matador(&s);
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

	u8 firstScoring = 0xFF;
	for (u8 k = 0; k < np; k++) if (mask & (1 << k)) { firstScoring = k; break; }

	// scored cards
	for (u8 k = 0; k < np; k++)
	{
		if (!(mask & (1 << k))) continue;
		if (card_is_debuffed(g.played[k])) { push(&s, EV_DEBUFF, SRC_PLAY(k), 0); continue; }
		u8 reps = (u8)(1 + (C_SEAL(pc[k]) == SEAL_RED) + joker_retriggers(k, pc[k], firstScoring));
		g_curSlot = g.played[k];
		for (u8 rep = 0; rep < reps; rep++)       // Red Seal and retrigger Jokers: the card scores again
		{
			Card c = pc[k]; u8 enh = C_ENH(c), ed = C_ED(c), src = SRC_PLAY(k);
			u8 nom = (enh == ENH_STONE) ? 0 : (u8)C_NOMINAL(c);
			s.chips += nom + g.dbonus[g.played[k]];
			push(&s, EV_CARD, src, (i16)(nom + g.dbonus[g.played[k]]));
			if (rep) text(&s, src, TX_AGAIN);
			switch (enh)
			{
				case ENH_BONUS: add_chips(&s, src, 30); break;
				case ENH_STONE: add_chips(&s, src, 50); break;
				case ENH_MULT:  add_mult(&s, src, 4); break;
				case ENH_GLASS: x_mult(&s, src, 200); break;
				case ENH_LUCKY:
					if (rnd_odds(5)) { add_mult(&s, src, 20); lucky_cat(); }
					if (rnd_odds(15)) { add_money(&s, src, 20); lucky_cat(); }
					break;
			}
			if (C_SEAL(c) == SEAL_GOLD) add_money(&s, src, 3);
			if (ed == ED_FOIL) add_chips(&s, src, 50);
			else if (ed == ED_HOLO) add_mult(&s, src, 10);
			else if (ed == ED_POLY) x_mult(&s, src, 150);
			for (u8 i = 0; i < g.nJk; i++)
			{
				u8 t = eff_index(i);
				if (t == 0xFF) continue;
				g_blueSrc = (t != i) ? SRC_JOKER(i) : 0xFF;
				joker_on_card(&s, t, c, k == firstFace);
			}
			g_blueSrc = 0xFF;
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
	u8 mimes = 0;
	for (u8 i = 0; i < g.nJk; i++) { u8 t = eff_index(i); if (t != 0xFF && g.jk[t].id == JK_MIME) mimes++; }
	for (u8 h = 0; h < g.nHand; h++)
		for (u8 rep = 0; rep <= mimes; rep++)                       // Mime retriggers cards held in hand
		{
			if (C_ENH(g.deck[g.hand[h]]) == ENH_STEEL && !card_is_debuffed(g.hand[h])) x_mult(&s, SRC_HELD(h), 150);
			for (u8 i = 0; i < g.nJk; i++)
			{
				u8 t = eff_index(i);
				if (t == 0xFF) continue;
				g_blueSrc = (t != i) ? SRC_JOKER(i) : 0xFF;
				joker_on_held(&s, t, h, lowest);
			}
			g_blueSrc = 0xFF;
		}

	// jokers, left to right
	for (u8 i = 0; i < g.nJk; i++)
		if (!(g.jk[i].flags & JF_DEBUFF))
		{
			u8 ed = g.jk[i].ed, t = eff_index(i);                 // joker editions: Foil/Holo before the effect, Polychrome after
			if (ed == ED_FOIL) add_chips(&s, SRC_JOKER(i), 50);
			else if (ed == ED_HOLO) add_mult(&s, SRC_JOKER(i), 10);
			if (t != 0xFF)
			{
				g_blueSrc = (t != i) ? SRC_JOKER(i) : 0xFF;
				joker_main(&s, t, ct, type, np, pc, mask);
				g_blueSrc = 0xFF;
			}
			if (ed == ED_POLY) x_mult(&s, SRC_JOKER(i), 150);
			if (g_Jokers[g.jk[i].id].rarity == 2)                  // Baseball Card: each Uncommon Joker gives X1.5 more
				for (u8 b = 0; b < g.nJk; b++)
				{
					u8 tb = eff_index(b);
					if (b != i && tb != 0xFF && g.jk[tb].id == JK_BASEBALL) x_mult(&s, SRC_JOKER(b), 150);
				}
		}

	// result: chips x mult (split to stay in 32 bits)
	u32 m = s.mult / 100, f = s.mult % 100;
	o->total = s.chips * m + s.chips * f / 100;
	g.lastTotal = o->total;

	// after-scoring joker phase
	for (u8 i = 0; i < g.nJk; i++)
	{
		if (!(g.jk[i].flags & JF_DEBUFF)) joker_after(&s, i);
		if (g.jk[i].id == JK_ICE_CREAM)
		{
			g.jk[i].v -= 5;
			if (g.jk[i].v <= 0) { text(&s, SRC_JOKER(i), TX_EATEN); g.jk[i].flags |= 0x80; }   // melted at resolve
		}
	}
}
