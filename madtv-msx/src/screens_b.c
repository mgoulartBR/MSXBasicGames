// Telas: Sala de Noticias e Arquivo (venda de filmes). Compilado no segmento 7 (banco 2).
#include "app.h"

// ---------------------------------------------------------------- SALA DE NOTICIAS
// linhas do cursor: 0 = agencias, 1..3 = telejornal, 4..11 = noticias recebidas
#define NEWS_ROWS (1 + NEWS_SLATE + NEWS_POOL)
static u8 s_NewsCol;        // agencia sob o cursor na linha 0

static u8 NewsRowY(u8 r)
{
	if (r == 0) return CONTENT_Y + 11;
	if (r <= NEWS_SLATE) return (u8)(CONTENT_Y + 35 + (r - 1) * ROW_H);
	return (u8)(CONTENT_Y + 78 + (r - 1 - NEWS_SLATE) * ROW_H);
}

static void NewsRowBody(u8 r)
{
	u8 y = NewsRowY(r), sel = (g_Sel == r), i;
	NewsRec rec;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (r == 0)
	{
		for (i = 0; i < DB_NUM_AGENCIES; i++)
		{
			Ui_Color(sel && s_NewsCol == i ? UI_YELLOW : (g_Game.news_sub[i] ? UI_GREEN : UI_GRAY));
			Ui_Text((u8)(4 + i * 84), y, g_Game.news_sub[i] ? "[x]" : "[ ]");
			Ui_Text((u8)(26 + i * 84), y, g_AgencyName[i]);
		}
		if (sel) { Ui_Color(UI_YELLOW); Ui_Text(0, y, ">"); }
		return;
	}
	Ui_Color(sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(0, y, sel ? ">" : " ");
	if (r <= NEWS_SLATE)
	{
		const NewsItem* it = &g_Game.news_slate[r - 1];
		if (it->idx == NONE) { Ui_Color(sel ? UI_YELLOW : UI_GRAY); Ui_Text(8, y, "- empty -"); return; }
		Db_News(it->idx, &rec);
		Ui_TextN(8, y, rec.title, 27);
		Ui_Color(UI_GRAY);
		Ui_Int(214, y, Sim_NewsFresh(it->age)); Ui_Text(232, y, "%");
	}
	else
	{
		const NewsItem* it = &g_Game.news_pool[r - 1 - NEWS_SLATE];
		if (it->idx == NONE) return;
		Db_News(it->idx, &rec);
		Ui_TextN(8, y, rec.title, 27);
		Ui_Color(UI_GRAY);
		Money(176, y, Sim_NewsCost(it->idx));
		Ui_Int(214, y, Sim_NewsFresh(it->age)); Ui_Text(232, y, "%");
	}
}

static void NewsRow(u8 r)
{
	if (r >= NEWS_ROWS) return;
	Ui_Begin();
	NewsRowBody(r);
	Ui_End(0, (u8)(NewsRowY(r) - 1), 255, ROW_H);
}

static void NewsHeaderBody(void)
{
	Ui_Fill(0, CONTENT_Y + 22, 255, 11, UI_BG);
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 24), "Newscast");
	{
		u8 q = Sim_NewsQuality(), x;
		Ui_Color(UI_GRAY);   Ui_Text(60, (u8)(CONTENT_Y + 24), "quality");
		x = Ui_Int(108, (u8)(CONTENT_Y + 24), q); Ui_Text(x, (u8)(CONTENT_Y + 24), "%");
		Ui_Text(150, (u8)(CONTENT_Y + 24), "audience x");
		x = Ui_Int(214, (u8)(CONTENT_Y + 24), 70 + (u16)q * 30 / 100); Ui_Text(x, (u8)(CONTENT_Y + 24), "%");
	}
}

void News_Enter(void) __banked { s_NewsCol = 0; }

void News_Draw(void) __banked
{
	u8 r;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "News room");
	NewsHeaderBody();
	Ui_Color(UI_YELLOW); Ui_Text(4, (u8)(CONTENT_Y + 67), "Incoming (price, freshness)");
	for (r = 0; r < NEWS_ROWS; r++) NewsRowBody(r);
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:use/toggle  BACK:office");
}

