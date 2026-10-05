// Shop, booster packs and consumables.
#include "bgame.h"

u8 g_packN, g_packPick, g_packKind;
u8 g_packType[PACK_CARD_MAX], g_packId[PACK_CARD_MAX];
Card g_packCard[PACK_CARD_MAX];                 // Standard pack: the playing cards on offer

enum { ST_NONE, ST_JOKER, ST_PLANET, ST_TAROT, ST_CARD };

i16 debt_limit(void) BANKED { return joker_has(JK_CREDIT_CARD) ? -20 : 0; }

static u8 popcnt16(u16 m) { u8 n = 0; while (m) { n += (u8)(m & 1); m >>= 1; } return n; }

//-----------------------------------------------------------------------------
// random generators
//-----------------------------------------------------------------------------
static i8 random_joker(u8 avoidMask[(JOKER_COUNT + 7) / 8])
{
	u8 roll = rnd8();
	u8 rarity = roll > 242 ? 3 : (roll > 178 ? 2 : 1);
	for (u8 pass = 0; pass < 2; pass++)
	{
		u8 cand[JOKER_COUNT], n = 0;
		for (u8 i = 0; i < JOKER_COUNT; i++)
		{
			if (pass == 0 && g_Jokers[i].rarity != rarity) continue;
			if (joker_has(i) || (avoidMask[i >> 3] & (1 << (i & 7)))) continue;
			cand[n++] = i;
		}
		if (n) return (i8)cand[rndn(n)];
	}
	return -1;
}

static u8 random_planet(void)
{
	u8 ok[HAND_COUNT], n = 0;
	for (u8 h = 0; h < HAND_COUNT; h++)
		if (h < HAND_FIVE || g.handPlays[h] > 0) ok[n++] = h;      // secret hands only once played
	return ok[rndn(n)];
}

static u8 random_tarot(void) { return rndn(TAROT_COUNT); }

//-----------------------------------------------------------------------------
// consumables
//-----------------------------------------------------------------------------
bool cons_add(u8 c) BANKED
{
	for (u8 i = 0; i < CONS_MAX; i++) if (g.cons[i] == 0) { g.cons[i] = c; return TRUE; }
	return FALSE;
}

u8 cons_sell_value(u8 c) BANKED { (void)c; return 1; }

void cons_sell(u8 slot) BANKED
{
	if (g.cons[slot]) { g.money += cons_sell_value(g.cons[slot]); g.cons[slot] = 0; }
}

void joker_sell(u8 idx) BANKED
{
	g.money += joker_sell_value(idx);
	joker_remove(idx);
	if (g.blind == BLIND_BOSS && g.boss == BS_FINAL_LEAF) g.bossOff = 1;
}

void planet_use(u8 hand) BANKED
{
	g.handLevel[hand]++;
	g.planetsUsed |= (u16)(1u << hand);
	for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].id == JK_CONSTELLATION) g.jk[i].v += 10;
}

bool cons_needs_cards(u8 c, u8* minc, u8* maxc) BANKED
{
	*minc = 0; *maxc = 0;
	if (!CONS_IS_TAROT(c)) return FALSE;
	switch (c - 0x20)
	{
		case TR_STRENGTH:   *minc = 1; *maxc = 2; return TRUE;
		case TR_HANGED_MAN: *minc = 1; *maxc = 2; return TRUE;
		case TR_DEATH:      *minc = 2; *maxc = 2; return TRUE;
		case TR_STAR: case TR_MOON: case TR_SUN: case TR_WORLD: *minc = 1; *maxc = 3; return TRUE;
		case TR_MAGICIAN: case TR_EMPRESS: case TR_HEIROPHANT: *minc = 1; *maxc = 2; return TRUE;
		case TR_LOVERS: case TR_CHARIOT: case TR_JUSTICE: case TR_DEVIL: case TR_TOWER: *minc = 1; *maxc = 1; return TRUE;
	}
	return FALSE;
}

static void hand_remove(u8 idx)
{
	for (u8 i = idx; i + 1 < g.nHand; i++) g.hand[i] = g.hand[i + 1];
	g.nHand--;
}

