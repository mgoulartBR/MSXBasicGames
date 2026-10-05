// Playing a hand: poker evaluation, boss rules and joker effects, ported from
// G.FUNCS.evaluate_play (functions/state_events.lua) and Card:calculate_joker (card.lua).
// The whole hand is computed at once into a list of events (running chips/mult after each);
// the UI replays that list as an animation.
#include "bgame.h"

typedef struct
{
	ScoreOut* o;
	u32 chips;
	u32 mult;                 // mult x 100 (fixed point, so X1.5 / X0.25 steps stay exact)
} SC;

static void push(SC* s, u8 kind, u8 src, i16 val)
{
	if (s->o->n >= EVENT_MAX) return;
	Ev* e = &s->o->ev[s->o->n++];
	u32 m = s->mult / 100;
	e->kind = kind; e->src = src; e->val = val;
	e->chips = s->chips > 65535UL ? 65535 : (u16)s->chips;
	e->mult = m > 65535UL ? 65535 : (u16)m;
}
static void add_chips(SC* s, u8 src, i16 v) { s->chips += v; push(s, EV_CHIPS, src, v); }
static void add_mult(SC* s, u8 src, i16 v)  { s->mult += (u32)v * 100; push(s, EV_MULT, src, v); }
static void x_mult(SC* s, u8 src, i16 x100) { s->mult = s->mult * (u32)x100 / 100; push(s, EV_XMULT, src, x100); }
static void add_money(SC* s, u8 src, i16 v) { g.money += v; push(s, EV_MONEY, src, v); }
static void text(SC* s, u8 src, u8 id)      { push(s, EV_TEXT, src, id); }

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
	return (r >= RANK_J && r <= RANK_K) || joker_has(JK_PAREIDOLIA);
}

static bool suit_is(Card c, u8 s)
{
	if (joker_has(JK_SMEARED)) return (C_SUIT(c) & 1) == (s & 1);
	return C_SUIT(c) == s;
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

#define CONTAINS(h) ((ct >> (h)) & 1)

// ---------------------------------------------------------------------------
// per scoring card effects of the jokers (context.individual, cardarea = play)
// ---------------------------------------------------------------------------
static void joker_on_card(SC* s, u8 ji, Card c, bool firstFace)
{
	JokerInst* j = &g.jk[ji];
	u8 src = SRC_JOKER(ji), id = C_ID(c);
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
		case JK_WEE:             if (id == 2) { j->v += 8; text(s, src, TX_UPGRADE); } break;
	}
}

// ---------------------------------------------------------------------------
// held-in-hand effects (cardarea = hand)
// ---------------------------------------------------------------------------
static void joker_on_held(SC* s, u8 ji, u8 hi, u8 lowestIdx)
{
	JokerInst* j = &g.jk[ji];
	Card c = g.deck[g.hand[hi]];
	u8 src = SRC_JOKER(ji);
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
static void joker_main(SC* s, u8 ji, u16 ct, u8 type, u8 nPlayed, const Card* pc, u8 mask)
{
	JokerInst* j = &g.jk[ji];
	u8 src = SRC_JOKER(ji);
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
		case JK_RAMEN:  if (j->v > 100) x_mult(s, src, j->v); break;
	}
}

// ---------------------------------------------------------------------------
// before-scoring joker phase (context.before): counters, level ups
// ---------------------------------------------------------------------------
static void joker_before(SC* s, u8 ji, u16 ct, u8 type, u8 nPlayed, const Card* pc, u8 mask)
{
	JokerInst* j = &g.jk[ji];
	u8 src = SRC_JOKER(ji);
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
	}
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
			g.loc[slot] = LOC_PLAY;
		}
		else nhand[nh++] = g.hand[i];
	}
	for (u8 i = 0; i < nh; i++) g.hand[i] = nhand[i];
	g.nHand = nh; g.nPlayed = np;
	g.handsLeft--; g.handsPlayed++; g.handsPlayedRun++;

	HandEval he;
	poker_eval(pc, np, poker_rules(), &he);
	u8 type = he.type, mask = he.mask;
	u16 ct = he.contains;
	if (joker_has(JK_SPLASH)) mask = (u8)((1 << np) - 1);
	o->type = type; o->mask = mask;

	g.handPlays[type]++;
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
		s.chips += C_NOMINAL(pc[k]);
		push(&s, EV_CARD, SRC_PLAY(k), C_NOMINAL(pc[k]));
		for (u8 i = 0; i < g.nJk; i++)
			if (!(g.jk[i].flags & JF_DEBUFF)) joker_on_card(&s, i, pc[k], k == firstFace);
	}

	// cards held in hand
	u8 lowest = 0xFF, lowId = 15;
	for (u8 h = 0; h < g.nHand; h++)
	{
		u8 id = C_ID(g.deck[g.hand[h]]);
		if (lowId >= id) { lowId = id; lowest = h; }
	}
	for (u8 h = 0; h < g.nHand; h++)
		for (u8 i = 0; i < g.nJk; i++)
			if (!(g.jk[i].flags & JF_DEBUFF)) joker_on_held(&s, i, h, lowest);

	// jokers, left to right
	for (u8 i = 0; i < g.nJk; i++)
		if (!(g.jk[i].flags & JF_DEBUFF)) joker_main(&s, i, ct, type, np, pc, mask);

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
