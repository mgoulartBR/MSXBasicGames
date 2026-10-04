// Play screen (port of PlayState/Feed/Paper): news feed on the left, newspaper grid on the right.
//
// Layout in 8x8 tiles (screen 32x24):
//   cols 0-4   : clock, day, End Day button, readers, loyalty gauge
//   cols 5-15  : news feed (rows 1-17, header on row 0)
//   cols 16-31 : paper (masthead rows 0-2, 4x5 cell grid rows 3-17, each cell 4x3 tiles)
//   rows 18-23 : details of the selected item + controls
//
// The original is mouse driven (drag & drop). Here a focus model is used: the feed has a
// selection, the article is "picked up" and a footprint is moved over the paper grid.
#include "ui.h"
#include "logic.h"
#include "widgets.h"
#include "../platform/msx_isr.h"
#include "../data/gfx_data.h"
#include "../platform/msx_seg.h"

#define PAPER_COL 16
#define PAPER_ROW 3
#define CELL_W 4
#define CELL_H 3
#define GRID_W 4
#define GRID_H 5

#define FEED_COL 5
#define FEED_W 11
#define FEED_ROW 1
#define FEED_ROWS 17
#define FEED_TEXT_X 9 // marker tile (8px) + 1
#define FEED_TEXT_W 78
#define FEED_MAX_LINES 3
#define MAX_ART 10

enum { MODE_FEED, MODE_PLACE, MODE_PAPER };

typedef struct
{
	u8 used;
	u8 slot;
	u8 size;
	u8 cx, cy;
} Art;

static char s_Blurb[MAX_DAY_ITEMS][112];
static char s_Label[MAX_DAY_ITEMS][84];
static u8 s_Shown[MAX_DAY_ITEMS];
static u8 s_FeedItem[MAX_DAY_ITEMS];  // news index per feed slot
static u8 s_Placed[MAX_DAY_ITEMS];    // size placed on paper, 0xFF = not placed
static u8 s_FeedCount, s_FeedTop, s_FeedSel;
static Art s_Art[MAX_ART];

static u8 s_Mode;
static u8 s_SelSize;           // currently chosen size for the selected feed entry
static u8 s_PickSlot;          // slot being placed
static u8 s_PickSize;
static u8 s_Cx, s_Cy;          // footprint / cursor position in cells
static u8 s_FpX, s_FpY, s_FpW, s_FpH; // painted footprint (tile rect), 0 width = none
static u16 s_Time;
static u16 s_TickInc;
static u8 s_LastFrame;
static u8 s_Speed10;           // End Day pressed
static bool s_AlarmDone;
static bool s_Over;
static TextLine s_Lines[8];

static u8 s_ArtBuf[12 * 9 * 8];
static Canvas s_ArtCv = { s_ArtBuf, 12, 9 };
static u8 s_CellBuf[CELL_W * CELL_H * 8];
static Canvas s_CellCv = { s_CellBuf, CELL_W, CELL_H };

// --- size helpers: SIZE_S=1x2, SIZE_M=2x2, SIZE_B=3x3 cells
static u8 size_w(u8 sz) { return sz == SIZE_B ? 3 : (sz == SIZE_M ? 2 : 1); }
static u8 size_h(u8 sz) { return sz == SIZE_S ? 2 : (sz == SIZE_M ? 2 : 3); }
// selector order shown to the player: BIG, MED, SMALL (as the icons on the original feed)
static const u8 s_SelToSize[3] = { SIZE_B, SIZE_M, SIZE_S };

//-----------------------------------------------------------------------------
// Paper

static void cell_draw_empty(u8 cx, u8 cy)
{
	Cv_Clear(&s_CellCv, 0);
	// dotted grid lines on the right/bottom edges; solid outer frame
	Cv_VLine(&s_CellCv, CELL_W * 8 - 1, 0, CELL_H * 8, cx != GRID_W - 1);
	Cv_HLine(&s_CellCv, 0, CELL_H * 8 - 1, CELL_W * 8, cy != GRID_H - 1);
	if (cx == 0)
		Cv_VLine(&s_CellCv, 0, 0, CELL_H * 8, 0);
	if (cy == 0)
		Cv_HLine(&s_CellCv, 0, 0, CELL_W * 8, 0);
	Gfx_BlitCanvas(PAPER_COL + cx * CELL_W, PAPER_ROW + cy * CELL_H, &s_CellCv);
}

