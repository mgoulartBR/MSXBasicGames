// Deck and stake selection screen.
#include "ui.h"

//-----------------------------------------------------------------------------
// deck + stake selection
//-----------------------------------------------------------------------------
#define DECK_ROW_Y(r) ((u8)(22 + (r) * 70))

// focus ring only: four fast fills instead of repainting the whole row (hovering with the mouse used to repaint all the text
// and made the music stutter)
static void deck_ring(u8 r)
{
	u8 y = DECK_ROW_Y(r), col = ui_focus_id() == W_DECK + r ? COL_GOLD : COL_SLATE;
	Vid_Fill(16, y, 224, 1, col); Vid_Fill(16, y + 63, 224, 1, col);
	Vid_Fill(16, y, 2, 64, col); Vid_Fill(238, y, 2, 64, col);
}

static void draw_deck_row(u8 r)
{
	u8 y = DECK_ROW_Y(r), sel = r ? g_stakeSel : g_deckSel, cnt = r ? STAKE_COUNT : DECK_COUNT;
	Vid_Panel(16, y, 224, 64, COL_INK, COL_SLATE);
	Vid_Text(24, y + 4, r ? "Stake" : "Deck", TC_SLATE);
	Vid_Num(210 - Vid_NumW(sel + 1), y + 4, sel + 1, TC_SLATE); Vid_Text(214, y + 4, "/", TC_SLATE); Vid_Num(221, y + 4, cnt, TC_SLATE);
	Vid_Text(24, y + 15, "<", TC_WHITE); Vid_Text(226, y + 15, ">", TC_WHITE);
	Vid_TextC(128, y + 15, r ? g_StakeName[sel] : g_DeckName[sel], TC_GOLD);
	Vid_Wrap(24, y + 30, r ? g_StakeDesc[sel] : g_DeckDesc[sel], 208, TC_WHITE, 3);
	deck_ring(r);
}

void scr_deck(void) BANKED
{
	Vid_Clear(COL_FELT);
	Vid_TextC(128, 6, "New Run", TC_GOLD);
	ui_add(W_DECK, 16, DECK_ROW_Y(0), 224, 64);
	ui_add(W_STAKE, 16, DECK_ROW_Y(1), 224, 64);
	ui_add(W_START, 78, 168, 100, 18);
	ui_set_focus(ui_find(W_START));
	draw_deck_row(0); draw_deck_row(1);
	ui_button(W_START, T_START, COL_GREEN, TRUE);
	Vid_TextC(128, 192, "Left / Right: change    ESC: back", TC_SLATE);
}

void deck_focus(u8 o, u8 n) BANKED
{
	for (u8 k = 0; k < 2; k++)
	{
		u8 id = k ? n : o;
		if (id == W_DECK) deck_ring(0);
		else if (id == W_STAKE) deck_ring(1);
		else if (id == W_START) ui_button(W_START, T_START, COL_GREEN, TRUE);
	}
}

void upd_deck(void) BANKED
{
	ui_pointer_focus();
	u16 p = in.pressed;
	if (p & IN_BACK) { ui_goto(SC_TITLE); return; }
	if (p & (IN_UP | IN_DOWN)) ui_nav((p & IN_UP) ? IN_UP : IN_DOWN);
	u8 id = ui_focus_id();
	i8 step = (p & IN_LEFT) ? -1 : ((p & IN_RIGHT) ? 1 : 0);
	bool act = (p & (IN_OK | IN_PLAY)) != 0;
	if (in.click)
	{
		u8 h = ui_hit(in.mx, in.my);
		if (h == 0xFF) return;
		id = ui.w[h].id; act = TRUE;
		if (id != W_START) step = in.mx < 128 ? -1 : 1;
	}
	if (act && id == W_START)
	{
		rng_seed((u16)(ui.frame * 31 + ui.timer * 7 + in.mx + in.my * 3));
		run_new();
		ui.shownHand = 0xFF;
		ui.packReturn = SC_SHOP;
		ui_goto(SC_BLIND);
		return;
	}
	if (act && !step && id != W_START) step = 1;
	if (step && (id == W_DECK || id == W_STAKE))
	{
		u8* v = id == W_DECK ? &g_deckSel : &g_stakeSel;
		u8 cnt = id == W_DECK ? DECK_COUNT : STAKE_COUNT;
		*v = (u8)((*v + cnt + step) % cnt);
		draw_deck_row(id - W_DECK);
		Snd_Play(SFX_SELECT);
	}
}

