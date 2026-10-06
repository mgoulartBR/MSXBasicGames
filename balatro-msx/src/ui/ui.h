// Balatro MSX - user interface (screens, widgets, HUD). Everything is banked code (bank 2).
#pragma once
// SDCC folds expressions such as (INFO_Y + 3) in signed-char range and reports a false warning 158 (the
// generated value is correct: verified in the ROM screenshots), so it is disabled for the UI files.
#pragma disable_warning 158
#include "gtypes.h"
#include "bgame.h"
#include "platform/pvideo.h"
#include "platform/ctrl.h"
#include "platform/audio.h"

//-----------------------------------------------------------------------------
// layout
//-----------------------------------------------------------------------------
#define AREA_X        64
#define AREA_W        192
#define JOKER_Y       2
#define PLAY_Y        54
#define HAND_Y        120
#define HAND_RAISE    10
#define BTN_Y         158
#define BTN_H         14
#define INFO_Y        176
// The Joker and consumable rows grow with the slot count (Black Deck, Negative, Crystal Ball, Nebula Deck): consumables are
// right-aligned, Jokers fill the rest of the row and overlap (like the hand) once 26 px per card no longer fits.
u8 jslots(void) BANKED;          // slots drawn: max(Joker slots, Jokers owned)
u8 cslots(void) BANKED;          // consumable slots drawn
u8 jpitch(void) BANKED;          // x distance between Joker slots (<= 26; below 24 the cards overlap)
u8 jwidth(u8 i) BANKED;          // width of the clickable strip of Joker i (the last one is the whole card)
u8 jx(u8 i) BANKED;
u8 cx(u8 i) BANKED;
#define JOKER_X(i)    jx(i)
#define CONS_X(i)     cx(i)

//-----------------------------------------------------------------------------
// widgets
//-----------------------------------------------------------------------------
typedef struct { u8 x, y, w, h, id; } Widget;
#define WMAX 40
enum
{
	W_HAND = 0,          // 0..15
	W_JOKER = 16,        // 16..23 (JOKER_MAX)
	W_CONS = 24,         // 24..27 (CONS_MAX)
	W_PLAY = 28, W_DISCARD, W_SORT_RANK, W_SORT_SUIT, W_INFO,    // 28..32
	W_SELL = 33, W_USE,                                          // 33, 34
	W_SHOPCARD = 35,     // 35..39 (SHOP_CARD_MAX)
	W_PACK = 40,         // 40..41
	W_REROLL = 42, W_NEXT, W_OK, W_SKIP,                         // 42..45
	W_BLIND = 46, W_BACK,                                        // 46, 47
	W_PACKCARD = 48,     // 48..52
	W_SKIPBLIND = 53, W_VOUCHER = 54,
	W_DECK = 55, W_STAKE = 56, W_START = 57,                      // deck screen
	W_CONTINUE = 58, W_NEWRUN = 59,                               // title screen with a saved run
};

enum { SC_TITLE, SC_BLIND, SC_ROUND, SC_CASHOUT, SC_SHOP, SC_PACK, SC_INFO, SC_OVER, SC_WIN, SC_DECK };
enum { PH_INPUT, PH_SCORING, PH_TALLY, PH_BANNER };

typedef struct
{
	u8  screen, prevScreen;
	u8  phase;
	Widget w[WMAX];
	u8  nw;
	u8  focus;            // widget index or 0xFF
	u8  defFocus;         // widget id focused by the first keyboard/joystick direction press
	u16 sel;              // selected hand positions
	u8  itemKind;         // 0 none, 1 joker, 2 consumable (selected for sell/use)
	u8  itemIdx;
	u8  timer, step;
	u8  evi;              // next event to play
	u8  hz;               // 50 or 60
	u16 frame;
	ScoreOut so;
	i32 shownScore;       // animated round score
	i16 shownMoney;
	u16 shownChips, shownMult;
	u8  shownHand;        // hand type name shown in the HUD (0xFF none)
	Cash cash[CASH_MAX];
	u8  nCash, cashShown;
	i16 cashTotal;
	u8  packReturn;       // screen to go back to when a pack is closed
	u8  msgTimer;
	const char* msg;
	u8  dirty;            // D_* redraw flags
	u8  hnc;              // the pending hand repaint comes from focus changes only: repaint cards in place, no felt clear (no flicker)
	u8  hlo, hhi;         // hand cards to repaint (range) when D_HAND is set
	u16 jmask;            // Joker (bits 0..7) and consumable (bits 8..11) slots to repaint individually
	u8  bmask;            // buttons to repaint individually
	u8  infoDelay;        // frames to wait before repainting the info panel (avoids repainting while the pointer sweeps)
} UI;
#define D_HAND    1
#define D_JOKERS  2
#define D_BUTTONS 4
#define D_INFO    8
#define D_HUD     16
#define D_PLAY    32
#define D_ALL     (D_HAND | D_BUTTONS | D_INFO | D_HUD | D_PLAY)   /* not the joker row: redrawn only when it changes */
#define D_EVERYTHING 63
extern UI ui;