static void paper_draw_empty(void)
{
	Gfx_FillColor(PAPER_COL, PAPER_ROW, GRID_W * CELL_W, GRID_H * CELL_H, CT_INK);
	for (u8 cy = 0; cy < GRID_H; ++cy)
		for (u8 cx = 0; cx < GRID_W; ++cx)
			cell_draw_empty(cx, cy);
}

static void art_erase(const Art* a)
{
	for (u8 y = 0; y < size_h(a->size); ++y)
		for (u8 x = 0; x < size_w(a->size); ++x)
			cell_draw_empty(a->cx + x, a->cy + y);
}

static void art_draw(const Art* a)
{
	char head[48];
	u8 cw = size_w(a->size), ch = size_h(a->size);
	u8 tw = cw * CELL_W, th = ch * CELL_H;
	u8 pw = tw * 8, ph = th * 8;
	u8 maxLines = (a->size == SIZE_B) ? 3 : 2;

	s_ArtCv.w = tw;
	s_ArtCv.h = th;
	Cv_Clear(&s_ArtCv, 0);
	News_Head(head, s_FeedItem[a->slot]);
	u8 n = Text_Wrap(head, pw - 4, s_Lines, maxLines);
	for (u8 i = 0; i < n; ++i)
		Cv_Text(&s_ArtCv, 2, i, head + s_Lines[i].start, s_Lines[i].len);
	// rule under the headline, then "text" bars in 1..3 columns
	u8 ry = n * 8 + 1;
	Cv_HLine(&s_ArtCv, 2, ry, pw - 4, 0);
	u8 colw = (pw - 4) / cw;
	for (u8 c = 0; c < cw; ++c)
	{
		u8 x0 = 2 + c * colw;
		if (c)
			Cv_VLine(&s_ArtCv, x0 - 1, ry + 2, ph - ry - 4, 0);
		for (u8 y = ry + 4; y + 2 < ph - 1; y += 4)
			Cv_FillRect(&s_ArtCv, x0 + 1, y, colw - 4, 2);
	}
	// dotted outline (left, right, bottom)
	Cv_VLine(&s_ArtCv, 0, 0, ph, 1);
	Cv_VLine(&s_ArtCv, pw - 1, 0, ph, 1);
	Cv_HLine(&s_ArtCv, 0, ph - 1, pw, 1);
	Gfx_BlitCanvas(PAPER_COL + a->cx * CELL_W, PAPER_ROW + a->cy * CELL_H, &s_ArtCv);
}

static Art* art_at(u8 cx, u8 cy)
{
	for (u8 i = 0; i < MAX_ART; ++i)
	{
		Art* a = &s_Art[i];
		if (a->used && cx >= a->cx && cx < a->cx + size_w(a->size) && cy >= a->cy && cy < a->cy + size_h(a->size))
			return a;
	}
	return 0;
}

static bool area_free(u8 cx, u8 cy, u8 sz)
{
	for (u8 y = 0; y < size_h(sz); ++y)
		for (u8 x = 0; x < size_w(sz); ++x)
			if (art_at(cx + x, cy + y))
				return FALSE;
	return TRUE;
}

static Art* art_of_slot(u8 slot)
{
	for (u8 i = 0; i < MAX_ART; ++i)
		if (s_Art[i].used && s_Art[i].slot == slot)
			return &s_Art[i];
	return 0;
}

static void art_remove(Art* a)
{
	art_erase(a);
	s_Placed[a->slot] = 0xFF;
	a->used = 0;
}

//-----------------------------------------------------------------------------
// Footprint highlight (colour table only - cheap)

static void fp_hide(void)
{
	if (s_FpW)
		Gfx_FillColor(s_FpX, s_FpY, s_FpW, s_FpH, CT_INK);
	s_FpW = 0;
}

