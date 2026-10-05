// Balatro MSX - core game state and logic API (platform independent: also built on the host for tests)
#pragma once
#include "gtypes.h"
#include "data_gen.h"
#include "assets_gen.h"

//-----------------------------------------------------------------------------
// Cards: bits 0-3 = rank (0=2 ... 8=10, 9=J, 10=Q, 11=K, 12=A), bits 4-5 = suit (H,C,D,S),
// bits 6-9 = enhancement, 10-11 = edition, 12-14 = seal (a Card value carries its modifiers, so copies keep them)
//-----------------------------------------------------------------------------
typedef u16 Card;
enum { ENH_NONE, ENH_BONUS, ENH_MULT, ENH_WILD, ENH_GLASS, ENH_STEEL, ENH_STONE, ENH_GOLD, ENH_LUCKY, ENH_COUNT };
enum { ED_NONE, ED_FOIL, ED_HOLO, ED_POLY };
enum { SEAL_NONE, SEAL_GOLD, SEAL_RED, SEAL_BLUE, SEAL_PURPLE, SEAL_COUNT };
#define SUIT_H 0
#define SUIT_C 1
#define SUIT_D 2
#define SUIT_S 3
#define RANK_2 0
#define RANK_J 9
#define RANK_Q 10
#define RANK_K 11
#define RANK_A 12
#define CARD(s, r)    ((Card)(((s) << 4) | (r)))
#define C_ENH(c)      (((c) >> 6) & 15)
#define C_ED(c)       (((c) >> 10) & 3)
#define C_SEAL(c)     (((c) >> 12) & 7)
#define C_BASE(c)     ((Card)((c) & 0x3F))                // rank + suit only
#define C_SETENH(c,e) ((Card)(((c) & ~(15u << 6)) | ((u16)(e) << 6)))
#define C_SETED(c,e)  ((Card)(((c) & ~(3u << 10)) | ((u16)(e) << 10)))
#define C_SETSEAL(c,e)((Card)(((c) & ~(7u << 12)) | ((u16)(e) << 12)))
#define C_SETRANK(c,r)((Card)(((c) & ~15u) | (r)))
#define C_SETSUIT(c,t)((Card)(((c) & ~(3u << 4)) | ((u16)(t) << 4)))
#define C_RANK(c)     ((c) & 15)
#define C_SUIT(c)     (((c) >> 4) & 3)
#define C_ID(c)       (C_RANK(c) + 2)                     // 2..14, like the original get_id()
#define C_NOMINAL(c)  (C_RANK(c) < 9 ? C_RANK(c) + 2 : (C_RANK(c) == RANK_A ? 11 : 10)) // chips of a rank
#define C_CELL(c)     (CELL_CARD + C_SUIT(c) * 13 + C_RANK(c))

//-----------------------------------------------------------------------------
// Limits / tunables
//-----------------------------------------------------------------------------
#define DECK_MAX         64
#define HAND_MAX         16
#define PLAY_MAX         5
#define JOKER_MAX        5
#define CONS_MAX         2
#define EVENT_MAX        110
#define SHOP_CARD_MAX    5
#define TAG_MAX          6
#define PACK_CARD_MAX    5
#define START_MONEY      4
#define START_HANDS      4
#define START_DISCARDS   3
#define START_HAND_SIZE  8
#define START_REROLL     5
#define INTEREST_CAP     25            // dollars; interest = $1 per $5 up to this
#define MAX_ANTE         8
#define END_ANTE         12                  // endless mode stops here (u32 chips)

//-----------------------------------------------------------------------------
// Poker hand evaluation (poker.c)
//-----------------------------------------------------------------------------
#define PR_FOUR_FINGERS  1
#define PR_SHORTCUT      2
#define PR_SMEARED       4
typedef struct
{
	u8  type;        // HAND_*
	u8  mask;        // scoring cards (bit i = i-th played card)
	u16 contains;    // bit per HAND_* the played hand "contains"
} HandEval;
void poker_eval(const Card* cards, u8 n, u8 rules, HandEval* out);