// use the consumable in slot (planets need no cards); sel = highlighted hand positions
bool cons_use(u8 slot, u16 sel) BANKED
{
	u8 c = g.cons[slot];
	if (!c) return FALSE;
	u8 minc, maxc;
	bool needs = cons_needs_cards(c, &minc, &maxc);
	u8 n = popcnt16(sel);
	if (needs && (n < minc || n > maxc || g.nHand == 0)) return FALSE;

	if (CONS_IS_PLANET(c))
	{
		planet_use(c - 1);
		g.cons[slot] = 0;
		g.lastCons = c;
		return TRUE;
	}
	u8 t = c - 0x20;
	if (t == TR_JUDGEMENT && g.nJk >= joker_slots()) return FALSE;
	if (t == TR_FOOL && (g.lastCons == 0 || g.lastCons == c)) return FALSE;
	g.cons[slot] = 0;                         // the card is spent first: it frees its slot for the ones it creates
	switch (t)
	{
		case TR_FOOL: cons_add(g.lastCons); break;
		case TR_HIGH_PRIESTESS: for (u8 k = 0; k < 2; k++) cons_add(CONS_PLANET(random_planet())); break;
		case TR_EMPEROR: for (u8 k = 0; k < 2; k++) cons_add(CONS_TAROT(random_tarot())); break;
		case TR_HERMIT: if (g.money > 0) g.money += (g.money > 20 ? 20 : g.money); break;
		case TR_TEMPERANCE:
		{
			i16 sum = 0;
			for (u8 i = 0; i < g.nJk; i++) sum += joker_sell_value(i);
			g.money += (sum > 50 ? 50 : sum);
			break;
		}
		case TR_JUDGEMENT:
		{
			u8 none[(JOKER_COUNT + 7) / 8] = { 0 };
			i8 j = random_joker(none);
			if (j >= 0) joker_add((u8)j);
			break;
		}
		case TR_STRENGTH:
			for (u8 i = 0; i < g.nHand; i++)
				if (sel & (1u << i))
				{
					Card* d = &g.deck[g.hand[i]];
					*d = C_SETRANK(*d, (C_RANK(*d) + 1) % 13);
				}
			break;
		case TR_HANGED_MAN:
			for (i8 i = (i8)g.nHand - 1; i >= 0; i--)
				if (sel & (1u << i)) { g.loc[g.hand[i]] = LOC_GONE; hand_remove((u8)i); }
			break;
		case TR_DEATH:
		{
			u8 a = 0xFF, b = 0xFF;
			for (u8 i = 0; i < g.nHand; i++) if (sel & (1u << i)) { if (a == 0xFF) a = i; else b = i; }
			g.deck[g.hand[a]] = g.deck[g.hand[b]];            // the left card becomes a copy of the right one
			break;
		}
		case TR_MAGICIAN: case TR_EMPRESS: case TR_HEIROPHANT: case TR_LOVERS: case TR_CHARIOT: case TR_JUSTICE: case TR_DEVIL: case TR_TOWER:
		{
			u8 e = (t == TR_MAGICIAN) ? ENH_LUCKY : (t == TR_EMPRESS) ? ENH_MULT : (t == TR_HEIROPHANT) ? ENH_BONUS :
			       (t == TR_LOVERS) ? ENH_WILD : (t == TR_CHARIOT) ? ENH_STEEL : (t == TR_JUSTICE) ? ENH_GLASS :
			       (t == TR_DEVIL) ? ENH_GOLD : ENH_STONE;
			for (u8 i = 0; i < g.nHand; i++)
				if (sel & (1u << i)) { Card* d = &g.deck[g.hand[i]]; *d = C_SETENH(*d, e); }
			break;
		}
		case TR_WHEEL_OF_FORTUNE:
		{
			u8 cand[JOKER_MAX], n = 0;
			for (u8 i = 0; i < g.nJk; i++) if (g.jk[i].ed == ED_NONE) cand[n++] = i;
			if (n && rnd_odds(4))
			{
				u8 r = rndn(100);
				g.jk[cand[rndn(n)]].ed = (r < 50) ? ED_FOIL : (r < 85) ? ED_HOLO : ED_POLY;
			}
			break;
		}
		case TR_STAR: case TR_MOON: case TR_SUN: case TR_WORLD:
		{
			u8 suit = (t == TR_STAR) ? SUIT_D : (t == TR_MOON) ? SUIT_C : (t == TR_SUN) ? SUIT_H : SUIT_S;
			for (u8 i = 0; i < g.nHand; i++)
				if (sel & (1u << i)) { Card* d = &g.deck[g.hand[i]]; *d = C_SETSUIT(*d, suit); }
			break;
		}
	}
	g.lastCons = (t == TR_FOOL) ? g.lastCons : c;
	if (needs) hand_sort(g.sortMode);
	return TRUE;
}

