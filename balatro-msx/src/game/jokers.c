// Joker bookkeeping: acquiring, removing, sell values and passive modifiers.
// Effects that fire while scoring live in score.c; round-end effects in run.c.
#include "bgame.h"

u8 joker_slots(void) BANKED { return JOKER_MAX; }

bool joker_has(u8 id) BANKED
{
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == id && !(g.jk[i].flags & JF_DEBUFF)) return TRUE;
	return FALSE;
}

u8 joker_count(u8 id) BANKED
{
	u8 n = 0;
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == id) n++;
	return n;
}

u8 joker_sell_value(u8 idx) BANKED
{
	u8 v = g_Jokers[g.jk[idx].id].cost >> 1;
	if (v < 1) v = 1;
	return (u8)(v + g.jk[idx].sell);
}

static i8 hand_size_mod(void)
{
	i8 m = 0;
	for (u8 i = 0; i < g.nJk; i++)
	{
		switch (g.jk[i].id)
		{
			case JK_JUGGLER:    m += 1; break;
			case JK_TROUBADOUR: m += 2; break;
			case JK_MERRY_ANDY: m -= 1; break;
			case JK_STUNTMAN:   m -= 2; break;
			case JK_TURTLE_BEAN: m += g.jk[i].v; break;
		}
	}
	return m;
}

// hands / discards bonuses coming from jokers (applied at the start of each round)
void joker_round_bonus(i8* hands, i8* discards) BANKED
{
	*hands = 0; *discards = 0;
	for (u8 i = 0; i < g.nJk; i++)
	{
		switch (g.jk[i].id)
		{
			case JK_BURGLAR:    *hands += 3; break;
			case JK_TROUBADOUR: *hands -= 1; break;
			case JK_DRUNKARD:   *discards += 1; break;
			case JK_MERRY_ANDY: *discards += 3; break;
		}
	}
}

void joker_recalc_modifiers(void) BANKED
{
	g.handSizeMod = hand_size_mod();
	g.interestBonus = 0;
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == JK_TO_THE_MOON) g.interestBonus++;
}

bool joker_add(u8 id) BANKED
{
	if (g.nJk >= joker_slots()) return FALSE;
	JokerInst* j = &g.jk[g.nJk++];
	j->id = id; j->flags = 0; j->v = 0; j->sell = 0; j->ed = ED_NONE; j->aux = 0;
	switch (id)
	{
		case JK_ICE_CREAM:     j->v = 100; break;
		case JK_POPCORN:       j->v = 20; break;
		case JK_RAMEN:         j->v = 200; break;
		case JK_CONSTELLATION: j->v = 100; break;
		case JK_TURTLE_BEAN:   j->v = 5; break;
		case JK_ROCKET:        j->v = 1; break;
		case JK_TODO_LIST:     j->v = rndn(9); break;       // a non-secret hand type
		case JK_HOLOGRAM: case JK_VAMPIRE: case JK_OBELISK: case JK_LUCKY_CAT: case JK_GLASS: case JK_CAINO: case JK_YORICK:
		case JK_MADNESS: case JK_HIT_THE_ROAD: case JK_CAMPFIRE: j->v = 100; break;      // X1.00 to start
		case JK_SELZER:        j->v = 10; break;
		case JK_ANCIENT: case JK_CASTLE: j->aux = rndn(4); break;
		case JK_IDOL:          j->aux = (u8)CARD(rndn(4), rndn(13)); break;
		case JK_MAIL:          j->aux = rndn(13); break;
	}
	joker_recalc_modifiers();
	return TRUE;
}

void joker_remove(u8 idx) BANKED
{
	for (u8 i = idx; i + 1 < g.nJk; i++) g.jk[i] = g.jk[i + 1];
	g.nJk--;
	joker_recalc_modifiers();
}