//-----------------------------------------------------------------------------
// Jokers
//-----------------------------------------------------------------------------
typedef struct
{
	u8  id;          // JK_*
	u8  flags;       // JF_*
	i16 v;           // per-joker counter (chips, mult, x100 mult, hand type...)
	u8  sell;        // sell value bonus (Egg)
	u8  ed;          // ED_* edition (Foil / Holographic / Polychrome)
	
} JokerInst;
#define JF_DEBUFF 1

//-----------------------------------------------------------------------------
// Scoring events (score.c) - computed in one go, replayed by the UI
//-----------------------------------------------------------------------------
enum { EV_BASE, EV_CARD, EV_CHIPS, EV_MULT, EV_XMULT, EV_MONEY, EV_TEXT, EV_DEBUFF };
#define SRC_PLAY(i)  (i)
#define SRC_HELD(i)  (0x20 | (i))
#define SRC_JOKER(i) (0x40 | (i))
#define SRC_NONE     0xFF
typedef struct
{
	u8  kind;
	u8  src;         // SRC_*
	i16 val;         // delta (chips, mult), x100 for EV_XMULT, dollars for EV_MONEY, text id for EV_TEXT
	u16 chips;       // running totals after this event
	u16 mult;
} Ev;
typedef struct
{
	Ev  ev[EVENT_MAX];
	u8  n;
	u8  type;        // HAND_*
	u8  mask;        // scoring cards
	u8  debuffed;    // boss blocked the hand: no score
	u32 total;       // hand score
} ScoreOut;

// Text ids for EV_TEXT popups
enum { TX_AGAIN, TX_UPGRADE, TX_RESET, TX_LEVELUP, TX_DEBUFFED, TX_EATEN, TX_EXTINCT, TX_SAFE, TX_SAVED, TX_COUNT };

//-----------------------------------------------------------------------------
// Consumables: 0 = empty, 1..12 = planet (hand type + 1), 0x20 + n = tarot n
//-----------------------------------------------------------------------------
#define CONS_PLANET(h) ((u8)((h) + 1))
#define CONS_TAROT(t)  ((u8)(0x20 + (t)))
#define CONS_IS_PLANET(c) ((c) >= 1 && (c) <= HAND_COUNT)
#define CONS_IS_TAROT(c)  ((c) >= 0x20)

//-----------------------------------------------------------------------------
// Game state
//-----------------------------------------------------------------------------
enum { BLIND_SMALL, BLIND_BIG, BLIND_BOSS };
enum { ROUND_PLAYING, ROUND_WON, ROUND_LOST };