void News_Row(u8 r) __banked { NewsRow(r); }
void News_Dyn(void) __banked { News_Draw(); }

void News_Detail(void) __banked
{
	Ui_Begin();
	NewsHeaderBody();
	Ui_End(0, CONTENT_Y + 22, 255, 11);
}

void News_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel + 1 < NEWS_ROWS) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) MarkRows(old, g_Sel);
	if (g_Sel == 0 && (ev & (IN_LEFT | IN_RIGHT)))
	{
		s_NewsCol = (ev & IN_RIGHT) ? (u8)((s_NewsCol + 1) % DB_NUM_AGENCIES) : (u8)((s_NewsCol + DB_NUM_AGENCIES - 1) % DB_NUM_AGENCIES);
		MarkRows(0, 0);
	}
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK)
	{
		if (g_Sel == 0) { Sim_NewsToggle(s_NewsCol); Sim_MsgNum("Agency fee: $", NEWS_FEE, "k/day each."); }
		else if (g_Sel <= NEWS_SLATE) { Sim_NewsClear((u8)(g_Sel - 1)); Sim_Msg("Item removed from the newscast."); }
		else
		{
			u8 r = Sim_NewsPick((u8)(g_Sel - 1 - NEWS_SLATE));
			Sim_Msg(r == 0 ? "News bought - in your newscast." : r == 1 ? "Not enough money!" : "Nothing there.");
		}
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

// ---------------------------------------------------------------- ARQUIVO (venda de filmes)
static u8 s_Own[DB_NUM_MOVIES];
static u8 s_OwnN;

static void BuildOwn(void)
{
	u8 i;
	s_OwnN = 0;
	for (i = 0; i < DB_NUM_MOVIES; i++) if (g_Game.owned[i]) s_Own[s_OwnN++] = i;
}

static void ArchiveRowBody(u8 i)
{
	u8 y = CONTENT_Y + 14 + i * ROW_H, m;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (g_First + i >= s_OwnN) return;
	m = s_Own[g_First + i];
	Ui_Color(g_First + i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, g_First + i == g_Sel ? ">" : " ");
	Ui_TextN(14, y, g_Movies[m].title, 22);
	Ui_Color(UI_GRAY);
	Ui_Int(160, y, g_Movies[m].blocks); Ui_Text(168, y, "bl");
	Ui_Text(186, y, "x"); Ui_Int(194, y, g_Game.plays[0][m]);
	Money(212, y, Sim_MovieValue(m));
}

static void ArchiveRow(u8 i)
{
	if (i >= LIST_ROWS) return;
	Ui_Begin();
	ArchiveRowBody(i);
	Ui_End(0, (u8)(CONTENT_Y + 14 + i * ROW_H - 1), 255, ROW_H);
}

void Archive_Enter(void) __banked { BuildOwn(); }

void Archive_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Archive");
	Ui_Color(UI_GRAY);   Ui_Text(60, CONTENT_Y, "plays / resale value");
	if (!s_OwnN) { Ui_Color(UI_GRAY); Ui_Text(14, (u8)(CONTENT_Y + 14), "(no movies - buy some at the Film agency)"); }
	for (i = 0; i < LIST_ROWS; i++) ArchiveRowBody(i);
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:sell BACK:office");
}

void Archive_Row(u8 r) __banked { ArchiveRow(r); }
void Archive_List(void) __banked { u8 i; for (i = 0; i < LIST_ROWS; i++) ArchiveRow(i); }

void Archive_Input(u8 ev) __banked
{
	NavApply(Nav(ev, s_OwnN, LIST_ROWS));
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if ((ev & IN_OK) && s_OwnN)
	{
		u8 m = s_Own[g_Sel], r;
		u16 v = Sim_MovieValue(m);
		r = Sim_Sell(m);
		if (r == 0) { Sim_MsgNum("Sold for $", v, "k."); }
		else Sim_Msg("It is in today's programme grid!");
		BuildOwn();
		if (g_Sel >= s_OwnN && g_Sel) g_Sel--;
		if (g_First > g_Sel) g_First = g_Sel;
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}