//-----------------------------------------------------------------------------
// shop
//-----------------------------------------------------------------------------
static u8 discounted(u8 base)
{
	if (!(g.vouchers & VBIT(VC_CLEARANCE_SALE))) return base;
	u16 v = ((u16)(2 * base + 1) * 3) / 8;                 // floor((cost + 0.5) * 75%), as Card:set_cost
	return v < 1 ? 1 : (u8)v;
}

static void roll_slot(u8 i, u8* avoid)
{
	// card rates of the original: joker 20 : tarot 4 : planet 4 (x2.4 with the merchant vouchers); weights x10
	u16 wt = (g.vouchers & VBIT(VC_TAROT_MERCHANT)) ? 96 : 40, wp = (g.vouchers & VBIT(VC_PLANET_MERCHANT)) ? 96 : 40;
	u16 r = (u16)(rnd16() % (200 + wt + wp));
	g.shopType[i] = 0;
	if (r < 200)
	{
		i8 j = random_joker(avoid);
		if (j >= 0) { g.shopType[i] = ST_JOKER; g.shopId[i] = (u8)j; avoid[j >> 3] |= (u8)(1 << (j & 7)); }
	}
	else if (r < 200 + wt) { g.shopType[i] = ST_TAROT; g.shopId[i] = random_tarot(); }
	else { g.shopType[i] = ST_PLANET; g.shopId[i] = random_planet(); }
	if (!g.shopType[i]) { g.shopType[i] = ST_PLANET; g.shopId[i] = random_planet(); }
}

static void shop_roll_cards(void)
{
	u8 avoid[(JOKER_COUNT + 7) / 8];
	for (u8 i = 0; i < sizeof(avoid); i++) avoid[i] = 0;
	g.shopN = (g.vouchers & VBIT(VC_OVERSTOCK)) ? 3 : 2;
	for (u8 i = 0; i < g.shopN; i++) roll_slot(i, avoid);
}

void voucher_new_ante(void) BANKED
{
	u8 ok[VOUCHER_COUNT], n = 0;
	for (u8 v = 0; v < VOUCHER_COUNT; v++) if (!(g.vouchers & VBIT(v))) ok[n++] = v;
	g.voucher = n ? (u8)(1 + ok[rndn(n)]) : 0;
}

u8 voucher_price(void) BANKED { return discounted(10); }

bool voucher_buy(void) BANKED
{
	if (!g.voucher) return FALSE;
	u8 cost = voucher_price(), v = (u8)(g.voucher - 1);
	if (g.money - cost < debt_limit()) return FALSE;
	g.money -= cost;
	g.vouchers |= VBIT(v);
	g.voucher = 0;
	switch (v)
	{
		case VC_OVERSTOCK:
			if (g.shopN < SHOP_CARD_MAX) { u8 avoid[(JOKER_COUNT + 7) / 8]; for (u8 i = 0; i < sizeof(avoid); i++) avoid[i] = 0; roll_slot(g.shopN, avoid); g.shopN++; }
			break;
		case VC_REROLL_SURPLUS: g.rerollBase = g.rerollBase > 2 ? (u8)(g.rerollBase - 2) : 0; g.rerollCost = g.rerollCost > 2 ? (u8)(g.rerollCost - 2) : 0; break;
		case VC_GRABBER: g.handsBase++; break;
		case VC_WASTEFUL: g.discardsBase++; break;
		case VC_SEED_MONEY: g.interestSteps = 50 / 5; break;
		case VC_PAINT_BRUSH: g.handSizeBase++; break;
	}
	return TRUE;
}

void shop_generate(void) BANKED
{
	g.rerollCost = g.rerollBase;
	g.freeMask = 0; g.freePacks = 0;
	shop_roll_cards();
	for (u8 i = 0; i < 2; i++)
	{
		u8 t = rndn(4), sz = rndn(10) < 6 ? PACK_NORMAL : (rndn(3) ? PACK_JUMBO : PACK_MEGA);
		g.packType[i] = PACK_KIND(t, sz);
	}
	g.shopOpen = 1;
	tags_shop_start();
}

u8 shop_cost(u8 i) BANKED
{
	if (g.freeMask & (1 << i)) return 0;
	switch (g.shopType[i])
	{
		case ST_JOKER: return discounted(g_Jokers[g.shopId[i]].cost);
		case ST_PLANET: case ST_TAROT: return discounted(3);
	}
	return 0;
}

u8 pack_cost(u8 kind) BANKED { u8 sz = (u8)((kind - 1) % 3); return discounted((u8)(4 + 2 * sz)); }
u8 pack_price(u8 slot) BANKED { return (g.freePacks & (1 << slot)) ? 0 : pack_cost(g.packType[slot]); }