static void fp_show(u8 cx, u8 cy, u8 cw, u8 ch, u8 color)
{
	fp_hide();
	s_FpX = PAPER_COL + cx * CELL_W;
	s_FpY = PAPER_ROW + cy * CELL_H;
	s_FpW = cw * CELL_W;
	s_FpH = ch * CELL_H;
	Gfx_FillColor(s_FpX, s_FpY, s_FpW, s_FpH, color);
}

static void place_preview(void)
{
	u8 cw = size_w(s_PickSize), ch = size_h(s_PickSize);
	fp_show(s_Cx, s_Cy, cw, ch, area_free(s_Cx, s_Cy, s_PickSize) ? CT_OK : CT_BAD);
}

static void paper_cursor(void)
{
	Art* a = art_at(s_Cx, s_Cy);
	if (a)
		fp_show(a->cx, a->cy, size_w(a->size), size_h(a->size), CT_GRAY);
	else
		fp_show(s_Cx, s_Cy, 1, 1, CT_GRAY);
}

//-----------------------------------------------------------------------------
// Feed

static u8 s_Nl[MAX_DAY_ITEMS]; // wrapped line count per feed slot (1..FEED_MAX_LINES), cached

static u8 entry_lines(u8 slot) { return s_Nl[slot]; }

static u8 entry_row(u8 slot)
{
	u8 row = FEED_ROW;
	for (u8 i = s_FeedTop; i < slot; ++i)
		row += s_Nl[i];
	return row;
}

static bool entry_visible(u8 slot)
{
	return slot >= s_FeedTop && slot < s_FeedCount && entry_row(slot) + s_Nl[slot] <= FEED_ROW + FEED_ROWS;
}

static void feed_fix_scroll(void)
{
	for (;;)
	{
		u8 rows = 0;
		for (u8 i = s_FeedTop; i < s_FeedCount; ++i)
			rows += s_Nl[i];
		if (rows <= FEED_ROWS || s_FeedTop + 1 >= s_FeedCount)
			break;
		++s_FeedTop;
	}
	if (s_FeedCount && s_FeedSel < s_FeedCount && s_FeedSel < s_FeedTop)
		s_FeedSel = s_FeedTop;
}

static bool is_rebel_slot(u8 slot) { return News_IsRebel(s_FeedItem[slot]); }

static u8 entry_color(u8 slot, bool selected)
{
	if (is_rebel_slot(slot))
		return selected ? CT_RED_INV : CT_RED;
	return selected ? CT_INV : CT_INK;
}

static void feed_color_entry(u8 slot)
{
	bool sel = (s_Mode == MODE_FEED) && (slot == s_FeedSel);
	Gfx_FillColor(FEED_COL, entry_row(slot), FEED_W, s_Nl[slot], entry_color(slot, sel));
}

// colours only (cheap): used when the selection or the mode changes
static void feed_colors(void)
{
	for (u8 i = s_FeedTop; i < s_FeedCount; ++i)
		if (entry_visible(i))
			feed_color_entry(i);
}

// text + marker + colour of one entry (incremental redraw)
static void feed_draw_entry(u8 slot)
{
	u8 row = entry_row(slot);
	u8 n = Text_Wrap(s_Label[slot], FEED_TEXT_W, s_Lines, FEED_MAX_LINES);
	for (u8 l = 0; l < s_Nl[slot]; ++l)
	{
		g_Strip.w = FEED_W;
		Cv_Clear(&g_Strip, 0);
		if (l == 0 && s_Placed[slot] != 0xFF)
		{
			static const char mk[3] = { 'S', 'M', 'B' };
			Cv_Text(&g_Strip, 2, 0, &mk[s_Placed[slot]], 1);
		}
		if (l < n)
			Cv_Text(&g_Strip, FEED_TEXT_X, 0, s_Label[slot] + s_Lines[l].start, s_Lines[l].len);
		Gfx_BlitCanvas(FEED_COL, row + l, &g_Strip);
	}
	feed_color_entry(slot);
}

static void feed_redraw_entry(u8 slot)
{
	if (entry_visible(slot))
		feed_draw_entry(slot);
}

// full redraw (background + every visible entry): only on scroll
static void feed_draw(void)
{
	Gfx_FillPattern(FEED_COL, FEED_ROW, FEED_W, FEED_ROWS, 0xAA, 0x55);
	Gfx_FillColor(FEED_COL, FEED_ROW, FEED_W, FEED_ROWS, CT_INK);
	for (u8 i = s_FeedTop; i < s_FeedCount; ++i)
		if (entry_visible(i))
			feed_draw_entry(i);
}