typedef struct
{
	// ---- run ----
	u8   ante;                       // 1..8
	u8   blind;                      // BLIND_*  (current / next to play)
	u8   boss;                       // BS_* of this ante
	u8   bossFlag;                   // boss defeated in this ante (for gating)
	i16  money;
	u8   handLevel[HAND_COUNT];
	u16  handPlays[HAND_COUNT];      // times played this run
	u8   handsBase, discardsBase;    // per-round resources before jokers/bosses
	i8   handSizeBase;
	u8   interestSteps;              // max $ interest steps (5 => $25 cap)
	u8   interestBonus;              // extra $ per step (To the Moon)
	u16  planetsUsed;                // bitmask of unique planets used (Satellite)
	u8   lastCons;                   // last tarot/planet used (The Fool)
	u8   rerollCost, rerollBase;
	// ---- skip tags ----
	u8   tagSmall, tagBig;           // TG_*+1 offered for skipping the Small / Big blind of this ante (0 none)
	u8   tags[TAG_MAX];              // held tags (TG_*), consumed when they trigger
	u8   nTags;
	u8   skips;                      // blinds skipped this run
	u16  unusedDiscards;             // discards left over at the end of rounds, whole run (Garbage Tag)
	u16  handsPlayedRun;             // hands played this run (Handy Tag)
	i8   tempHand;                   // temporary hand size bonus for the current round (Juggle Tag)
	u8   freeMask;                   // shop slots that cost $0 this visit (Coupon/Uncommon/Rare tags)
	u8   freePacks;                  // packs that cost $0 (Coupon Tag)
	// ---- jokers / consumables ----
	JokerInst jk[JOKER_MAX];
	u8   nJk;
	u8   cons[CONS_MAX];
	// ---- deck ----
	Card deck[DECK_MAX];
	u8   dflag[DECK_MAX];            // DF_*
	u8   nDeck;
	// ---- round ----
	u8   state;                      // ROUND_*
	u32  target, score;
	u8   handsLeft, discardsLeft;
	u8   handsPlayed, discardsUsed;  // this round
	u8   playedCnt[HAND_COUNT];      // times each hand type was played this round
	u16  eyeMask;                    // The Eye: hand types already used
	u32  bossUsed;                   // bosses already seen this run (bit per BS_*)
	u32  lastTotal;                  // score of the hand being resolved
	u8   forced;                     // slot that must stay selected (Cerulean Bell) or 0xFF
	u8   sortMode;                   // 0 rank, 1 suit
	bool crimsonPrep;                // Crimson Heart armed
	u8   loc[DECK_MAX];              // LOC_*
	u8   pile[DECK_MAX];             // draw pile (slot indexes), pile[nPile-1] is drawn first
	u8   nPile;
	u8   hand[HAND_MAX];             // slots currently in hand, in display order
	u8   nHand;
	u8   played[PLAY_MAX];           // slots just played (resolved after scoring)
	u8   nPlayed;
	i8   handSizeMod;                // current additional hand size (jokers/boss)
	u8   mouthHand, mostPlayed;      // boss helpers (0xFF = none)
	u8   bossOff;                    // boss disabled
	u8   endless;                    // continued past the Ante 8 win
	u8   lastHandType;               // HAND_* of the last hand played this run (Blue Seal), 0xFF = none
	// ---- shop ----
	u8   shopType[SHOP_CARD_MAX];    // 0 empty, 1 joker, 2 planet, 3 tarot
	u8   shopId[SHOP_CARD_MAX];
	u8   shopN;
	u8   packType[2];                // 0 none else pack kind
	u8   voucher;                    // 0 none else VC_*+1: the voucher offered this Ante
	u16  vouchers;                   // redeemed vouchers (bit per VC_*)
	u8   shopOpen;
	u8   flags;
} Game;
enum { LOC_PILE, LOC_HAND, LOC_PLAY, LOC_DISCARD, LOC_GONE };
#define VBIT(v) ((u16)(1u << (v)))
#define DF_BREAK  4                  // glass card that shattered this hand
#define DF_FD     2                  // drawn face down (House/Wheel/Fish/Mark)
#define DF_PILLAR 1                  // played this ante (The Pillar)
extern Game g;

//-----------------------------------------------------------------------------
// rng.c
//-----------------------------------------------------------------------------
void rng_seed(u16 s);
u8   rnd8(void);
u16  rnd16(void);
u8   rndn(u8 n);                     // 0..n-1
bool rnd_odds(u8 n);                 // true with probability 1/n

