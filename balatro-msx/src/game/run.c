// Run / round flow: deck, blinds, discards, round end, cash out.
// Rules are those of Balatro (see docs/PORTING.md for the list of what is not ported).
#include "bgame.h"

Game g;
u8 g_deckSel, g_stakeSel;                  // chosen on the deck screen, applied by run_new()

void deck_new(void) BANKED
{
	u8 n = 0;
	for (u8 s = 0; s < 4; s++)
		for (u8 r = 0; r < 13; r++)
		{
			if (g.deckId == DK_ABANDONED && r >= RANK_J && r <= RANK_K) continue;            // no face cards
			u8 suit = s;
			if (g.deckId == DK_CHECKERED) suit = (s < 2) ? SUIT_H : SUIT_S;                  // 26 Spades + 26 Hearts
			g.deck[n] = (g.deckId == DK_ERRATIC) ? CARD(rndn(4), rndn(13)) : CARD(suit, r);
			g.dflag[n] = 0; n++;
		}
	g.nDeck = n;
}

u8 bosses_for_ante(u8 ante) BANKED
{
	for (u8 pass = 0; pass < 2; pass++)
	{
		u8 ok[BOSS_COUNT], n = 0;
		for (u8 b = 0; b < BOSS_COUNT; b++)
		{
			bool finisher = g_Bosses[b].reward == 8;
			if (g_Bosses[b].minAnte > ante) continue;
			if (finisher != (ante == MAX_ANTE)) continue;
			if (g.bossUsed & (1UL << b)) continue;
			ok[n++] = b;
		}
		if (n) { u8 b = ok[rndn(n)]; g.bossUsed |= 1UL << b; return b; }
		g.bossUsed = 0;
	}
	return 0;
}

void run_new(void) BANKED
{
	{ u8* p = (u8*)&g; for (u16 i = 0; i < sizeof(Game); i++) p[i] = 0; }
	g.ante = 1; g.blind = BLIND_SMALL;
	g.money = START_MONEY;
	for (u8 i = 0; i < HAND_COUNT; i++) g.handLevel[i] = 1;
	g.handsBase = START_HANDS; g.discardsBase = START_DISCARDS; g.handSizeBase = START_HAND_SIZE;
	g.interestSteps = INTEREST_CAP / 5;
	g.rerollBase = START_REROLL;
	g.deckId = g_deckSel; g.stake = g_stakeSel;
	switch (g.deckId)
	{
		case DK_RED:     g.discardsBase++; break;
		case DK_BLUE:    g.handsBase++; break;
		case DK_YELLOW:  g.money += 10; break;
		case DK_GREEN:   g.interestSteps = 0; break;                       // no interest: pays per hand / discard left instead
		case DK_ZODIAC:  g.vouchers |= VBIT(VC_TAROT_MERCHANT) | VBIT(VC_PLANET_MERCHANT) | VBIT(VC_OVERSTOCK); break;
		case DK_PAINTED: g.handSizeBase += 2; break;
		case DK_GHOST:   g.cons[0] = CONS_SPECTRAL(SP_HEX); break;
	}
	if (g.stake >= 4 && g.discardsBase > 0) g.discardsBase--;               // Blue Stake
	g.forced = 0xFF; g.mouthHand = 0xFF; g.mostPlayed = 0xFF; g.lastHandType = 0xFF;
	deck_new();
	g.boss = bosses_for_ante(1);
	tags_new_ante();
	voucher_new_ante();
}

//-----------------------------------------------------------------------------
// blinds
//-----------------------------------------------------------------------------
u32 blind_target(void) BANKED
{
	u32 a = g_AnteAmount[g.stake >= 5 ? 2 : (g.stake >= 2 ? 1 : 0)][(g.ante > END_ANTE ? END_ANTE : g.ante) - 1];   // Green / Purple Stake scale faster
	if (g.deckId == DK_PLASMA) a = mul_sat(a, 2);
	if (g.blind == BLIND_SMALL) return a;
	if (g.blind == BLIND_BIG) return a / 2 * 3;
	return (a & 1) ? mul_sat(a, g_Bosses[g.boss].mult2) / 2 : mul_sat(a / 2, g_Bosses[g.boss].mult2);
}