// returns TRUE if the list scrolled (full redraw needed)
static bool feed_add(u8 dayIdx)
{
	u8 slot = s_FeedCount;
	u8 item = g_Day.item[dayIdx];
	u8 oldTop = s_FeedTop;
	bool endSel = (s_FeedCount > 0 && s_FeedSel >= s_FeedCount);
	s_FeedItem[slot] = item;
	s_Placed[slot] = 0xFF;
	News_Blurb(s_Blurb[slot], item);
	if (g_News[item].head)
		News_Head(s_Label[slot], item);
	else
	{
		// rebel message: first characters in the feed, whole text in the detail panel
		u8 i = 0;
		while (s_Blurb[slot][i] && i < 78)
		{
			s_Label[slot][i] = s_Blurb[slot][i];
			++i;
		}
		s_Label[slot][i] = 0;
	}
	{
		u8 n = Text_Wrap(s_Label[slot], FEED_TEXT_W, s_Lines, FEED_MAX_LINES);
		s_Nl[slot] = n ? n : 1;
	}
	s_FeedCount++;
	s_Shown[dayIdx] = 1;
	if (endSel)
		s_FeedSel = s_FeedCount; // keep the End Day button selected
	feed_fix_scroll();
	// a new rebel message grabs the selection so it is read
	if (is_rebel_slot(slot) && s_Mode == MODE_FEED)
		s_FeedSel = slot;
	return s_FeedTop != oldTop;
}

//-----------------------------------------------------------------------------
// Panels

static void draw_sidebar_static(void)
{
	char buf[8];
	u8 n = 0;
	Gfx_FillColor(0, 0, 5, 24, CT_INK);
	Gfx_TextCenter(0, 4, 5, "6AM-6PM", CT_INK);
	buf[n++] = 'D'; buf[n++] = 'a'; buf[n++] = 'y'; buf[n++] = ' ';
	if (g_Game.day >= 10)
		buf[n++] = '0' + g_Game.day / 10;
	buf[n++] = '0' + g_Game.day % 10;
	buf[n] = 0;
	Gfx_TextCenter(0, 5, 5, buf, CT_INK);
	Readers_Draw(0, 10, g_Game.readers, 0, FALSE, TRUE);
	Meter_Draw(0, 12, g_Game.loyalty);
}

static void end_button_draw(bool selected)
{
	static const char label[] = "End Day";
	// 5x3 tile button on rows 7-9: frame (top edge on the last pixel row of row 7,
	// bottom edge on the first pixel row of row 9) around the label on row 8
	g_Strip.w = 5;
	g_Strip.h = 3;
	Cv_Clear(&g_Strip, 0);
	Cv_HLine(&g_Strip, 0, 7, 40, 0);
	Cv_HLine(&g_Strip, 0, 16, 40, 0);
	Cv_VLine(&g_Strip, 0, 7, 10, 0);
	Cv_VLine(&g_Strip, 39, 7, 10, 0);
	Cv_Text(&g_Strip, (40 - Text_Width(label, 7)) >> 1, 1, label, 7);
	Gfx_BlitCanvas(0, 7, &g_Strip);
	g_Strip.h = 1;
	Gfx_FillColor(0, 7, 5, 3, selected ? CT_INV : CT_INK);
}

static void panel_clear(void)
{
	Gfx_FillPattern(0, 18, 32, 6, 0, 0);
	Gfx_FillColor(0, 18, 32, 6, CT_INK);
}

static void panel_line(u8 row, const char* s, u8 color)
{
	Gfx_TextLine(0, row, 32, 2, s, Text_Len(s), color);
}

static void panel_size_selector(void)
{
	static const char* const names[3] = { "BIG 3x3", "MED 2x2", "SMALL 1x2" };
	// three 7-tile blocks on row 22
	for (u8 i = 0; i < 3; ++i)
	{
		bool on = (s_SelSize == i);
		Gfx_TextCenter(1 + i * 9, 22, 8, names[i], on ? CT_INV : CT_INK);
	}
}

