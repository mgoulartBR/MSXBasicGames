// Shared regression cases: compiled natively for the host (tests/host_main.c) AND for the Z80 (selftest ROM, built with
// SDCC), so code-generation bugs in the target compiler cannot hide behind a passing host run.
#include "bgame.h"

#include "test_common.h"

void run_poker_cases(void)
{
	{ Card h[] = { CARD(H,RANK_A), CARD(S,5), CARD(D,9) }; chk_hand("poker: high card A", h, 3, 0, HAND_HIGH_CARD, 1); }
	{ Card h[] = { CARD(H,RANK_A), CARD(S,RANK_A), CARD(D,9) }; chk_hand("poker: pair of aces", h, 3, 0, HAND_PAIR, 3); }
	{ Card h[] = { CARD(H,3), CARD(S,3), CARD(D,5), CARD(C,5), CARD(H,9) }; chk_hand("poker: two pair", h, 5, 0, HAND_TWO_PAIR, 0x0F); }
	{ Card h[] = { CARD(H,3), CARD(S,3), CARD(D,3), CARD(C,5), CARD(H,9) }; chk_hand("poker: trips", h, 5, 0, HAND_THREE, 7); }
	{ Card h[] = { CARD(H,3), CARD(S,4), CARD(D,5), CARD(C,6), CARD(H,7) }; chk_hand("poker: straight 5-9", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,RANK_A), CARD(S,0), CARD(D,1), CARD(C,2), CARD(H,3) }; chk_hand("poker: wheel A-5", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,8), CARD(S,9), CARD(D,10), CARD(C,11), CARD(H,12) }; chk_hand("poker: broadway", h, 5, 0, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,10), CARD(S,11), CARD(D,12), CARD(C,0), CARD(H,1) }; chk_hand("poker: no wrap Q K A 2 3", h, 5, 0, HAND_HIGH_CARD, 4); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), CARD(H,10) }; chk_hand("poker: flush", h, 5, 0, HAND_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,3), CARD(H,4), CARD(H,5), CARD(H,6) }; chk_hand("poker: straight flush", h, 5, 0, HAND_STRAIGHT_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,2), CARD(H,6) }; chk_hand("poker: quads", h, 5, 0, HAND_FOUR, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,6), CARD(H,6) }; chk_hand("poker: full house", h, 5, 0, HAND_FULL_HOUSE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,2), CARD(D,2), CARD(C,2), CARD(H,2) }; chk_hand("poker: five of a kind", h, 5, 0, HAND_FIVE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,2) }; chk_hand("poker: flush five", h, 5, 0, HAND_FLUSH_FIVE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,2), CARD(H,2), CARD(H,6), CARD(H,6) }; chk_hand("poker: flush house", h, 5, 0, HAND_FLUSH_HOUSE, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), CARD(S,10) }; chk_hand("poker: four fingers flush", h, 5, PR_FOUR_FINGERS, HAND_FLUSH, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,3), CARD(D,4), CARD(C,5), CARD(H,10) }; chk_hand("poker: four fingers straight", h, 5, PR_FOUR_FINGERS, HAND_STRAIGHT, 0x0F); }
	{ Card h[] = { CARD(H,2), CARD(S,4), CARD(D,6), CARD(C,8), CARD(H,10) }; chk_hand("poker: shortcut straight", h, 5, PR_SHORTCUT, HAND_STRAIGHT, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(S,4), CARD(D,6), CARD(C,8), CARD(H,10) }; chk_hand("poker: no shortcut", h, 5, 0, HAND_HIGH_CARD, 0x10); }
	{ Card h[] = { CARD(H,2), CARD(D,4), CARD(H,6), CARD(D,8), CARD(H,10) }; chk_hand("poker: smeared flush", h, 5, PR_SMEARED, HAND_FLUSH, 0x1F); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8) }; chk_hand("poker: 4 hearts is no flush", h, 4, 0, HAND_HIGH_CARD, 0x08); }
}

