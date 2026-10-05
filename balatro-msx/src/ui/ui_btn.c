// Buttons and focus rings. Lives in the fixed code area (not a mapper segment): the banked UI segments are full.
#include "ui.h"

void ui_focus_ring(u8 id)
{
	u8 i = ui_find(id);
	if (i == 0xFF) return;
	Widget* k = &ui.w[i];
	Vid_Frame(k->x - 1, k->y - 1, k->w + 2, k->h + 2, COL_GOLD);
}

void ui_button(u8 id, const char* label, u8 col, bool enabled)
{
	u8 i = ui_find(id);
	if (i == 0xFF) return;
	Widget* k = &ui.w[i];
	Vid_Panel(k->x, k->y, k->w, k->h, enabled ? col : COL_SLATE, COL_INK);
	Vid_TextC(k->x + k->w / 2, k->y + (k->h - 9) / 2, label, enabled ? TC_WHITE : TC_INK);
	if (i == ui.focus) ui_button_ring(id);
}

// The ring replaces border pixels (gold when focused, ink otherwise), so moving the focus never touches the label or the fill.
void ui_button_ring(u8 id)
{
	u8 i = ui_find(id);
	if (i == 0xFF) return;
	Widget* k = &ui.w[i];
	u8 c = i == ui.focus ? COL_GOLD : COL_INK;
	Vid_Frame(k->x, k->y, k->w, k->h, c);
	Vid_Fill(k->x + 1, k->y, 1, k->h, c);
	Vid_Fill(k->x + k->w - 2, k->y, 1, k->h, c);
}