bool shop_buy(u8 i) BANKED
{
	u8 t = g.shopType[i];
	if (!t) return FALSE;
	u8 cost = shop_cost(i);
	if (g.money - cost < debt_limit()) return FALSE;
	if (t == ST_JOKER) { if (!joker_add(g.shopId[i])) return FALSE; }
	else if (t == ST_PLANET) { if (!cons_add(CONS_PLANET(g.shopId[i]))) return FALSE; }
	else { if (!cons_add(CONS_TAROT(g.shopId[i]))) return FALSE; }
	g.money -= cost;
	g.shopType[i] = 0;
	return TRUE;
}

bool shop_reroll(void) BANKED
{
	if (g.money - g.rerollCost < debt_limit()) return FALSE;
	g.money -= g.rerollCost;
	g.rerollCost++;
	g.freeMask = 0;                      // rerolled cards are not part of the Coupon deal
	shop_roll_cards();
	return TRUE;
}

//-----------------------------------------------------------------------------
// booster packs: Arcana (tarots), Celestial (planets), Buffoon (jokers), Standard (playing cards)
//-----------------------------------------------------------------------------
static void pack_fill(u8 kind);

bool pack_open(u8 slot) BANKED
{
	u8 kind = g.packType[slot];
	if (!kind) return FALSE;
	u8 cost = pack_price(slot);
	if (g.money - cost < debt_limit()) return FALSE;
	g.money -= cost;
	g.packType[slot] = 0;
	pack_fill(kind);
	return TRUE;
}

bool pack_open_free(u8 kind) BANKED { pack_fill(kind); return TRUE; }

static void pack_fill(u8 kind)
{
	u8 type = (u8)((kind - 1) / 3), sz = (u8)((kind - 1) % 3);
	g_packKind = kind;
	g_packN = (type == 2) ? (sz == 0 ? 2 : 4) : (sz == 0 ? 3 : 5);   // Standard (type 3) like Arcana: 3 / 5 cards
	g_packPick = (sz == 2) ? 2 : 1;
	u8 avoid[(JOKER_COUNT + 7) / 8];
	for (u8 i = 0; i < sizeof(avoid); i++) avoid[i] = 0;
	u16 usedPlanet = 0; u32 usedTarot = 0;
	for (u8 i = 0; i < g_packN; i++)
	{
		if (type == 3)
		{
			Card c = CARD(rndn(4), rndn(13));                          // random playing card, often modified
			if (rndn(10) < 4) c = C_SETENH(c, 1 + rndn(ENH_COUNT - 1));
			if (rndn(100) < 8) c = C_SETED(c, 1 + rndn(3));
			if (rndn(5) == 0) c = C_SETSEAL(c, 1 + rndn(SEAL_COUNT - 1));
			g_packType[i] = ST_CARD; g_packCard[i] = c;
		}
		else if (type == 0)
		{
			u8 t; do { t = random_tarot(); } while (usedTarot & (1UL << t));
			usedTarot |= (1UL << t); g_packType[i] = ST_TAROT; g_packId[i] = t;
		}
		else if (type == 1)
		{
			u8 h; do { h = random_planet(); } while (usedPlanet & (1u << h));
			usedPlanet |= (u16)(1u << h); g_packType[i] = ST_PLANET; g_packId[i] = h;
		}
		else
		{
			i8 j = random_joker(avoid);
			if (j < 0) { g_packType[i] = 0; continue; }
			avoid[j >> 3] |= (u8)(1 << (j & 7));
			g_packType[i] = ST_JOKER; g_packId[i] = (u8)j;
		}
	}
}

bool pack_choose(u8 i) BANKED
{
	if (i >= g_packN || !g_packType[i] || !g_packPick) return FALSE;
	switch (g_packType[i])
	{
		case ST_PLANET: planet_use(g_packId[i]); g.lastCons = CONS_PLANET(g_packId[i]); break;
		case ST_TAROT:  if (!cons_add(CONS_TAROT(g_packId[i]))) return FALSE; break;
		case ST_JOKER:  if (!joker_add(g_packId[i])) return FALSE; break;
		case ST_CARD:
			if (g.nDeck >= DECK_MAX) return FALSE;
			g.deck[g.nDeck] = g_packCard[i]; g.dflag[g.nDeck] = 0; g.loc[g.nDeck] = LOC_PILE; g.nDeck++;
			break;
	}
	g_packType[i] = 0;
	g_packPick--;
	return TRUE;
}