u8 blind_reward(void) BANKED
{
	if (g.blind == BLIND_SMALL) return g.stake >= 1 ? 0 : 3;       // Red Stake: no reward for the Small Blind
	if (g.blind == BLIND_BIG) return 4;
	return g_Bosses[g.boss].reward;
}

u8 hand_size(void) BANKED
{
	i8 n = (i8)(g.handSizeBase + g.handSizeMod + g.tempHand);
	if (bossActive(BS_MANACLE)) n -= 1;
	if (n < 1) n = 1;
	if (n > HAND_MAX) n = HAND_MAX;
	return (u8)n;
}

//-----------------------------------------------------------------------------
// hand ordering / drawing
//-----------------------------------------------------------------------------
static u8 sort_key(u8 slot, u8 mode)
{
	Card c = g.deck[slot];
	static const u8 suitRank[4] = { 3, 2, 1, 4 };         // H C D S: nominal order of the original
	if (mode == 0) return (u8)(C_RANK(c) * 5 + suitRank[C_SUIT(c)]);
	return (u8)(suitRank[C_SUIT(c)] * 16 + C_RANK(c));
}

void hand_sort(u8 mode) BANKED
{
	g.sortMode = mode;
	for (u8 i = 1; i < g.nHand; i++)                      // insertion sort, descending
	{
		u8 s = g.hand[i], k = sort_key(s, mode);
		i8 j = (i8)i - 1;
		while (j >= 0 && sort_key(g.hand[j], mode) < k) { g.hand[j + 1] = g.hand[j]; j--; }
		g.hand[j + 1] = s;
	}
}

void draw_to_hand(void) BANKED
{
	u8 n;
	if (bossActive(BS_SERPENT) && (g.handsPlayed > 0 || g.discardsUsed > 0)) n = 3;
	else n = (g.nHand < hand_size()) ? (u8)(hand_size() - g.nHand) : 0;
	while (n && g.nPile && g.nHand < HAND_MAX)
	{
		u8 slot = g.pile[--g.nPile];
		g.hand[g.nHand++] = slot;
		g.loc[slot] = LOC_HAND;
		g.dflag[slot] &= (u8)~DF_FD;
		if ((bossActive(BS_HOUSE) && !g.handsPlayed && !g.discardsUsed) ||
			(bossActive(BS_FISH) && g.handsPlayed) ||
			(bossActive(BS_MARK) && card_is_face(g.deck[slot])) ||
			(bossActive(BS_WHEEL) && rndn(7) == 0))
			g.dflag[slot] |= DF_FD;
		n--;
	}
	hand_sort(g.sortMode);
	g.forced = 0xFF;
	if (bossActive(BS_FINAL_BELL) && g.nHand) g.forced = g.hand[rndn(g.nHand)];
}

static void shuffle_pile(void)
{
	g.nPile = 0;
	for (u8 s = 0; s < g.nDeck; s++)
		if (g.loc[s] != LOC_GONE) g.pile[g.nPile++] = s;
	for (u8 i = g.nPile; i > 1; i--)
	{
		u8 j = rndn(i), t = g.pile[i - 1];
		g.pile[i - 1] = g.pile[j]; g.pile[j] = t;
	}
}