//-----------------------------------------------------------------------------
// run.c - the run / round / shop rules
//-----------------------------------------------------------------------------
void run_new(void) BANKED;
u8   poker_rules(void) BANKED;
bool card_is_face(Card c) BANKED;
bool card_is_debuffed(u8 slot) BANKED;
bool bossActive(u8 b) BANKED;
void joker_round_bonus(i8* hands, i8* discards) BANKED;
u32  blind_target(void) BANKED;
u8   blind_reward(void) BANKED;
void blind_start(void) BANKED;                       // start playing the current blind (draw, effects)
bool round_can_play(u16 selMask) BANKED;
void round_play(u16 selMask, ScoreOut* out) BANKED;  // play highlighted hand slots
void round_resolve_play(void) BANKED;                // after the UI replayed the events: move cards, draw
bool round_discard(u16 selMask) BANKED;
void round_check_end(void) BANKED;
u8   hand_size(void) BANKED;
void hand_sort(u8 mode) BANKED;                      // 0 rank, 1 suit
void draw_to_hand(void) BANKED;
void round_end_effects(void) BANKED;                 // joker end-of-round state changes
typedef struct { u8 kind; i16 amount; u8 who; } Cash;
#define CASH_MAX 12
u8   cashout_build(Cash* rows, i16* total) BANKED;   // rows: blind, hands, jokers, interest
void next_blind(void) BANKED;                         // advance blind/ante after shop
bool run_won(void) BANKED;

// consumables
bool cons_needs_cards(u8 c, u8* minc, u8* maxc) BANKED;
bool cons_use(u8 slot, u16 selMask) BANKED;
bool cons_add(u8 c) BANKED;
void planet_use(u8 hand) BANKED;

// jokers
bool joker_add(u8 id) BANKED;
void joker_remove(u8 idx) BANKED;
u8   joker_sell_value(u8 idx) BANKED;
u8   joker_count(u8 id) BANKED;
u8   joker_slots(void) BANKED;
void joker_recalc_modifiers(void) BANKED;            // hand size / hands / discards bonuses
bool joker_has(u8 id) BANKED;

// shop
void shop_generate(void) BANKED;
bool shop_buy(u8 i) BANKED;
bool shop_reroll(void) BANKED;
u8   shop_cost(u8 i) BANKED;
bool pack_open(u8 slot) BANKED;
extern u8 g_packN, g_packPick, g_packKind;
extern u8 g_packType[PACK_CARD_MAX], g_packId[PACK_CARD_MAX];
extern Card g_packCard[PACK_CARD_MAX];
bool pack_choose(u8 i) BANKED;
bool voucher_buy(void) BANKED;
u8   voucher_price(void) BANKED;
void voucher_new_ante(void) BANKED;
void joker_sell(u8 idx) BANKED;
void cons_sell(u8 slot) BANKED;
u8   cons_sell_value(u8 c) BANKED;
i16  debt_limit(void) BANKED;
#define PACK_NORMAL 0
#define PACK_JUMBO  1
#define PACK_MEGA   2
// pack kinds: 1..12 = (kind-1)/3: 0 arcana, 1 celestial, 2 buffoon, 3 standard ; (kind-1)%3: size
#define PACK_KIND(t, sz) ((u8)(1 + (t) * 3 + (sz)))
u8   pack_cost(u8 kind) BANKED;

// skip tags (tags.c)
void tags_new_ante(void) BANKED;
bool blind_can_skip(void) BANKED;
void blind_skip(void) BANKED;                 // skip the current Small/Big blind and gain its tag
u8   tags_choice_effects(void) BANKED;        // tags that fire when the next Blind choice appears; returns a free pack kind or 0
i16  tags_eval_bonus(void) BANKED;            // Investment Tag after a Boss (consumes it)
void tags_shop_start(void) BANKED;            // Coupon / Uncommon / Rare / D6 effects when the shop opens
void tags_round_start(void) BANKED;           // Juggle Tag
bool pack_open_free(u8 kind) BANKED;          // open a pack without paying (tags)
u8   pack_price(u8 slot) BANKED;

// misc
void deck_new(void) BANKED;
u8   bosses_for_ante(u8 ante) BANKED;                // picks a boss id for the ante
u32  hand_chips(u8 type) BANKED;
u16  hand_mult(u8 type) BANKED;
extern const char* const g_TextMsg[TX_COUNT];