static void panel_hint(const char* s)
{
	Gfx_TextLine(0, 23, 32, 2, s, Text_Len(s), CT_INK);
}

static void panel_draw(void)
{
	panel_clear();
	if (s_Mode == MODE_PLACE)
	{
		panel_line(18, "Placing article. Pick a spot on the paper:", CT_INK);
		panel_line(20, s_Label[s_PickSlot], CT_INK);
		panel_hint("ARROWS:move  SPACE:drop  ESC:discard");
		return;
	}
	if (s_Mode == MODE_PAPER)
	{
		panel_line(18, "Paper: select a placed article to pick it up", CT_INK);
		panel_hint("ARROWS:move  SPACE:pick up  ESC:back to feed");
		return;
	}
	// feed mode
	if (s_FeedSel >= s_FeedCount)
	{
		panel_line(18, "End the day now: time runs 10x faster and", CT_INK);
		panel_line(19, "the paper goes to print.", CT_INK);
		panel_hint("UP:feed  SPACE:end day  ESC:paper");
		return;
	}
	const char* t = s_Blurb[s_FeedSel];
	u8 n = Text_Wrap(t, 244, s_Lines, 4);
	u8 col = is_rebel_slot(s_FeedSel) ? CT_RED : CT_INK;
	for (u8 i = 0; i < n; ++i)
		Gfx_TextLine(0, 18 + i, 32, 2, t + s_Lines[i].start, s_Lines[i].len, col);
	if (g_News[s_FeedItem[s_FeedSel]].head)
	{
		panel_size_selector();
		panel_hint("UP/DN:item LT/RT:size SPACE:place ESC:paper");
	}
	else
		panel_hint("UP/DN:item   (no article for this message)");
}

//-----------------------------------------------------------------------------
// Game flow

static void popup_dayover(void)
{
	// centred black box (22x7 tiles) with the original message and a "button" bar
	Gfx_FillPattern(5, 8, 22, 7, 0, 0);
	Gfx_FillColor(5, 8, 22, 7, CT_INV);
	Gfx_TextCenter(5, 9, 22, "The day is over. There is no", CT_INV);
	Gfx_TextCenter(5, 10, 22, "more time. We must send to", CT_INV);
	Gfx_TextCenter(5, 11, 22, "print immediately.", CT_INV);
	Gfx_TextCenter(7, 13, 18, "[SPACE]  Send to Print", CT_INK);
}

static void summarize(void)
{
	PaperSummary sum;
	Summary_Init(&sum);
	for (u8 i = 0; i < MAX_ART; ++i)
		if (s_Art[i].used)
			Summary_Add(&sum, s_FeedItem[s_Art[i].slot], s_Art[i].size);
	Readership_Apply(&sum);
	for (u8 i = 0; i < MAX_ART; ++i)
		if (s_Art[i].used)
			News_MarkUsed(s_FeedItem[s_Art[i].slot]);
}

static void pick_up(u8 slot, u8 size, u8 cx, u8 cy)
{
	s_PickSlot = slot;
	s_PickSize = size;
	s_Cx = cx;
	s_Cy = cy;
	if (s_Cx + size_w(size) > GRID_W) s_Cx = GRID_W - size_w(size);
	if (s_Cy + size_h(size) > GRID_H) s_Cy = GRID_H - size_h(size);
	s_Mode = MODE_PLACE;
	Sfx_Play(SFX_DRAG);
	panel_draw();
	feed_colors();
	place_preview();
}

static void to_feed_mode(void)
{
	fp_hide();
	s_Mode = MODE_FEED;
	panel_draw();
	feed_colors();
	end_button_draw(s_FeedSel >= s_FeedCount);
}