//-----------------------------------------------------------------------------
// round start
//-----------------------------------------------------------------------------
void blind_start(void) BANKED
{
	i8 jh, jd;
	tags_round_start();
	g.bossOff = 0;
	joker_blind_select();                                           // Blind selected: Jokers that act now (and Chicot)
	joker_round_bonus(&jh, &jd);
	joker_recalc_modifiers();
	g.target = blind_target();
	g.score = 0;
	g.handsLeft = (u8)(g.handsBase + jh);
	g.discardsLeft = (u8)(g.discardsBase + jd);
	if (joker_has(JK_BURGLAR)) g.discardsLeft = 0;            // lose all discards
	if (g.blind == BLIND_BOSS)
	{
		if (g.boss == BS_WATER && !g.bossOff) g.discardsLeft = 0;
		if (g.boss == BS_NEEDLE && !g.bossOff) g.handsLeft = 1;
	}
	g.handsPlayed = 0; g.discardsUsed = 0;
	if (bossActive(BS_FINAL_ACORN))                                // Amber Acorn: shuffle the Jokers
		for (u8 i = g.nJk; i > 1; i--)
		{
			u8 j = rndn(i); JokerInst t = g.jk[i - 1]; g.jk[i - 1] = g.jk[j]; g.jk[j] = t;
		}
	for (u8 i = 0; i < HAND_COUNT; i++) g.playedCnt[i] = 0;
	g.eyeMask = 0; g.mouthHand = 0xFF; g.crimsonPrep = FALSE;
	for (u8 i = 0; i < g.nJk; i++) { g.jk[i].flags &= (u8)~JF_DEBUFF; if (g.jk[i].flags & JF_PERISHED) g.jk[i].flags |= JF_DEBUFF; }
	// the hand the Ox punishes: most played so far (ties: the more valuable hand wins like the original list order)
	{
		u16 best = 0; g.mostPlayed = HAND_HIGH_CARD;     // the original defaults to High Card when nothing was played yet
		for (u8 t = 0; t < HAND_COUNT; t++)
			if (g.handPlays[t] > best || (g.handPlays[t] == best && best > 0)) { best = g.handPlays[t]; g.mostPlayed = t; }
	}
	for (u8 s = 0; s < g.nDeck; s++) if (g.loc[s] != LOC_GONE) g.loc[s] = LOC_PILE;
	g.nHand = 0; g.nPlayed = 0;
	shuffle_pile();
	g.sortMode = 0;
	draw_to_hand();
	joker_hand_drawn();
	g.state = ROUND_PLAYING;
}

//-----------------------------------------------------------------------------
// playing / discarding
//-----------------------------------------------------------------------------
static u8 popcnt(u16 m) { u8 n = 0; while (m) { n += (u8)(m & 1); m >>= 1; } return n; }

bool round_can_play(u16 sel) BANKED
{
	u8 n = popcnt(sel);
	return n >= 1 && n <= PLAY_MAX && g.handsLeft > 0;
}

static void remove_flagged_jokers(void)
{
	for (u8 i = 0; i < g.nJk; )
	{
		if ((g.jk[i].flags & 0x80) && !(g.jk[i].flags & JF_ETERNAL)) joker_remove(i);
		else { g.jk[i].flags &= (u8)~0x80; i++; }
	}
}

void round_resolve_play(void) BANKED
{
	for (u8 i = 0; i < g.nPlayed; i++)
	{
		u8 sl = g.played[i];
		if (g.dflag[sl] & DF_BREAK) card_destroyed(g.deck[sl]);
		g.loc[sl] = (g.dflag[sl] & DF_BREAK) ? LOC_GONE : LOC_DISCARD;       // glass that shattered leaves the deck
		g.dflag[sl] = (u8)((g.dflag[sl] & ~DF_BREAK) | DF_PILLAR);
	}
	g.nPlayed = 0;
	g.score += g.lastTotal;
	g.lastTotal = 0;
	remove_flagged_jokers();
	if (g.crimsonPrep) { g.crimsonPrep = FALSE; }
	if (bossActive(BS_FINAL_HEART) && g.nJk)                   // debuff one random Joker each hand
	{
		u8 r = rndn(g.nJk);
		for (u8 i = 0; i < g.nJk; i++) { g.jk[i].flags &= (u8)~JF_DEBUFF; if (g.jk[i].flags & JF_PERISHED) g.jk[i].flags |= JF_DEBUFF; }
		g.jk[r].flags |= JF_DEBUFF;
	}
	round_check_end();
	if (g.state == ROUND_PLAYING) draw_to_hand();
}

