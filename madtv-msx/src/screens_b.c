// Telas: Sala de Noticias e Arquivo (venda de filmes). Compilado no segmento 7 (banco 2).
#include "app.h"

// ---------------------------------------------------------------- SALA DE NOTICIAS
// linhas do cursor: 0 = agencias, 1..3 = telejornal, 4..11 = noticias recebidas
#define NEWS_ROWS (1 + NEWS_SLATE + NEWS_POOL)
static u8 s_NewsCol;        // agencia sob o cursor na linha 0

#if ART_SCENES
// cena do estudio a esquerda; telejornal (3) + recebidas (8) num painel a direita; agencias e detalhes no painel de baixo
static u8 NewsRowY(u8 r) { return r == 0 ? 141 : (u8)(CONTENT_Y + (r - 1) * ROW_H); }

static const NewsItem* NewsIt(u8 r) { return r <= NEWS_SLATE ? &g_Game.news_slate[r - 1] : &g_Game.news_pool[r - 1 - NEWS_SLATE]; }

static void NewsPanelBody(void)
{
	u8 i, x, q = Sim_NewsQuality();
	Chrome_Box(0, 138, 256, 48, 14, 1);
	for (i = 0; i < DB_NUM_AGENCIES; i++)
	{
		if (g_Sel == 0 && s_NewsCol == i) Chrome_Box((u8)(4 + i * 84), 140, 80, 11, 5, 0);
		Ui_Color(UI_BLACK);
		Ui_Text((u8)(8 + i * 84), 142, g_Game.news_sub[i] ? "[x]" : "[ ]");
		Ui_Text((u8)(30 + i * 84), 142, g_AgencyName[i]);
	}
	Ui_Color(UI_BLACK);
	if (g_Sel > 0)
	{
		const NewsItem* it = NewsIt(g_Sel);
		if (it->idx != NONE)
		{
			NewsRec rec;
			Db_News(it->idx, &rec);
			Ui_TextN(8, 155, rec.title, 40);
			if (g_Sel > NEWS_SLATE) { Ui_Text(8, 166, "Price"); Money(44, 166, Sim_NewsCost(it->idx)); }
			Ui_Text(110, 166, "Fresh"); x = Ui_Int(146, 166, Sim_NewsFresh(it->age)); Ui_Text(x, 166, "%");
		}
	}
	else Ui_Text(8, 155, "Agencies sell you news every day.");
	Ui_Text(8, 176, "Newscast quality"); x = Ui_Int(110, 176, q); Ui_Text(x, 176, "%");
	Ui_Text(150, 176, "audience x"); x = Ui_Int(212, 176, 70 + (u16)q * 30 / 100); Ui_Text(x, 176, "%");
}

static void NewsRowBody(u8 r)
{
	u8 y = NewsRowY(r), i;
	NewsRec rec;
	const NewsItem* it;
	if (r == 0) return;
	Ui_Fill(126, y - 1, 128, ROW_H, 14);
	if (g_Sel == r) Chrome_Box(126, y - 1, 128, ROW_H, 5, 0);
	Ui_Color(UI_BLACK);
	it = NewsIt(r);
	i = (r <= NEWS_SLATE);
	if (it->idx == NONE) { if (i) Ui_Text(130, y, "- empty -"); return; }
	Db_News(it->idx, &rec);
	Ui_TextN(130, y, rec.title, i ? 14 : 20);
	if (i) { Ui_Int(220, y, Sim_NewsFresh(it->age)); Ui_Text(238, y, "%"); }
}

static void NewsRow(u8 r)
{
	if (r >= NEWS_ROWS) return;
	if (r)
	{
		Ui_Begin();
		NewsRowBody(r);
		Ui_End(126, (u8)(NewsRowY(r) - 1), 128, ROW_H);
	}
	if (r == 0 || g_Sel == r)
	{
		Ui_Begin();
		NewsPanelBody();
		Ui_End(0, 138, 256, 48);
	}
}

void News_Enter(void) __banked { s_NewsCol = 0; }

void News_Draw(void) __banked
{
	u8 r;
	Ui_Begin();
	Room_Frame(SCN_NEWS);
	for (r = 1; r < NEWS_ROWS; r++) NewsRowBody(r);
	NewsPanelBody();
	Ui_End(0, CONTENT_Y - 2, 0, 160);
	Hint("OK:use/toggle  BACK:office");
}

void News_Detail(void) __banked { Ui_Begin(); NewsPanelBody(); Ui_End(0, 138, 256, 48); }
#else
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

void News_Detail(void) __banked
{
	Ui_Begin();
	NewsHeaderBody();
	Ui_End(0, CONTENT_Y + 22, 255, 11);
}
#endif

void News_Row(u8 r) __banked { NewsRow(r); }
void News_Dyn(void) __banked { News_Draw(); }

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
static u8 s_Own[NUM_ALL];
static u8 s_OwnN;
#if ART_SCENES
#define ARCH_ROWS 9
#else
#define ARCH_ROWS LIST_ROWS
#endif

static void BuildOwn(void)
{
	u8 i;
	s_OwnN = 0;
	for (i = 0; i < NUM_ALL; i++) if (g_Game.owned[i]) s_Own[s_OwnN++] = i;
}