static void handle_feed(void)
{
	bool changed = FALSE;
	if (g_Push & IN_UP)
	{
		if (s_FeedSel > s_FeedTop && s_FeedSel <= s_FeedCount)
		{
			--s_FeedSel;
			changed = TRUE;
		}
	}
	if (g_Push & IN_DOWN)
	{
		if (s_FeedSel < s_FeedCount)
		{
			++s_FeedSel;
			changed = TRUE;
		}
	}
	if (g_Push & IN_LEFT)
	{
		if (s_SelSize > 0) { --s_SelSize; changed = TRUE; }
	}
	if (g_Push & IN_RIGHT)
	{
		if (s_SelSize < 2) { ++s_SelSize; changed = TRUE; }
	}
	if (changed)
	{
		Sfx_Play(SFX_CLICK);
		panel_draw();
		feed_colors();
		end_button_draw(s_FeedSel >= s_FeedCount);
	}
	if (g_Push & IN_A)
	{
		if (s_FeedSel >= s_FeedCount)
		{
			s_Speed10 = 1;
			Sfx_Play(SFX_CLICK);
		}
		else if (g_News[s_FeedItem[s_FeedSel]].head)
		{
			// same news item may only be on the paper once: moving it removes the old copy
			Art* old = art_of_slot(s_FeedSel);
			if (old)
			{
				art_remove(old);
				feed_redraw_entry(s_FeedSel);
			}
			pick_up(s_FeedSel, s_SelToSize[s_SelSize], 0, 0);
		}
		else
			Sfx_Play(SFX_ERROR);
	}
	if (g_Push & IN_B)
	{
		s_Mode = MODE_PAPER;
		s_Cx = s_Cy = 0;
		Sfx_Play(SFX_CLICK);
		panel_draw();
		feed_colors();
		end_button_draw(FALSE);
		paper_cursor();
	}
}

static void handle_place(void)
{
	bool moved = FALSE;
	u8 mw = GRID_W - size_w(s_PickSize), mh = GRID_H - size_h(s_PickSize);
	if ((g_Push & IN_LEFT) && s_Cx > 0) { --s_Cx; moved = TRUE; }
	if ((g_Push & IN_RIGHT) && s_Cx < mw) { ++s_Cx; moved = TRUE; }
	if ((g_Push & IN_UP) && s_Cy > 0) { --s_Cy; moved = TRUE; }
	if ((g_Push & IN_DOWN) && s_Cy < mh) { ++s_Cy; moved = TRUE; }
	if (moved)
		place_preview();
	if (g_Push & IN_A)
	{
		if (area_free(s_Cx, s_Cy, s_PickSize))
		{
			for (u8 i = 0; i < MAX_ART; ++i)
			{
				if (!s_Art[i].used)
				{
					Art* a = &s_Art[i];
					a->used = 1;
					a->slot = s_PickSlot;
					a->size = s_PickSize;
					a->cx = s_Cx;
					a->cy = s_Cy;
					s_Placed[s_PickSlot] = s_PickSize;
					fp_hide();
					art_draw(a);
					Sfx_Play(SFX_DROP);
					s_FeedSel = s_PickSlot;
					feed_redraw_entry(s_PickSlot);
					to_feed_mode();
					return;
				}
			}
		}
		Sfx_Play(SFX_ERROR);
	}
	if (g_Push & IN_B)
	{
		// dropped outside the paper in the original: the article is discarded
		Sfx_Play(SFX_DROP);
		to_feed_mode();
	}
}

static void handle_paper(void)
{
	bool moved = FALSE;
	if ((g_Push & IN_LEFT) && s_Cx > 0) { --s_Cx; moved = TRUE; }
	if ((g_Push & IN_RIGHT) && s_Cx < GRID_W - 1) { ++s_Cx; moved = TRUE; }
	if ((g_Push & IN_UP) && s_Cy > 0) { --s_Cy; moved = TRUE; }
	if ((g_Push & IN_DOWN) && s_Cy < GRID_H - 1) { ++s_Cy; moved = TRUE; }
	if (moved)
		paper_cursor();
	if (g_Push & IN_A)
	{
		Art* a = art_at(s_Cx, s_Cy);
		if (a)
		{
			u8 slot = a->slot, size = a->size, cx = a->cx, cy = a->cy;
			fp_hide();
			art_remove(a);
			feed_redraw_entry(slot);
			pick_up(slot, size, cx, cy);
		}
		else
			Sfx_Play(SFX_ERROR);
	}
	if (g_Push & IN_B)
	{
		Sfx_Play(SFX_CLICK);
		to_feed_mode();
	}
}