// core (ui_core.c)
void ui_init(void) BANKED;
void ui_update(void) BANKED;
void ui_clear_widgets(void) BANKED;
void ui_add(u8 id, u8 x, u8 y, u8 w, u8 h) BANKED;
u8   ui_find(u8 id) BANKED;
void ui_ensure(u8 id, u8 x, u8 y, u8 w, u8 h) BANKED;
u8   ui_hit(u8 x, u8 y) BANKED;
u8   ui_focus_id(void) BANKED;
bool ui_pointer_focus(void) BANKED;   // true when the focus changed
void ui_set_focus(u8 idx) BANKED;
void ui_info_tick(void) BANKED;
void draw_jslot(u8 id) BANKED;
u16  jmask_of(u8 id) BANKED;            // slots to repaint when this slot's focus changes
void draw_jmask(u16 m) BANKED;          // one joker / consumable slot (W_JOKER+i, W_CONS+i) with its focus ring
void scr_focus(u8 oldId, u8 newId) BANKED;   // repaint what a focus change touches (per screen)
void rnd_focus(u8 o, u8 n) BANKED;
void rnd_rebuild(void) BANKED;
void shop_focus(u8 o, u8 n) BANKED;
void pack_focus(u8 o, u8 n) BANKED;
void blind_focus(u8 o, u8 n) BANKED;
void ui_nav(u8 dir) BANKED;
void ui_goto(u8 screen) BANKED;
void ui_msg(const char* m) BANKED;
void ui_button(u8 id, const char* label, u8 col, bool enabled);
void ui_button_ring(u8 id);      // focus changed: only the 1-2 px ring is repainted (the label is never erased)
void ui_focus_ring(u8 id);
void hud_draw(void) BANKED;              // static layout + all fields
void hud_update(void) BANKED;            // refresh fields that changed
void hud_mini(void) BANKED;
void draw_tag(u8 tag, u8 x, u8 y) BANKED;   // 16x16 tag icon (tag = TG_* )              // money / ante panel for non-round screens
void hud_hand(u8 type, u16 chips, u16 mult) BANKED;   // type 0xFF = blank
void info_show(u8 id) BANKED;            // description of widget `id` in the info panel
void info_blind(void) BANKED;
void draw_joker_row(bool shop) BANKED;
u8   joker_item_at(u8 id) BANKED;
void snd(u8 id) BANKED;
void snd_event(u8 kind, u8 n) BANKED;

// screens
void scr_deck(void) BANKED;
void upd_deck(void) BANKED;
void deck_focus(u8 o, u8 n) BANKED;
void title_focus(u8 o, u8 n) BANKED;
void scr_title(void) BANKED;
void scr_blind(void) BANKED;
void scr_round(void) BANKED;
void scr_cashout(void) BANKED;
void scr_shop(void) BANKED;
void scr_pack(void) BANKED;
void scr_info(void) BANKED;
void scr_over(void) BANKED;
void scr_win(void) BANKED;
void upd_title(void) BANKED;
void upd_blind(void) BANKED;
void upd_round(void) BANKED;
void upd_cashout(void) BANKED;
void upd_shop(void) BANKED;
void upd_pack(void) BANKED;
void upd_info(void) BANKED;
void upd_over(void) BANKED;

// frame helpers
#define FR(n)   ((u8)(((u16)(n) * ui.hz) / 60))

// shared strings (ui_text.c, always-mapped SEG20)
extern const char T_PLAY[], T_START[], T_CONTINUE[], T_NEWRUN[], T_DISCARD[], T_RANK[], T_SUIT[], T_RUNINFO[], T_SELECT[], T_BACK[], T_NEXT[], T_USE[], T_SKIP[], T_SKIPBLIND[];
extern const char I_BLINDDESC[], I_FACEDOWN[], I_EMPTYJ[], I_EMPTYC[], I_PLAYD[], I_DISCD[], I_RANKD[], I_SUITD[], I_INFOD[], I_SELLD[], I_USED[], I_REROLLD[], I_NEXTD[], I_RANKT[], I_SUITT[], I_PLAYT[], I_CHIPSW[], I_SMALLB[], I_BIGB[];
extern const char* const g_PackName[], * const g_PackDesc[];
extern const char M_NEEDCARDS[], M_CANTUSE[], M_NOMONEY[], M_NOROOM[], M_SOLDOUT[], M_NOJOKER[], M_NOCONS[], M_ETERNAL[];

extern const char* const g_EnhText[], * const g_EdName[], * const g_SealName[], * const g_SealShort[];
extern const char* const g_DeckName[], * const g_DeckDesc[], * const g_StakeName[], * const g_StakeDesc[];