static void check_bones(void)
{
	for (u8 i = 0; i < g.nJk; i++)
		if (g.jk[i].id == JK_MR_BONES && g.score * 4 >= g.target) { joker_remove(i); g.state = ROUND_WON; return; }
	g.state = ROUND_LOST;
}

void round_check_end(void) BANKED
{
	if (g.score >= g.target) { g.state = ROUND_WON; return; }
	if (g.handsLeft == 0) { check_bones(); return; }
	if (g.nHand == 0 && g.nPile == 0) { check_bones(); return; }
	g.state = ROUND_PLAYING;
}

bool round_discard(u16 sel) BANKED
{
	u8 n = popcnt(sel);
	if (n < 1 || n > 5 || g.discardsLeft == 0) return FALSE;
	u8 faces = 0, nh = 0, nhand[HAND_MAX], dslots[5], nd = 0;
	for (u8 i = 0; i < g.nHand; i++)
	{
		u8 slot = g.hand[i];
		if (sel & (1u << i))
		{
			g.loc[slot] = LOC_DISCARD;
			if (nd < 5) dslots[nd++] = slot;
			joker_on_discard(g.deck[slot]);
			if (card_is_face(g.deck[slot])) faces++;
			if (C_SEAL(g.deck[slot]) == SEAL_PURPLE) cons_add(CONS_TAROT(rndn(TAROT_COUNT)));       // Purple Seal
			for (u8 k = 0; k < g.nJk; k++)               // context.discard, once per card
				if (g.jk[k].id == JK_RAMEN && !(g.jk[k].flags & JF_DEBUFF))
				{
					g.jk[k].v -= 1;
					if (g.jk[k].v <= 100) g.jk[k].flags |= 0x80;     // eaten
				}
		}
		else nhand[nh++] = slot;
	}
	for (u8 i = 0; i < nh; i++) g.hand[i] = nhand[i];
	g.nHand = nh;
	joker_on_discard_hand(dslots, nd);
	for (u8 k = 0; k < g.nJk; k++)
	{
		if (g.jk[k].flags & JF_DEBUFF) continue;
		if (g.jk[k].id == JK_GREEN_JOKER && g.jk[k].v > 0) g.jk[k].v -= 1;
		if (g.jk[k].id == JK_FACELESS && faces >= 3) g.money += 5;
	}
	remove_flagged_jokers();
	g.discardsLeft--; g.discardsUsed++;
	draw_to_hand();
	round_check_end();
	return TRUE;
}

//-----------------------------------------------------------------------------
// round end: joker state changes, then the cash out table
//-----------------------------------------------------------------------------
void round_end_effects(void) BANKED
{
	g.unusedDiscards += g.discardsLeft;
	for (u8 i = 0; i < g.nHand; i++)                                  // Blue Seal: the Planet of the last hand played
		if (C_SEAL(g.deck[g.hand[i]]) == SEAL_BLUE && g.lastHandType < HAND_COUNT) cons_add(CONS_PLANET(g.lastHandType));
	g.tempHand = 0;
	for (u8 i = 0; i < g.nJk; i++)
	{
		JokerInst* j = &g.jk[i];
		switch (j->id)
		{
			case JK_POPCORN:    j->v -= 4; if (j->v <= 0) j->flags |= 0x80; break;
			case JK_EGG:        j->sell += 3; break;
			case JK_GROS_MICHEL: if (rnd_odds(6)) j->flags |= 0x80; break;
			case JK_CAVENDISH:  if (rnd16() % 1000 == 0) j->flags |= 0x80; break;
			case JK_TURTLE_BEAN: j->v -= 1; if (j->v <= 0) j->flags |= 0x80; break;
			case JK_ROCKET:     if (g.blind == BLIND_BOSS) j->v += 2; break;
			case JK_TODO_LIST:
			{
				u8 nv;
				do { nv = rndn(9); } while (nv == (u8)j->v);
				j->v = nv;
				break;
			}
		}
	}
	for (u8 i = 0; i < g.nJk; i++)
		if ((g.jk[i].flags & JF_PERISH) && ++g.jk[i].age >= 5) g.jk[i].flags |= JF_PERISHED;      // Orange Stake
	joker_round_end2();
	remove_flagged_jokers();
	joker_recalc_modifiers();
}