void run_score_cases(void)
{
	{ fresh(); Card c[] = { CARD(H,RANK_A), CARD(S,RANK_A) }; EXPECT("score: pair of aces (10+11+11)x2", score_of(c, 2), 64); }
	{ fresh(); Card c[] = { CARD(H,RANK_K) }; EXPECT("score: high card K (5+10)x1", score_of(c, 1), 15); }
	{ fresh(); Card c[] = { CARD(H,0), CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8) }; EXPECT("score: flush (35+30)x4", score_of(c, 5), 260); }
	{ fresh(); joker_add(JK_JOKER); Card c[] = { CARD(H,RANK_A), CARD(S,RANK_A) }; EXPECT("score: pair aces + Joker 32x6", score_of(c, 2), 192); }
	{ fresh(); joker_add(JK_JOLLY); joker_add(JK_GREEDY_JOKER); Card c[] = { CARD(D,8), CARD(S,8) }; EXPECT("score: Jolly + Greedy 30x13", score_of(c, 2), 390); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_CAVENDISH); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("score: Joker then Cavendish 20x18", score_of(c, 2), 360); }
	{ fresh(); joker_add(JK_PHOTOGRAPH); Card c[] = { CARD(C,RANK_K), CARD(H,RANK_K) }; EXPECT("score: Photograph 30x4", score_of(c, 2), 120); }
	{ fresh(); joker_add(JK_CAVENDISH); joker_add(JK_JOKER); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("score: order Cavendish then Joker 20x10", score_of(c, 2), 200); }
	{ fresh(); planet_use(HAND_PAIR); Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("score: pair level 2 (25+10)x3", score_of(c, 2), 105); }
	{ fresh(); joker_add(JK_SCHOLAR); Card c[] = { CARD(C,RANK_A), CARD(H,RANK_A) }; EXPECT("score: Scholar on two aces (10+22+40)x(2+8)", score_of(c, 2), 720); }
	{ fresh(); joker_add(JK_HALF); Card c[] = { CARD(C,RANK_Q) }; EXPECT("score: Half Joker high card (5+10)x21", score_of(c, 1), 315); }
	{ fresh(); joker_add(JK_BULL); g.money = 10; Card c[] = { CARD(C,RANK_Q) }; EXPECT("score: Bull $10 (5+10+20)x1", score_of(c, 1), 35); }
	{ fresh(); g.blind = BLIND_SMALL; EXPECT("blind: ante 1 small target", blind_target(), 300);
	  g.blind = BLIND_BIG; EXPECT("blind: ante 1 big target", blind_target(), 450);
	  g.ante = 8; g.blind = BLIND_SMALL; EXPECT("blind: ante 8 small target", blind_target(), 50000);
	  g.ante = 9; EXPECT("blind: endless ante 9 small target", blind_target() > 50000, 1); }
	{ fresh(); blind_start(); g.score = g.target; g.money = 23; Cash rows[CASH_MAX]; i16 t; cashout_build(rows, &t);
	  EXPECT("cashout: $3 blind + 4 hands + $4 interest (23/5)", t, 3 + 4 + 4); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_BONUS), CARD(S,RANK_A) }; EXPECT("card: Bonus +30 chips (10+11+11+30)x2", score_of(c, 2), 124); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_MULT), CARD(S,RANK_A) }; EXPECT("card: Mult +4 (32)x6", score_of(c, 2), 192); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_GLASS), CARD(S,RANK_A) }; EXPECT("card: Glass x2 (32)x4", score_of(c, 2), 128); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_STONE) }; EXPECT("card: Stone alone: (5+0+50)x1", score_of(c, 1), 55); }
	{ fresh(); Card c[] = { C_SETENH(CARD(H,RANK_A), ENH_STONE), CARD(S,RANK_A), CARD(D,RANK_A) }; EXPECT("card: Stone scores beside a pair (10+11+11+50)x2", score_of(c, 3), 164); }
	{ fresh(); Card c[] = { C_SETED(CARD(H,RANK_A), ED_FOIL), CARD(S,RANK_A) }; EXPECT("card: Foil +50 chips (82)x2", score_of(c, 2), 164); }
	{ fresh(); Card c[] = { C_SETED(CARD(H,RANK_A), ED_HOLO), CARD(S,RANK_A) }; EXPECT("card: Holo +10 mult (32)x12", score_of(c, 2), 384); }
	{ fresh(); Card c[] = { C_SETED(CARD(H,RANK_A), ED_POLY), CARD(S,RANK_A) }; EXPECT("card: Polychrome x1.5 (32)x3", score_of(c, 2), 96); }
	{ fresh(); Card c[] = { C_SETSEAL(CARD(H,RANK_A), SEAL_RED), CARD(S,RANK_A) }; EXPECT("card: Red Seal retriggers (10+11+11+11)x2", score_of(c, 2), 86); }
	{ fresh(); g.money = 0; Card c[] = { C_SETSEAL(CARD(H,RANK_A), SEAL_GOLD), CARD(S,RANK_A) }; score_of(c, 2); EXPECT("card: Gold Seal pays $3", g.money, 3); }
	{ Card h[] = { CARD(H,2), CARD(H,4), CARD(H,6), CARD(H,8), C_SETENH(CARD(S,10), ENH_WILD) }; chk_hand("poker: Wild card completes a flush", h, 5, 0, HAND_FLUSH, 0x1F); }
	{ fresh(); Card c[] = { CARD(H,RANK_A), CARD(S,RANK_A) }; load_hand(c, 2); g.deck[g.hand[0]] = CARD(H,RANK_A); g.nHand = 1;
	  g.deck[5] = C_SETENH(CARD(D,3), ENH_STEEL); g.hand[g.nHand++] = 5; g.loc[5] = LOC_HAND; ScoreOut o; round_play(1, &o);
	  EXPECT("card: Steel held x1.5 on high card (5+11)x1.5", o.total, 24); }
	{ fresh(); blind_start(); g.cons[0] = CONS_TAROT(TR_MAGICIAN); u8 slot0 = g.hand[0], slot1 = g.hand[1];
	  EXPECT("tarot: Magician applies Lucky to 2 cards", cons_use(0, 3) && C_ENH(g.deck[slot0]) == ENH_LUCKY && C_ENH(g.deck[slot1]) == ENH_LUCKY, 1);
	  g.cons[0] = CONS_TAROT(TR_TOWER); EXPECT("tarot: Tower needs exactly 1 card", cons_use(0, 3), 0); }
	{ fresh(); blind_start(); g.cons[0] = CONS_TAROT(TR_STRENGTH); u8 sl = g.hand[0]; g.deck[sl] = C_SETENH(CARD(H,3), ENH_GOLD);
	  cons_use(0, 1); EXPECT("tarot: Strength keeps the enhancement", C_ENH(g.deck[sl]) == ENH_GOLD && C_RANK(g.deck[sl]) == 4, 1); }
	{ fresh(); g.nDeck = 52; pack_open_free(PACK_KIND(3, 0)); u8 n0 = g.nDeck;
	  EXPECT("pack: Standard pack adds the chosen card to the deck", pack_choose(0) && g.nDeck == n0 + 1, 1); }
	{ fresh(); blind_start(); g.score = g.target; for (u8 i = 0; i < 2; i++) g.deck[g.hand[i]] = C_SETENH(g.deck[g.hand[i]], ENH_GOLD);
	  Cash rows[CASH_MAX]; i16 t; u8 n = cashout_build(rows, &t); u8 found = 0; for (u8 i = 0; i < n; i++) if (rows[i].kind == 5) found = (u8)rows[i].amount;
	  EXPECT("cashout: Gold Cards held pay $3 each", found, 6); }
	{ fresh(); joker_add(JK_JOKER); g.jk[0].ed = ED_POLY; Card c[] = { CARD(C,3), CARD(H,3) }; EXPECT("joker edition: Polychrome (20)x(2+4)x1.5", score_of(c, 2), 180); }
	{ fresh(); blind_start(); g.nHand = 1; g.hand[0] = 0; g.deck[0] = C_SETSEAL(CARD(H,4), SEAL_BLUE); g.loc[0] = LOC_HAND; g.lastHandType = HAND_PAIR; g.nHand = 1;
	  g.cons[0] = g.cons[1] = 0; round_end_effects(); EXPECT("seal: Blue Seal held creates the Planet of the last hand", g.cons[0], CONS_PLANET(HAND_PAIR)); }
	{ fresh(); blind_start(); u8 d0 = g.nHand; g.cons[0] = CONS_SPECTRAL(SP_FAMILIAR); u8 face = 0;
	  EXPECT("spectral: Familiar uses no selection", cons_use(0, 0), 1);
	  for (u8 i = 0; i < g.nHand; i++) face += (C_ENH(g.deck[g.hand[i]]) != 0);
	  EXPECT("spectral: Familiar: hand +2 net, enhanced face cards added", g.nHand == d0 + 2 && face >= 3, 1); }
	{ fresh(); blind_start(); g.cons[0] = CONS_SPECTRAL(SP_TALISMAN); u8 sl = g.hand[2];
	  EXPECT("spectral: Talisman needs one card", cons_use(0, 0), 0);
	  EXPECT("spectral: Talisman gives a Gold Seal", cons_use(0, 4) && C_SEAL(g.deck[sl]) == SEAL_GOLD, 1); }
	{ fresh(); blind_start(); g.cons[0] = CONS_SPECTRAL(SP_SIGIL); cons_use(0, 0); u8 su = C_SUIT(g.deck[g.hand[0]]), same = 1;
	  for (u8 i = 0; i < g.nHand; i++) if (C_SUIT(g.deck[g.hand[i]]) != su) same = 0;
	  EXPECT("spectral: Sigil makes the whole hand one suit", same, 1); }
	{ fresh(); blind_start(); g.cons[0] = CONS_SPECTRAL(SP_IMMOLATE); u8 d0 = g.nHand; i16 m0 = g.money; cons_use(0, 0);
	  EXPECT("spectral: Immolate destroys 5 cards, pays $20", g.nHand == d0 - 5 && g.money == m0 + 20, 1); }
	{ fresh(); g.cons[0] = CONS_SPECTRAL(SP_BLACK_HOLE); cons_use(0, 0); EXPECT("spectral: Black Hole levels every hand", g.handLevel[HAND_PAIR] == 2 && g.handLevel[HAND_FLUSH_FIVE] == 2, 1); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_GREEDY_JOKER); joker_add(JK_JOLLY); g.cons[0] = CONS_SPECTRAL(SP_ANKH);
	  EXPECT("spectral: Ankh keeps one Joker and copies it", cons_use(0, 0) && g.nJk == 2 && g.jk[0].id == g.jk[1].id, 1); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_JOLLY); g.cons[0] = CONS_SPECTRAL(SP_HEX);
	  EXPECT("spectral: Hex makes one Polychrome Joker, destroys the rest", cons_use(0, 0) && g.nJk == 1 && g.jk[0].ed == ED_POLY, 1); }
	{ fresh(); g.money = 40; g.cons[0] = CONS_SPECTRAL(SP_WRAITH); EXPECT("spectral: Wraith creates a Joker and zeroes money", cons_use(0, 0) && g.nJk == 1 && g.money == 0, 1); }
	{ fresh(); g.nDeck = 52; pack_open_free(PACK_KIND(4, 0)); EXPECT("pack: Spectral pack offers 2 cards", g_packN, 2);
	  EXPECT("pack: choosing a Spectral card puts it in a consumable slot", pack_choose(0) && g.cons[0] >= 0x40, 1); }
	{ fresh(); g.blind = BLIND_BOSS; g.boss = BS_HOUSE; blind_start(); u8 fd = 0;
	  for (u8 i = 0; i < g.nHand; i++) fd += (g.dflag[g.hand[i]] & DF_FD) != 0;
	  EXPECT("boss: House draws the first hand face down", fd == g.nHand && fd > 0, 1);
	  ScoreOut o; round_play(1, &o);
	  EXPECT("boss: played cards turn face up", g.dflag[g.played[0]] & DF_FD, 0); }
	{ fresh(); g.blind = BLIND_BOSS; g.boss = BS_MARK; blind_start(); u8 bad = 0;
	  for (u8 i = 0; i < g.nHand; i++) bad += ((g.dflag[g.hand[i]] & DF_FD) != 0) != card_is_face(g.deck[g.hand[i]]);
	  EXPECT("boss: Mark hides exactly the face cards", bad, 0); }
	{ fresh(); joker_add(JK_JOKER); joker_add(JK_GREEDY_JOKER); joker_add(JK_JOLLY); g.blind = BLIND_BOSS; g.boss = BS_FINAL_ACORN; blind_start();
	  u8 sum = 0; for (u8 i = 0; i < g.nJk; i++) sum += g.jk[i].id;
	  EXPECT("boss: Amber Acorn keeps the joker set", g.nJk == 3 ? sum : 0, JK_JOKER + JK_GREEDY_JOKER + JK_JOLLY); }
}

void run_deck_cases(void) BANKED;
void run_joker_cases_a(void) BANKED;
void run_joker_cases_b(void) BANKED;
void run_all_cases(void) BANKED { run_poker_cases(); run_score_cases(); run_joker_cases_a(); run_joker_cases_b(); run_deck_cases(); }
