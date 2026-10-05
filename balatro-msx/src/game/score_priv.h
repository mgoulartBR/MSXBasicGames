// Shared by score.c (seg 24) and score_jk.c (seg 22): event helpers of the scoring engine (static: one tiny copy per segment)
#pragma once
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

static bool suit_is(Card c, u8 s)
{
	if (C_ENH(c) == ENH_WILD) return TRUE;
	if (C_ENH(c) == ENH_STONE) return FALSE;
	if (joker_has(JK_SMEARED)) return (C_SUIT(c) & 1) == (s & 1);
	return C_SUIT(c) == s;
}

#define CONTAINS(h) ((ct >> (h)) & 1)

void joker_on_card(SC* s, u8 ji, Card c, bool firstFace) BANKED;
void joker_on_held(SC* s, u8 ji, u8 hi, u8 lowestIdx) BANKED;
void joker_main(SC* s, u8 ji, u16 ct, u8 type, u8 nPlayed, const Card* pc, u8 mask) BANKED;
void joker_before(SC* s, u8 ji, u16 ct, u8 type, u8 nPlayed, const Card* pc, u8 mask) BANKED;
