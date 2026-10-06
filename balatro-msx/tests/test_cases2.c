// Joker regression cases (second file: the Z80 self-test code does not fit one 8 KB mapper segment).
#include "bgame.h"


#include "test_common.h"

#if !defined(JK_PART) || JK_PART == 1
void run_joker_cases_a(void) BANKED
{
	{ fresh(); joker_add(JK_HACK); Card c[] = { CARD(H,3), CARD(S,3) }; EXPECT("joker: Hack retriggers 5s (10+5x4)x2", score_of(c, 2), 60); }
	{ fresh(); joker_add(JK_SOCK_AND_BUSKIN); Card c[] = { CARD(H,RANK_K), CARD(S,RANK_K) }; EXPECT("joker: Sock and Buskin retriggers face cards (10+10x4)x2", score_of(c, 2), 100); }
	{ fresh(); joker_add(JK_MIME); Card c[] = { CARD(H,RANK_A) }; load_hand(c, 1); g.deck[5] = C_SETENH(CARD(D,3), ENH_STEEL); g.hand[g.nHand++] = 5; g.loc[5] = LOC_HAND;
	  ScoreOut o; round_play(1, &o); EXPECT("joker: Mime retriggers held Steel (5+11)x2.25", o.total, 36); }
	{ fresh(); joker_add(JK_BLUEPRINT); joker_add(JK_JOKER); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("joker: Blueprint copies the Joker on its right (20)x(2+4+4)", score_of(c, 2), 200); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_BRAINSTORM); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("joker: Brainstorm copies the leftmost Joker (20)x(2+4+4)", score_of(c, 2), 200); }
	{ fresh(); joker_add(JK_TRIBOULET); Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Triboulet X2 on a King (5+10)x2", score_of(c, 1), 30); }
	{ fresh(); joker_add(JK_THROWBACK); g.skips = 2; Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Throwback X1.5 after 2 skips", score_of(c, 1), 22); }
	{ fresh(); joker_add(JK_STONE); g.deck[1] = C_SETENH(g.deck[1], ENH_STONE); g.deck[2] = C_SETENH(g.deck[2], ENH_STONE); Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Stone Joker +50 chips for 2 stones (5+10+50)x1", score_of(c, 1), 65); }
	{ fresh(); joker_add(JK_STEEL_JOKER); g.deck[1] = C_SETENH(g.deck[1], ENH_STEEL); g.deck[2] = C_SETENH(g.deck[2], ENH_STEEL); Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Steel Joker X1.4 for 2 steel cards", score_of(c, 1), 21); }
	{ fresh(); joker_add(JK_EROSION); g.loc[2] = g.loc[3] = LOC_GONE; Card c[] = { CARD(C,RANK_K) }; EXPECT("joker: Erosion +8 Mult with 2 cards missing (15)x9", score_of(c, 1), 135); }
	{ fresh(); joker_add(JK_OOPS); u8 all = 1; for (u8 i = 0; i < 60; i++) all &= rnd_odds(2); EXPECT("joker: Oops All 6s makes 1-in-2 certain", all, 1); }
	{ fresh(); joker_add(JK_MIDAS_MASK); Card c[] = { CARD(C,RANK_K), CARD(H,RANK_K) }; score_of(c, 2); EXPECT("joker: Midas Mask turns played face cards Gold", C_ENH(g.deck[0]) == ENH_GOLD && C_ENH(g.deck[1]) == ENH_GOLD, 1); }
	{ fresh(); joker_add(JK_LOYALTY_CARD); u8 x6 = 0; for (u8 i = 0; i < 6; i++) { Card c[] = { CARD(C,RANK_K) }; if (score_of(c, 1) == 60) x6 = (u8)(i + 1); } EXPECT("joker: Loyalty Card X4 on every 6th hand", x6, 6); }
	{ fresh(); joker_add(JK_SELZER); Card c[] = { CARD(H,3), CARD(S,3) }; EXPECT("joker: Seltzer retriggers every card (10+5x4)x2", score_of(c, 2), 60); for (u8 i = 0; i < 9; i++) score_of(c, 2); EXPECT("joker: Seltzer is eaten after 10 hands", (g.jk[0].flags & 0x80) != 0 || g.nJk == 0, 1); }
	{ fresh(); joker_add(JK_DNA); Card c[] = { CARD(C,RANK_K) }; load_hand(c, 1); u8 n0 = g.nDeck; g.handsPlayed = 0; ScoreOut o; round_play(1, &o); EXPECT("joker: DNA copies a lone first-hand card into the deck", g.nDeck, n0 + 1); }
	{ fresh(); joker_add(JK_MARBLE); u8 n0 = g.nDeck; blind_start(); u8 st = 0; for (u8 i = 0; i < g.nDeck; i++) st += C_ENH(g.deck[i]) == ENH_STONE;
	  EXPECT("joker: Marble adds a Stone card when the Blind starts", g.nDeck == n0 + 1 && st == 1, 1); }
	{ fresh(); joker_add(JK_RIFF_RAFF); blind_start(); EXPECT("joker: Riff-raff creates 2 Common Jokers", g.nJk, 3); }
	{ fresh(); joker_add(JK_CEREMONIAL); joker_add(JK_JOKER); u8 sv = joker_sell_value(1); blind_start(); EXPECT("joker: Ceremonial Dagger eats the Joker on its right for 2x its sell value", g.nJk == 1 && g.jk[0].v == 2 * sv, 1); }
}
#endif
#if !defined(JK_PART) || JK_PART == 2
void run_joker_cases_b(void) BANKED
{
	{ fresh(); joker_add(JK_MADNESS); joker_add(JK_JOKER); blind_start(); EXPECT("joker: Madness gains X0.5 and destroys another Joker", g.nJk == 1 && g.jk[0].id == JK_MADNESS && g.jk[0].v == 150, 1); }
	{ fresh(); joker_add(JK_HIT_THE_ROAD); blind_start(); g.deck[g.hand[0]] = CARD(H,RANK_J); g.deck[g.hand[1]] = CARD(S,RANK_J); round_discard(3); EXPECT("joker: Hit the Road X0.5 per discarded Jack", g.jk[0].v, 200); }
	{ fresh(); joker_add(JK_MAIL); blind_start(); g.jk[0].aux = RANK_Q; g.deck[g.hand[0]] = CARD(H,RANK_Q); i16 m0 = g.money; round_discard(1); EXPECT("joker: Mail-In Rebate pays $5 for the target rank", g.money, m0 + 5); }
	{ fresh(); joker_add(JK_CASTLE); blind_start(); g.jk[0].aux = SUIT_S; g.deck[g.hand[0]] = CARD(S,4); g.deck[g.hand[1]] = CARD(H,4); round_discard(3); EXPECT("joker: Castle +3 chips per discarded card of the target suit", g.jk[0].v, 3); }
	{ fresh(); joker_add(JK_TRADING); blind_start(); u8 sl = g.hand[0]; i16 m0 = g.money; round_discard(1); EXPECT("joker: Trading Card destroys a lone first discard for $3", g.loc[sl] == LOC_GONE && g.money == m0 + 3, 1); }
	{ fresh(); joker_add(JK_FLASH); g.money = 50; shop_generate(); shop_reroll(); EXPECT("joker: Flash Card +2 Mult per reroll", g.jk[0].v, 2); }
	{ fresh(); joker_add(JK_CHAOS); g.money = 50; shop_generate(); i16 m0 = g.money; shop_reroll(); u8 free1 = g.money == m0; shop_reroll(); EXPECT("joker: Chaos the Clown first reroll is free", free1 && g.money < m0, 1); }
	{ fresh(); joker_add(JK_LUCHADOR); g.blind = BLIND_BOSS; g.boss = BS_CLUB; blind_start(); joker_sell(0); EXPECT("joker: Luchador disables the Boss Blind when sold", g.bossOff, 1); }
	{ fresh(); joker_add(JK_CHICOT); g.blind = BLIND_BOSS; g.boss = BS_WATER; blind_start(); EXPECT("joker: Chicot disables the Boss Blind", g.bossOff == 1 && g.discardsLeft == START_DISCARDS + 1, 1); }
	{ fresh(); joker_add(JK_INVISIBLE); joker_add(JK_JOKER); g.jk[0].v = 2; joker_sell(0); EXPECT("joker: Invisible Joker duplicates a Joker when sold after 2 rounds", g.nJk, 2); }
	{ fresh(); joker_add(JK_HOLOGRAM); deck_add(CARD(H,3), FALSE); deck_add(CARD(H,4), FALSE); EXPECT("joker: Hologram X0.25 per card added", g.jk[0].v, 150); }
	{ fresh(); joker_add(JK_GLASS); card_destroyed(C_SETENH(CARD(H,3), ENH_GLASS)); EXPECT("joker: Glass Joker gains X0.75 per shattered Glass Card", g.jk[0].v, 175); }
	{ fresh(); joker_add(JK_ASTRONOMER); g.shopType[0] = 2; g.shopId[0] = 0; EXPECT("joker: Astronomer makes Planets free", shop_cost(0), 0); }
	{ fresh(); joker_add(JK_RED_CARD); g_packN = 1; pack_skipped(); EXPECT("joker: Red Card +3 Mult per skipped pack", g.jk[0].v, 3); }
	{ fresh(); joker_add(JK_OBELISK); Card c[] = { CARD(C,RANK_K) }; score_of(c, 1); score_of(c, 1); EXPECT("joker: Obelisk resets while playing the most played hand", g.jk[0].v, 100); }
	{ fresh(); joker_add(JK_VAGABOND); g.money = 3; g.cons[0] = g.cons[1] = 0; Card c[] = { CARD(C,RANK_K) }; score_of(c, 1); EXPECT("joker: Vagabond creates a Tarot when poor", g.cons[0] != 0, 1); }
	{ fresh(); g.cons[0] = CONS_SPECTRAL(SP_SOUL); EXPECT("spectral: The Soul creates a Legendary Joker", cons_use(0, 0) && g.nJk == 1 && g_Jokers[g.jk[0].id].rarity == 4, 1); }
}
#endif
#if !defined(JK_PART) || JK_PART == 3
void run_deck_cases(void) BANKED
{
	fresh_with(DK_RED, 0); EXPECT("deck: Red +1 discard", g.discardsBase, START_DISCARDS + 1);
	fresh_with(DK_BLUE, 0); EXPECT("deck: Blue +1 hand", g.handsBase, START_HANDS + 1);
	fresh_with(DK_YELLOW, 0); EXPECT("deck: Yellow +$10", g.money, START_MONEY + 10);
	fresh_with(DK_ABANDONED, 0); { u8 f = 0; for (u8 i = 0; i < g.nDeck; i++) f += C_RANK(g.deck[i]) >= RANK_J && C_RANK(g.deck[i]) <= RANK_K; EXPECT("deck: Abandoned has 40 cards and no faces", g.nDeck == 40 && f == 0, 1); }
	fresh_with(DK_CHECKERED, 0); { u8 h = 0, sp = 0; for (u8 i = 0; i < g.nDeck; i++) { h += C_SUIT(g.deck[i]) == SUIT_H; sp += C_SUIT(g.deck[i]) == SUIT_S; } EXPECT("deck: Checkered is 26 hearts + 26 spades", h == 26 && sp == 26, 1); }
	fresh_with(DK_ZODIAC, 0); EXPECT("deck: Zodiac starts with 3 vouchers", (g.vouchers & VBIT(VC_TAROT_MERCHANT)) && (g.vouchers & VBIT(VC_PLANET_MERCHANT)) && (g.vouchers & VBIT(VC_OVERSTOCK)), 1);
	fresh_with(DK_PAINTED, 0); EXPECT("deck: Painted +2 hand size, 4 Joker slots", g.handSizeBase == START_HAND_SIZE + 2 && joker_slots() == JOKER_BASE - 1, 1);
	fresh_with(DK_GHOST, 0); EXPECT("deck: Ghost starts with Hex", g.cons[0], CONS_SPECTRAL(SP_HEX));
	fresh_with(DK_ERRATIC, 0); { u8 diff = 0; for (u8 i = 0; i < g.nDeck; i++) diff += g.deck[i] != CARD(i / 13, i % 13); EXPECT("deck: Erratic is shuffled", diff > 10, 1); }
	fresh_with(DK_PLASMA, 0); { Card c[] = { CARD(C,RANK_K) }; EXPECT("deck: Plasma doubles the Blind", blind_target(), 600); EXPECT("deck: Plasma balances (5+10, 1) -> 8x8", score_of(c, 1), 64); }
	fresh_with(DK_GREEN, 0); blind_start(); g.score = g.target; g.money = 20; { Cash rows[CASH_MAX]; i16 t; cashout_build(rows, &t); EXPECT("deck: Green pays $2 per hand + $1 per discard, no interest", t, 3 + 8 + 3); }
	fresh_with(DK_RED, 1); EXPECT("stake: Red gives no Small Blind reward", blind_reward(), 0);
	fresh_with(DK_RED, 2); EXPECT("stake: Green scales Ante 2", g.ante = 2, 2); EXPECT("stake: Green Ante 2 small blind is 900", blind_target(), 900);
	fresh_with(DK_RED, 4); EXPECT("stake: Blue has one discard less", g.discardsBase, START_DISCARDS);
	fresh_with(DK_RED, 5); g.ante = 2; EXPECT("stake: Purple Ante 2 small blind is 1000", blind_target(), 1000);
	fresh_with(DK_RED, 3); { u8 e = 0; for (u8 i = 0; i < 60; i++) { rng_seed((u16)(i + 5)); e += (shop_joker_stickers() & JF_ETERNAL) != 0; } EXPECT("stake: Black makes some shop Jokers Eternal", e > 5 && e < 40, 1); }
	fresh_with(DK_RED, 7); joker_add(JK_JOKER); g.jk[0].flags |= JF_RENTAL; { Cash rows[CASH_MAX]; i16 t; blind_start(); g.score = g.target; u8 n = cashout_build(rows, &t); u8 r = 0; for (u8 i = 0; i < n; i++) if (rows[i].kind == 6) r = (u8)(-rows[i].amount); EXPECT("stake: Gold Rental costs $3 per Joker", r, 3); }
	fresh_with(DK_RED, 6); joker_add(JK_JOKER); g.jk[0].flags |= JF_PERISH; for (u8 i = 0; i < 5; i++) round_end_effects(); EXPECT("stake: Orange Perishable Jokers expire after 5 rounds", (g.jk[0].flags & JF_PERISHED) != 0, 1);
	fresh_with(DK_RED, 3); joker_add(JK_GROS_MICHEL); g.jk[0].flags |= JF_ETERNAL | 0x80; round_end_effects(); EXPECT("stake: Eternal Jokers survive destruction", g.nJk, 1);
}
#endif

// Negative edition, slot counts, the Black / Magic / Nebula / Anaglyph decks, Double Tag, Diet Cola, Crystal Ball, Telescope
#if !defined(JK_PART) || JK_PART == 4
void run_negative_cases(void) BANKED
{
	fresh_with(DK_BLACK, 0); EXPECT("deck: Black has +1 Joker slot and -1 hand", joker_slots() == JOKER_BASE + 1 && g.handsBase == START_HANDS - 1, 1);
	fresh_with(DK_MAGIC, 0); EXPECT("deck: Magic starts with Crystal Ball and 2 Fools", (g.vouchers & VBIT(VC_CRYSTAL_BALL)) && g.cons[0] == CONS_TAROT(TR_FOOL) && g.cons[1] == CONS_TAROT(TR_FOOL) && cons_slots() == CONS_BASE + 1, 1);
	fresh_with(DK_NEBULA, 0); EXPECT("deck: Nebula has the Telescope and one consumable slot", (g.vouchers & VBIT(VC_TELESCOPE)) && cons_slots() == CONS_BASE - 1, 1);
	fresh_with(DK_ANAGLYPH, 0); g.blind = BLIND_BOSS; g.state = ROUND_WON; round_end_effects(); EXPECT("deck: Anaglyph gives a Double Tag after a Boss", g.nTags == 1 && g.tags[0] == TG_DOUBLE, 1);
	fresh_with(DK_ANAGLYPH, 0); g.blind = BLIND_BIG; g.state = ROUND_WON; round_end_effects(); EXPECT("deck: Anaglyph gives no tag after a Big Blind", g.nTags, 0);
	fresh(); joker_add(JK_JOKER); g.jk[0].ed = ED_NEG; EXPECT("negative: a Negative Joker adds a Joker slot", joker_slots(), JOKER_BASE + 1);
	fresh(); joker_add(JK_JOKER); joker_add(JK_JOLLY); g.cons[0] = CONS_SPECTRAL(SP_ECTOPLASM); { u8 hs = g.handSizeBase;
	  EXPECT("spectral: Ectoplasm adds Negative to a Joker, -1 hand size", cons_use(0, 0) && joker_slots() == JOKER_BASE + 1 && g.handSizeBase == hs - 1, 1); }
	fresh(); g.cons[0] = CONS_SPECTRAL(SP_ECTOPLASM); EXPECT("spectral: Ectoplasm needs a Joker", cons_use(0, 0), 0);
	fresh(); g.cons[0] = CONS_PLANET(1); g.cons[1] = CONS_PLANET(2); EXPECT("consumables: the slots are full", cons_add(CONS_PLANET(3)), 0);
	EXPECT("negative: a Negative consumable makes its own slot", cons_add_ed(CONS_PLANET(3), TRUE) && cons_slots() == CONS_BASE + 1 && g.consNeg == 4, 1);
	fresh(); cons_add_ed(CONS_PLANET(1), TRUE); cons_add(CONS_PLANET(2)); cons_add(CONS_PLANET(3)); cons_use(0, 0);
	EXPECT("negative: using it moves the others down and drops the slot", g.cons[0] == CONS_PLANET(2) && g.cons[1] == CONS_PLANET(3) && g.consNeg == 0 && cons_slots() == CONS_BASE, 1);
	fresh(); joker_add(JK_PERKEO); g.cons[0] = CONS_PLANET(1); g.cons[1] = CONS_PLANET(2); shop_leave();
	EXPECT("joker: Perkeo copies a consumable as Negative", g.consNeg == 4 && g.cons[2] != 0, 1);
	fresh(); g.tags[0] = TG_DOUBLE; g.nTags = 1; g.tagSmall = 1 + TG_JUGGLE; blind_skip();
	EXPECT("tag: Double Tag copies the next tag", g.nTags == 2 && g.tags[0] == TG_JUGGLE && g.tags[1] == TG_JUGGLE, 1);
	fresh(); g.tags[0] = TG_DOUBLE; g.nTags = 1; g.tagSmall = 1 + TG_DOUBLE; blind_skip();
	EXPECT("tag: a Double Tag is not copied", g.nTags == 2 && g.tags[0] == TG_DOUBLE && g.tags[1] == TG_DOUBLE, 1);
	fresh(); joker_add(JK_DIET_COLA); joker_sell(0); EXPECT("joker: Diet Cola gives a Double Tag when sold", g.nTags == 1 && g.tags[0] == TG_DOUBLE, 1);
	fresh(); g.vouchers |= VBIT(VC_TELESCOPE); g.handPlays[HAND_FLUSH] = 5; g.nDeck = 52; pack_open_free(PACK_KIND(1, 0));
	EXPECT("voucher: Telescope puts the most played hand's Planet first", g_packId[0], HAND_FLUSH);
}
#endif