u8 Scr_Play(void)
{
	u8 i;
	Gfx_Display(FALSE);
	Gfx_Clear(CT_INK);
	draw_sidebar_static();
	Clock_Init();
	Seg_Set(SEG_GFX);
	if (g_Game.stateInControl)
		Gfx_Blit(PAPER_COL, 0, g_Gfx_logo_small, GFX_LOGO_SMALL_W, GFX_LOGO_SMALL_H);
	else
		Gfx_Blit(PAPER_COL, 0, g_Gfx_logo_small2, GFX_LOGO_SMALL2_W, GFX_LOGO_SMALL2_H);
	Seg_Set(SEG_NEWS); // news table stays mapped for the whole play screen
	paper_draw_empty();
	// feed header: white "News Feed" on a black bar
	g_Strip.w = FEED_W;
	Cv_Clear(&g_Strip, 0);
	Cv_Text(&g_Strip, 4, 0, "News Feed", 9);
	Gfx_BlitCanvas(FEED_COL, 0, &g_Strip);
	Gfx_FillColor(FEED_COL, 0, FEED_W, 1, CT_INV);

	for (i = 0; i < MAX_ART; ++i)
		s_Art[i].used = 0;
	for (i = 0; i < MAX_DAY_ITEMS; ++i)
	{
		s_Shown[i] = 0;
		s_Placed[i] = 0xFF;
	}
	s_FeedCount = s_FeedTop = s_FeedSel = 0;
	s_Mode = MODE_FEED;
	s_SelSize = 0;
	s_FpW = 0;
	s_Time = 0;
	s_Speed10 = 0;
	s_AlarmDone = FALSE;
	s_Over = FALSE;
	s_TickInc = Sys_Is60Hz() ? 10 : 12; // 1/600 s units per frame
	s_LastFrame = g_Frames;
	Day_Generate(g_Game.day);

	feed_draw();
	panel_draw();
	end_button_draw(TRUE);
	Gfx_Display(TRUE);
	Music_Play(MUSIC_NONE);
	Ui_WaitRelease();

	for (;;)
	{
		Ui_Frame();

		// --- time (day 1 runs at half speed, End Day at 10x, as in PlayState)
		u16 prev = s_Time;
		// elapsed VBlanks since the previous pass (the main loop may take several frames to redraw)
		u8 df = g_Frames - s_LastFrame;
		s_LastFrame += df;
		if (df > 8)
			df = 8;
		u16 inc = df * (s_Speed10 ? s_TickInc * 10 : (g_Game.day == 1 ? s_TickInc / 2 : s_TickInc));
#ifdef DBG_FAST
		inc *= DBG_FAST;
#endif
		if (!s_Over)
			s_Time += inc;
		if (!s_AlarmDone && prev < (DAY_DURATION / 4) * 3 && s_Time >= (DAY_DURATION / 4) * 3)
		{
			s_AlarmDone = TRUE;
			Sfx_Play(SFX_ALARM);
		}
		if (s_Time > DAY_DURATION && !s_Over)
		{
			s_Over = TRUE;
			s_Time = DAY_DURATION;
			fp_hide();
			Sfx_Play(SFX_DAYOVER);
			popup_dayover();
		}
		Clock_Draw(0, 0, s_Time, s_Time >= (DAY_DURATION / 4) * 3);

		if (s_Over)
		{
			if (g_Push & IN_A)
				break;
			continue;
		}

		// --- new blurbs
		for (i = 0; i < g_Day.count; ++i)
		{
			if (!s_Shown[i] && s_Time > g_Day.appear[i] && s_FeedCount < MAX_DAY_ITEMS)
			{
				if (feed_add(i))
					feed_draw();
				else
					feed_draw_entry(s_FeedCount - 1);
				feed_colors();
				if (s_Mode == MODE_FEED)
				{
					panel_draw();
					end_button_draw(s_FeedSel >= s_FeedCount);
				}
				Sfx_Play(SFX_FEED);
			}
		}

		// --- input
		switch (s_Mode)
		{
		case MODE_FEED:  handle_feed();  break;
		case MODE_PLACE: handle_place(); break;
		case MODE_PAPER: handle_paper(); break;
		}
	}
	Sfx_Play(SFX_CLICK);
	summarize();
	return ST_NIGHT;
}