u8 cashout_build(Cash* rows, i16* total) BANKED
{
	u8 n = 0; i16 sum = 0;
	rows[n].kind = 0; rows[n].who = (g.score >= g.target) ? 1 : 0;           // who=0: saved by Mr. Bones, no reward
	rows[n].amount = rows[n].who ? blind_reward() : 0; sum += rows[n].amount; n++;
	if (g.handsLeft > 0) { rows[n].kind = 1; rows[n].amount = (i16)(g.handsLeft * (g.deckId == DK_GREEN ? 2 : 1)); rows[n].who = g.handsLeft; sum += rows[n].amount; n++; }
	if (g.deckId == DK_GREEN && g.discardsLeft > 0) { rows[n].kind = 7; rows[n].amount = g.discardsLeft; rows[n].who = g.discardsLeft; sum += rows[n].amount; n++; }
	for (u8 i = 0; i < g.nJk && n < CASH_MAX - 1; i++)
	{
		i16 a = 0; JokerInst* j = &g.jk[i];
		if (j->flags & JF_DEBUFF) continue;
		switch (j->id)
		{
			case JK_GOLDEN:   a = 4; break;
			case JK_ROCKET:   a = j->v; break;
			case JK_CLOUD_9:
				for (u8 s = 0; s < g.nDeck; s++) if (g.loc[s] != LOC_GONE && C_ID(g.deck[s]) == 9) a++;
				break;
			case JK_SATELLITE:
			{
				u16 m = g.planetsUsed;
				while (m) { a += (i16)(m & 1); m >>= 1; }
				break;
			}
			case JK_DELAYED_GRAT: if (g.discardsUsed == 0 && g.discardsLeft > 0) a = (i16)(2 * g.discardsLeft); break;
		}
		if (a > 0) { rows[n].kind = 2; rows[n].amount = a; rows[n].who = i; sum += a; n++; }
	}
	{
		u8 gold = 0;                                                         // Gold Cards still in hand
		for (u8 i = 0; i < g.nHand; i++) if (C_ENH(g.deck[g.hand[i]]) == ENH_GOLD) gold++;
		if (gold && n < CASH_MAX - 1) { rows[n].kind = 5; rows[n].amount = (i16)(3 * gold); rows[n].who = gold; sum += rows[n].amount; n++; }
	}
	{
		i16 inv = tags_eval_bonus();
		if (inv) { rows[n].kind = 4; rows[n].amount = inv; rows[n].who = 0; sum += inv; n++; }
	}
	{
		u8 rent = 0;                                                       // Gold Stake: Rental Jokers cost $3 each round
		for (u8 i = 0; i < g.nJk; i++) rent += (g.jk[i].flags & JF_RENTAL) != 0;
		if (rent) { rows[n].kind = 6; rows[n].amount = (i16)(-3 * rent); rows[n].who = rent; sum += rows[n].amount; n++; }
	}
	if (g.money >= 5 && g.interestSteps)
	{
		u8 steps = (u8)(g.money / 5);
		if (steps > g.interestSteps) steps = g.interestSteps;
		i16 a = (i16)(steps * (1 + g.interestBonus));
		rows[n].kind = 3; rows[n].amount = a; rows[n].who = steps; sum += a; n++;
	}
	*total = sum;
	return n;
}

void next_blind(void) BANKED
{
	if (g.blind == BLIND_BOSS)
	{
		g.ante++; g.blind = BLIND_SMALL;
		for (u8 s = 0; s < g.nDeck; s++) g.dflag[s] &= (u8)~DF_PILLAR;
		if (g.ante <= END_ANTE) { g.boss = bosses_for_ante(g.ante); tags_new_ante(); voucher_new_ante(); }
	}
	else g.blind++;
}

bool run_won(void) BANKED { return g.ante > MAX_ANTE; }
