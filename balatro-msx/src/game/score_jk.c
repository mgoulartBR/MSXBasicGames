// Per-joker scoring phases (context.individual / held / joker_main / before), ported from Card:calculate_joker.
#include "score_priv.h"

u8 g_blueSrc = 0xFF, g_curSlot;

// ---------------------------------------------------------------------------
// per scoring card effects of the jokers (context.individual, cardarea = play)
// ---------------------------------------------------------------------------
void joker_on_card(SC* s, u8 ji, Card c, bool firstFace) BANKED
{
	JokerInst* j = &g.jk[ji];
	u8 src = SRCJ(ji), id = C_ID(c);
	if (C_ENH(c) == ENH_STONE) return;                           // stone cards have no rank or suit
	switch (j->id)
	{
		case JK_GREEDY_JOKER:    if (suit_is(c, SUIT_D)) add_mult(s, src, 3); break;
		case JK_LUSTY_JOKER:     if (suit_is(c, SUIT_H)) add_mult(s, src, 3); break;
		case JK_WRATHFUL_JOKER:  if (suit_is(c, SUIT_S)) add_mult(s, src, 3); break;
		case JK_GLUTTENOUS_JOKER:if (suit_is(c, SUIT_C)) add_mult(s, src, 3); break;
		case JK_SCARY_FACE:      if (card_is_face(c)) add_chips(s, src, 30); break;
		case JK_SMILEY:          if (card_is_face(c)) add_mult(s, src, 5); break;
		case JK_SCHOLAR:         if (id == 14) { add_chips(s, src, 20); add_mult(s, src, 4); } break;
		case JK_WALKIE_TALKIE:   if (id == 10 || id == 4) { add_chips(s, src, 10); add_mult(s, src, 4); } break;
		case JK_BUSINESS:        if (card_is_face(c) && rnd_odds(2)) add_money(s, src, 2); break;
		case JK_FIBONACCI:       if (id == 2 || id == 3 || id == 5 || id == 8 || id == 14) add_mult(s, src, 8); break;
		case JK_EVEN_STEVEN:     if (id <= 10 && (id & 1) == 0) add_mult(s, src, 4); break;
		case JK_ODD_TODD:        if ((id <= 10 && (id & 1)) || id == 14) add_chips(s, src, 31); break;
		case JK_ROUGH_GEM:       if (suit_is(c, SUIT_D)) add_money(s, src, 1); break;
		case JK_ONYX_AGATE:      if (suit_is(c, SUIT_C)) add_mult(s, src, 7); break;
		case JK_ARROWHEAD:       if (suit_is(c, SUIT_S)) add_chips(s, src, 50); break;
		case JK_BLOODSTONE:      if (suit_is(c, SUIT_H) && rnd_odds(2)) x_mult(s, src, 150); break;
		case JK_PHOTOGRAPH:      if (firstFace) x_mult(s, src, 200); break;
		case JK_WEE:             if (id == 2 && g_blueSrc == 0xFF) { j->v += 8; text(s, src, TX_UPGRADE); } break;
		case JK_ANCIENT:         if (suit_is(c, j->aux)) x_mult(s, src, 150); break;
		case JK_IDOL:            if (C_BASE(c) == j->aux) x_mult(s, src, 200); break;
		case JK_TRIBOULET:       if (C_RANK(c) == RANK_K || C_RANK(c) == RANK_Q) x_mult(s, src, 200); break;
		case JK_TICKET:          if (C_ENH(c) == ENH_GOLD) add_money(s, src, 4); break;
		case JK_8_BALL:          if (id == 8 && rnd_odds(4)) cons_add(CONS_TAROT(rndn(TAROT_COUNT))); break;
		case JK_HIKER:           if (g_blueSrc == 0xFF && g.dbonus[g_curSlot] < 250) { g.dbonus[g_curSlot] += 5; text(s, src, TX_UPGRADE); } break;
	}
}

