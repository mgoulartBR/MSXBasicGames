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
#define JOKER_Y       2
#define PLAY_Y        54
#define HAND_Y        120
#define HAND_RAISE    10
#define BTN_Y         158
#define BTN_H         14
#define INFO_Y        176
#define JOKER_X(i)    (66 + (i) * 26)
#define CONS_X(i)     (204 + (i) * 26)

//-----------------------------------------------------------------------------
// widgets
//-----------------------------------------------------------------------------
typedef struct { u8 x, y, w, h, id; } Widget;
#define WMAX 26
enum
{
	W_HAND = 0,          // 0..15
	W_JOKER = 16,        // 16..20
	W_CONS = 21,         // 21..22
	W_PLAY = 23, W_DISCARD, W_SORT_RANK, W_SORT_SUIT, W_INFO,    // 23..27
	W_SELL = 28, W_USE,                                          // 28, 29
	W_SHOPCARD = 30,     // 30..32
	W_PACK = 33,         // 33..34
	W_REROLL = 35, W_NEXT, W_OK, W_SKIP,                         // 35..38
	W_BLIND = 39, W_BACK,                                        // 39, 40
	W_PACKCARD = 41,     // 41..45
};

enum { SC_TITLE, SC_BLIND, SC_ROUND, SC_CASHOUT, SC_SHOP, SC_PACK, SC_INFO, SC_OVER, SC_WIN };
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
	u8  msgTimer;
	const char* msg;
	u8  dirty;            // D_* redraw flags
} UI;
#define D_HAND    1
#define D_JOKERS  2
#define D_BUTTONS 4
#define D_INFO    8
#define D_HUD     16
#define D_PLAY    32
#define D_ALL     63
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
void ui_nav(u8 dir) BANKED;
void ui_goto(u8 screen) BANKED;
void ui_msg(const char* m) BANKED;
void ui_button(u8 id, const char* label, u8 col, bool enabled) BANKED;
void ui_focus_ring(u8 id) BANKED;
void hud_draw(void) BANKED;              // static layout + all fields
void hud_update(void) BANKED;            // refresh fields that changed
void hud_mini(void) BANKED;              // money / ante panel for non-round screens
void hud_hand(u8 type, u16 chips, u16 mult) BANKED;   // type 0xFF = blank
void info_show(u8 id) BANKED;            // description of widget `id` in the info panel
void info_blind(void) BANKED;
void draw_joker_row(bool shop) BANKED;
u8   joker_item_at(u8 id) BANKED;
void snd(u8 id) BANKED;
void snd_event(u8 kind, u8 n) BANKED;

// screens
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
extern const char T_PLAY[], T_DISCARD[], T_RANK[], T_SUIT[], T_RUNINFO[], T_SELECT[], T_BACK[], T_NEXT[], T_USE[], T_SKIP[];
extern const char M_NEEDCARDS[], M_CANTUSE[], M_NOMONEY[], M_NOROOM[], M_SOLDOUT[], M_NOJOKER[], M_NOCONS[];