#if ART_SCENES
#define AR_Y(i) ((u8)(CONTENT_Y + 14 + (i) * ROW_H))      // lista no painel cinza a direita da cena; detalhes embaixo
#define AR_ROWS 9
static void ArchiveRowBody(u8 i)
{
	u8 y = AR_Y(i);
	Ui_Fill(126, y - 1, 128, ROW_H, 14);
	if (g_First + i >= s_OwnN) return;
	if (g_First + i == g_Sel) Chrome_Box(126, y - 1, 128, ROW_H, 5, 0);
	Ui_Color(UI_BLACK);
	Ui_TextN(130, y, (*Mov(s_Own[g_First + i])).title, 20);
}

static void ArchiveDetailBody(void)
{
	Chrome_Box(0, 138, 256, 48, 14, 1);
	Ui_Color(UI_BLACK);
	if (!s_OwnN) { Ui_Text(8, 142, "No movies - buy them at the Film agency."); return; }
	{
		u8 m = s_Own[g_Sel], x;
		Ui_TextN(8, 141, (*Mov(m)).title, 30);
		Ui_Text(8, 152, "Blocks"); Ui_Int(50, 152, (*Mov(m)).blocks);
		Ui_Text(80, 152, "Aired"); x = Ui_Int(116, 152, g_Game.plays[0][m]); Ui_Text(x, 152, "x");
		Ui_Text(150, 152, "Stars"); Ui_Int(186, 152, Db_Stars(m));
		Ui_Text(8, 162, "Resale value"); Money(86, 162, Sim_MovieValue(m));
		Chrome_CastLines(m);
	}
}

static void ArchiveRow(u8 i)
{
	if (i >= AR_ROWS) return;
	Ui_Begin();
	ArchiveRowBody(i);
	Ui_End(126, (u8)(AR_Y(i) - 1), 128, ROW_H);
	if (g_First + i == g_Sel) { Ui_Begin(); ArchiveDetailBody(); Ui_End(0, 138, 256, 48); }
}

void Archive_Enter(void) __banked { BuildOwn(); }

void Archive_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
	Room_Frame(SCN_LIBRARY);
	Ui_Color(UI_BLACK); Ui_Text(130, CONTENT_Y + 2, "Your archive");
	for (i = 0; i < AR_ROWS; i++) ArchiveRowBody(i);
	ArchiveDetailBody();
	Ui_End(0, CONTENT_Y - 2, 0, 160);
	Hint("OK:sell BACK:office");
}

void Archive_List(void) __banked { u8 i; for (i = 0; i < AR_ROWS; i++) ArchiveRow(i); }
#else
static void ArchiveRowBody(u8 i)
{
	u8 y = CONTENT_Y + 14 + i * ROW_H, m;
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	if (g_First + i >= s_OwnN) return;
	m = s_Own[g_First + i];
	Ui_Color(g_First + i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, g_First + i == g_Sel ? ">" : " ");
	Ui_TextN(14, y, (*Mov(m)).title, 22);
	Ui_Color(UI_GRAY);
	Ui_Int(160, y, (*Mov(m)).blocks); Ui_Text(168, y, "bl");
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
	if (!s_OwnN) { Ui_Color(UI_GRAY); Ui_Text(14, (u8)(CONTENT_Y + 14), "(no movies - buy at the Film agency)"); }
	for (i = 0; i < LIST_ROWS; i++) ArchiveRowBody(i);
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:sell BACK:office");
}
void Archive_List(void) __banked { u8 i; for (i = 0; i < LIST_ROWS; i++) ArchiveRow(i); }
#endif

void Archive_Row(u8 r) __banked { ArchiveRow(r); }

void Archive_Input(u8 ev) __banked
{
	NavApply(Nav(ev, s_OwnN, ARCH_ROWS));
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if ((ev & IN_OK) && s_OwnN)
	{
		u8 m = s_Own[g_Sel], r;
		u16 v = Sim_MovieValue(m);
		r = Sim_Sell(m);
		if (r == 0) { Sim_OwnFree(m); Sim_MsgNum("Sold for $", v, "k."); }
		else Sim_Msg("It is in today's programme grid!");
		BuildOwn();
		if (g_Sel >= s_OwnN && g_Sel) g_Sel--;
		if (g_First > g_Sel) g_First = g_Sel;
		g_Dirty |= D_CON | D_MSG | D_HDR;
	}
}

// ---------------------------------------------------------------- mouse
void News_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r, hit = 0xFF;
	for (r = 0; r < NEWS_ROWS; r++) { u8 ry = NewsRowY(r); if (y >= ry - 1 && y < ry + 9) { hit = r; break; } }
#if ART_SCENES
	if (hit != 0xFF && hit > 0 && x < 122) return;
#endif
	if (hit == 0xFF) return;
	if (hit == 0)
	{
		u8 col = (x < 86) ? 0 : (x < 170) ? 1 : 2;
		if (col != s_NewsCol) { s_NewsCol = col; MarkRows(0, 0); }
	}
	if (hit != g_Sel) { u8 old = g_Sel; g_Sel = hit; MarkRows(old, hit); }
	if (btn) News_Input(IN_OK);
}

void Archive_Mouse(u8 x, u8 y, u8 btn) __banked
{
#if ART_SCENES
	u8 r = HitRow(y, (u8)(AR_Y(0) - 1), ROW_H, AR_ROWS);
	if (x < 122) return;
#else
	u8 r = HitRow(y, (u8)(CONTENT_Y + 13), ROW_H, LIST_ROWS);
	(void)x;
#endif
	if (r == 0xFF || !SelVisible(r, s_OwnN)) return;
	if (btn) Archive_Input(IN_OK);
}
