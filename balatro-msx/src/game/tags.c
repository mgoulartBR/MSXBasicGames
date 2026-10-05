// Skip tags: what you get for skipping the Small / Big blind (tag.lua of the original, subset: no editions/vouchers/
// standard or spectral packs, so Negative/Foil/Holo/Polychrome/Voucher/Standard/Ethereal/Double tags are not offered).
#include "bgame.h"

static u8 tag_roll(void)
{
	u8 ok[TAG_COUNT], n = 0;
	for (u8 t = 0; t < TAG_COUNT; t++) if (g_Tags[t].minAnte <= g.ante) ok[n++] = t;
	return (u8)(1 + ok[rndn(n)]);
}

void tags_new_ante(void) BANKED
{
	g.tagSmall = tag_roll();
	g.tagBig = tag_roll();
}

bool blind_can_skip(void) BANKED { return g.blind != BLIND_BOSS; }

static u8 random_hand_for_orbital(void)
{
	u8 ok[HAND_COUNT], n = 0;
	for (u8 h = 0; h < HAND_COUNT; h++) if (h < HAND_FIVE || g.handPlays[h] > 0) ok[n++] = h;
	return ok[rndn(n)];
}

static void give_tag(u8 t)
{
	switch (t)                                           // 'immediate' tags act at once
	{
		case TG_HANDY:   g.money += (i16)g.handsPlayedRun; return;
		case TG_GARBAGE: g.money += (i16)g.unusedDiscards; return;
		case TG_SKIP:    g.money += (i16)(5 * g.skips); return;
		case TG_ECONOMY: if (g.money > 0) g.money += (g.money > 40 ? 40 : g.money); return;
		case TG_ORBITAL: g.handLevel[random_hand_for_orbital()] += 3; return;
		case TG_TOP_UP:
			for (u8 k = 0; k < 2 && g.nJk < joker_slots(); k++)
			{
				u8 cand[JOKER_COUNT], n = 0;
				for (u8 j = 0; j < JOKER_COUNT; j++) if (g_Jokers[j].rarity == 1 && !joker_has(j)) cand[n++] = j;
				if (n) joker_add(cand[rndn(n)]);
			}
			return;
	}
	if (g.nTags < TAG_MAX) g.tags[g.nTags++] = t;
}

void blind_skip(void) BANKED
{
	u8 tag = (g.blind == BLIND_SMALL ? g.tagSmall : g.tagBig);
	g.skips++;
	if (tag) give_tag((u8)(tag - 1));
	g.blind++;
}

static bool take_tag(u8 t)
{
	for (u8 i = 0; i < g.nTags; i++)
		if (g.tags[i] == t) { for (u8 k = i; k + 1 < g.nTags; k++) g.tags[k] = g.tags[k + 1]; g.nTags--; return TRUE; }
	return FALSE;
}

u8 tags_choice_effects(void) BANKED
{
	if (take_tag(TG_BOSS)) g.boss = bosses_for_ante(g.ante);            // reroll the Boss Blind
	if (take_tag(TG_CHARM)) return PACK_KIND(0, PACK_MEGA);
	if (take_tag(TG_METEOR)) return PACK_KIND(1, PACK_MEGA);
	if (take_tag(TG_BUFFOON)) return PACK_KIND(2, PACK_MEGA);
	return 0;
}

i16 tags_eval_bonus(void) BANKED
{
	if (g.blind == BLIND_BOSS && take_tag(TG_INVESTMENT)) return 25;
	return 0;
}

void tags_round_start(void) BANKED
{
	g.tempHand = take_tag(TG_JUGGLE) ? 3 : 0;
}

void tags_shop_start(void) BANKED
{
	g.freeMask = 0; g.freePacks = 0;
	if (take_tag(TG_COUPON)) { g.freeMask = 3; g.freePacks = 3; }           // the two initial cards and both packs
	if (take_tag(TG_D_SIX)) g.rerollCost = 0;
	for (u8 pass = 0; pass < 2; pass++)
	{
		u8 t = pass ? TG_RARE : TG_UNCOMMON, rar = pass ? 3 : 2;
		if (g.shopN < SHOP_CARD_MAX && take_tag(t))
		{
			u8 cand[JOKER_COUNT], n = 0;
			for (u8 j = 0; j < JOKER_COUNT; j++) if (g_Jokers[j].rarity == rar && !joker_has(j)) cand[n++] = j;
			if (n)
			{
				g.shopType[g.shopN] = 1; g.shopId[g.shopN] = cand[rndn(n)];
				g.freeMask |= (u8)(1 << g.shopN); g.shopN++;
			}
		}
	}
}