// ---------------------------------------------------------------------------
// held-in-hand effects (cardarea = hand)
// ---------------------------------------------------------------------------
void joker_on_held(SC* s, u8 ji, u8 hi, u8 lowestIdx) BANKED
{
	JokerInst* j = &g.jk[ji];
	Card c = g.deck[g.hand[hi]];
	u8 src = SRCJ(ji);
	if (card_is_debuffed(g.hand[hi])) return;
	switch (j->id)
	{
		case JK_SHOOT_THE_MOON:    if (C_RANK(c) == RANK_Q) add_mult(s, src, 13); break;
		case JK_BARON:             if (C_RANK(c) == RANK_K) x_mult(s, src, 150); break;
		case JK_RESERVED_PARKING:  if (card_is_face(c) && rnd_odds(2)) add_money(s, src, 1); break;
		case JK_RAISED_FIST:       if (hi == lowestIdx) add_mult(s, src, (i16)(2 * C_NOMINAL(c))); break;
	}
}

// ---------------------------------------------------------------------------
// main joker phase (context.joker_main), left to right
// ---------------------------------------------------------------------------
void joker_main(SC* s, u8 ji, u16 ct, u8 type, u8 nPlayed, const Card* pc, u8 mask) BANKED
{
	JokerInst* j = &g.jk[ji];
	u8 src = SRCJ(ji);
	switch (j->id)
	{
		case JK_JOKER:        add_mult(s, src, 4); break;
		case JK_JOLLY:        if (CONTAINS(HAND_PAIR)) add_mult(s, src, 8); break;
		case JK_ZANY:         if (CONTAINS(HAND_THREE)) add_mult(s, src, 12); break;
		case JK_MAD:          if (CONTAINS(HAND_TWO_PAIR)) add_mult(s, src, 10); break;
		case JK_CRAZY:        if (CONTAINS(HAND_STRAIGHT)) add_mult(s, src, 12); break;
		case JK_DROLL:        if (CONTAINS(HAND_FLUSH)) add_mult(s, src, 10); break;
		case JK_SLY:          if (CONTAINS(HAND_PAIR)) add_chips(s, src, 50); break;
		case JK_WILY:         if (CONTAINS(HAND_THREE)) add_chips(s, src, 100); break;
		case JK_CLEVER:       if (CONTAINS(HAND_TWO_PAIR)) add_chips(s, src, 80); break;
		case JK_DEVIOUS:      if (CONTAINS(HAND_STRAIGHT)) add_chips(s, src, 100); break;
		case JK_CRAFTY:       if (CONTAINS(HAND_FLUSH)) add_chips(s, src, 80); break;
		case JK_HALF:         if (nPlayed <= 3) add_mult(s, src, 20); break;
		case JK_STENCIL:
		{
			u8 empty = (u8)(joker_slots() - g.nJk);
			if (empty > 0) x_mult(s, src, (i16)((empty + joker_count(JK_STENCIL)) * 100));
			break;
		}
		case JK_ABSTRACT:     add_mult(s, src, (i16)(3 * g.nJk)); break;
		case JK_ACROBAT:      if (g.handsLeft == 0) x_mult(s, src, 300); break;
		case JK_MYSTIC_SUMMIT:if (g.discardsLeft == 0) add_mult(s, src, 15); break;
		case JK_MISPRINT:     add_mult(s, src, (i16)rndn(24)); break;
		case JK_BANNER:       if (g.discardsLeft > 0) add_chips(s, src, (i16)(30 * g.discardsLeft)); break;
		case JK_STUNTMAN:     add_chips(s, src, 250); break;
		case JK_SUPERNOVA:    add_mult(s, src, (i16)g.handPlays[type]); break;
		case JK_WEE:          if (j->v > 0) add_chips(s, src, j->v); break;
		case JK_RUNNER:
		case JK_SQUARE:
		case JK_ICE_CREAM:    if (j->v > 0) add_chips(s, src, j->v); break;
		case JK_POPCORN:
		case JK_GREEN_JOKER:
		case JK_RIDE_THE_BUS: if (j->v > 0) add_mult(s, src, j->v); break;
		case JK_SWASHBUCKLER:
		{
			i16 sum = 0;
			for (u8 k = 0; k < g.nJk; k++) if (k != ji) sum += joker_sell_value(k);
			if (sum > 0) add_mult(s, src, sum);
			break;
		}
		case JK_GROS_MICHEL:  add_mult(s, src, 15); break;
		case JK_CAVENDISH:    x_mult(s, src, 300); break;
		case JK_CARD_SHARP:   if (g.playedCnt[type] > 1) x_mult(s, src, 300); break;
		case JK_BOOTSTRAPS:
			if (g.money >= 5) add_mult(s, src, (i16)(2 * (g.money / 5)));
			break;
		case JK_BULL:         if (g.money > 0) add_chips(s, src, (i16)(2 * g.money)); break;
		case JK_BLUE_JOKER:   if (g.nPile > 0) add_chips(s, src, (i16)(2 * g.nPile)); break;
		case JK_BLACKBOARD:
		{
			bool all = TRUE;
			for (u8 k = 0; k < g.nHand; k++)
			{
				Card c = g.deck[g.hand[k]];
				if (!(suit_is(c, SUIT_C) || suit_is(c, SUIT_S))) { all = FALSE; break; }
			}
			if (all) x_mult(s, src, 300);
			break;
		}
		case JK_FLOWER_POT:
		{
			u8 got = 0;
			for (u8 k = 0; k < nPlayed; k++)
			{
				if (!(mask & (1 << k))) continue;
				Card c = pc[k];
				if (suit_is(c, SUIT_H) && !(got & 1)) got |= 1;
				else if (suit_is(c, SUIT_D) && !(got & 2)) got |= 2;
				else if (suit_is(c, SUIT_S) && !(got & 4)) got |= 4;
				else if (suit_is(c, SUIT_C) && !(got & 8)) got |= 8;
			}
			if (got == 15) x_mult(s, src, 300);
			break;
		}
		case JK_SEEING_DOUBLE:
		{
			bool club = FALSE, other = FALSE;
			for (u8 k = 0; k < nPlayed; k++)
			{
				if (!(mask & (1 << k))) continue;
				if (suit_is(pc[k], SUIT_C)) club = TRUE;
				if (suit_is(pc[k], SUIT_H) || suit_is(pc[k], SUIT_D) || suit_is(pc[k], SUIT_S)) other = TRUE;
			}
			if (club && other) x_mult(s, src, 200);
			break;
		}
		case JK_DUO:    if (CONTAINS(HAND_PAIR)) x_mult(s, src, 200); break;
		case JK_TRIO:   if (CONTAINS(HAND_THREE)) x_mult(s, src, 300); break;
		case JK_FAMILY: if (CONTAINS(HAND_FOUR)) x_mult(s, src, 400); break;
		case JK_ORDER:  if (CONTAINS(HAND_STRAIGHT)) x_mult(s, src, 300); break;
		case JK_TRIBE:  if (CONTAINS(HAND_FLUSH)) x_mult(s, src, 200); break;
		case JK_CONSTELLATION:
		case JK_RAMEN:
		case JK_HOLOGRAM: case JK_VAMPIRE: case JK_OBELISK: case JK_LUCKY_CAT: case JK_GLASS: case JK_CAINO: case JK_YORICK:
		case JK_MADNESS: case JK_HIT_THE_ROAD: case JK_CAMPFIRE:
			if (j->v > 100) x_mult(s, src, j->v);
			break;
		case JK_THROWBACK:    if (g.skips) x_mult(s, src, (i16)(100 + 25 * g.skips)); break;
		case JK_LOYALTY_CARD: if (j->aux == 5) x_mult(s, src, 400); break;
		case JK_RED_CARD: case JK_FLASH: case JK_TROUSERS: case JK_CEREMONIAL:
			if (j->v > 0) add_mult(s, src, j->v);
			break;
		case JK_CASTLE:       if (j->v > 0) add_chips(s, src, j->v); break;
		case JK_FORTUNE_TELLER: if (g.tarotsUsed) add_mult(s, src, g.tarotsUsed); break;
		case JK_STEEL_JOKER: case JK_STONE: case JK_DRIVERS_LICENSE:
		{
			u8 steel = 0, stone = 0, enh = 0;
			for (u8 k = 0; k < g.nDeck; k++)
			{
				if (g.loc[k] == LOC_GONE) continue;
				u8 e = C_ENH(g.deck[k]);
				steel += (e == ENH_STEEL); stone += (e == ENH_STONE); enh += (e != ENH_NONE);
			}
			if (j->id == JK_STEEL_JOKER && steel) x_mult(s, src, (i16)(100 + 20 * steel));
			else if (j->id == JK_STONE && stone) add_chips(s, src, (i16)(25 * stone));
			else if (j->id == JK_DRIVERS_LICENSE && enh >= 16) x_mult(s, src, 300);
			break;
		}
		case JK_EROSION:
		{
			u8 n = 0;
			for (u8 k = 0; k < g.nDeck; k++) if (g.loc[k] != LOC_GONE) n++;
			if (n < 52) add_mult(s, src, (i16)(4 * (52 - n)));
			break;
		}
		case JK_SUPERPOSITION:
			if (CONTAINS(HAND_STRAIGHT))
				for (u8 k = 0; k < nPlayed; k++)
					if ((mask & (1 << k)) && C_RANK(pc[k]) == RANK_A && C_ENH(pc[k]) != ENH_STONE) { cons_add(CONS_TAROT(rndn(TAROT_COUNT))); break; }
			break;
		case JK_SEANCE:       if (CONTAINS(HAND_STRAIGHT_FLUSH)) cons_add(CONS_SPECTRAL(rndn(SP_SOUL))); break;
		case JK_VAGABOND:     if (g.money <= 4) cons_add(CONS_TAROT(rndn(TAROT_COUNT))); break;
	}
}

// ---------------------------------------------------------------------------
// before-scoring joker phase (context.before): counters, level ups
// ---------------------------------------------------------------------------
void joker_before(SC* s, u8 ji, u16 ct, u8 type, u8 nPlayed, Card* pc, u8 mask) BANKED
{
	JokerInst* j = &g.jk[ji];
	u8 src = SRCJ(ji);
	switch (j->id)
	{
		case JK_SPACE:
			if (rnd_odds(4)) { g.handLevel[type]++; text(s, src, TX_LEVELUP); }
			break;
		case JK_SQUARE: if (nPlayed == 4) { j->v += 4; text(s, src, TX_UPGRADE); } break;
		case JK_RUNNER: if (CONTAINS(HAND_STRAIGHT)) { j->v += 15; text(s, src, TX_UPGRADE); } break;
		case JK_TODO_LIST:
			if (type == (u8)j->v) add_money(s, src, 4);
			break;
		case JK_RIDE_THE_BUS:
		{
			bool face = FALSE;
			for (u8 k = 0; k < nPlayed; k++) if ((mask & (1 << k)) && card_is_face(pc[k])) face = TRUE;
			if (face) { if (j->v > 0) text(s, src, TX_RESET); j->v = 0; }
			else j->v += 1;
			break;
		}
		case JK_GREEN_JOKER: j->v += 1; break;
		case JK_TROUSERS: if (type == HAND_TWO_PAIR) { j->v += 2; text(s, src, TX_UPGRADE); } break;
		case JK_OBELISK:
		{
			bool top = TRUE;
			for (u8 h = 0; h < HAND_COUNT; h++) if (h != type && g.handPlays[h] >= g.handPlays[type]) top = FALSE;
			if (top) { if (j->v > 100) text(s, src, TX_RESET); j->v = 100; } else j->v += 20;
			break;
		}
		case JK_MIDAS_MASK:
			for (u8 k = 0; k < nPlayed; k++)
				if ((mask & (1 << k)) && card_is_face(pc[k]))
				{
					pc[k] = C_SETENH(pc[k], ENH_GOLD); g.deck[g.played[k]] = pc[k];
				}
			break;
		case JK_VAMPIRE:
			for (u8 k = 0; k < nPlayed; k++)
				if ((mask & (1 << k)) && C_ENH(pc[k]) != ENH_NONE)
				{
					pc[k] = C_SETENH(pc[k], ENH_NONE); g.deck[g.played[k]] = pc[k]; j->v += 10; text(s, src, TX_UPGRADE);
				}
			break;
		case JK_DNA:
			if (g.handsPlayed == 1 && nPlayed == 1) { if (deck_add(pc[0], TRUE)) text(s, src, TX_UPGRADE); }
			break;
		case JK_SIXTH_SENSE:
			if (g.handsPlayed == 1 && nPlayed == 1 && C_RANK(pc[0]) == 4 && C_ENH(pc[0]) != ENH_STONE)       // a 6
			{
				g.dflag[g.played[0]] |= DF_BREAK;                      // destroyed when the hand resolves
				cons_add(CONS_SPECTRAL(rndn(SP_SOUL)));
			}
			break;
	}
}

// after the hand has been scored (context.after)
void joker_after(SC* s, u8 ji) BANKED
{
	JokerInst* j = &g.jk[ji];
	switch (j->id)
	{
		case JK_LOYALTY_CARD: j->aux = (u8)((j->aux + 1) % 6); break;
		case JK_SELZER:
			if (j->v > 0 && --j->v == 0) { text(s, SRC_JOKER(ji), TX_EATEN); j->flags |= 0x80; }
			break;
	}
}

