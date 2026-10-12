// Acessores de dados em segmentos do mapper. Regra: o banco 3 (A000h) fica mapeado no segmento 3 (catalogo de filmes/anuncios);
// dados em outros segmentos so sao lidos aqui dentro, copiados para RAM, e o banco 3 volta ao segmento 3.
#include "msxgl.h"
#include "data/db_data.h"

#define SEG_CATALOG 3
#define SEG_NEWS    4

void Db_News(u8 idx, NewsRec* out)
{
	u16 prev = GET_BANK_SEGMENT(3);
	const u8* src;
	u8* dst = (u8*)out;
	u8 i;
	SET_BANK_SEGMENT(3, SEG_NEWS);
	src = (const u8*)&g_NewsRec[idx];
	for (i = 0; i < sizeof(NewsRec); i++) dst[i] = src[i];
	SET_BANK_SEGMENT(3, prev);
}

u8 Db_Stars(u8 m)                                     // soma das estrelas do elenco (0..9); filmes proprios = 0
{
	u16 prev;
	u8 v;
	if (m >= DB_NUM_MOVIES) return 0;
	prev = GET_BANK_SEGMENT(3);
	SET_BANK_SEGMENT(3, SEG_NEWS);
	v = g_Cast[m][3];
	SET_BANK_SEGMENT(3, prev);
	return v;
}

static void CopyName(char* dst, u8 p)
{
	const char* src = g_Person[p].name;
	u8 i;
	for (i = 0; i < DB_NAME_LEN; i++) dst[i] = src[i];
}

void Db_Cast(u8 m, CastRec* out)                      // diretor, 2 atores e estrelas copiados para RAM (filme proprio: nomes vazios)
{
	u16 prev;
	if (m >= DB_NUM_MOVIES) { out->dir[0] = out->a1[0] = out->a2[0] = 0; out->stars = 0; return; }
	prev = GET_BANK_SEGMENT(3);
	SET_BANK_SEGMENT(3, SEG_NEWS);
	CopyName(out->dir, g_Cast[m][0]); CopyName(out->a1, g_Cast[m][1]); CopyName(out->a2, g_Cast[m][2]);
	out->stars = g_Cast[m][3];
	SET_BANK_SEGMENT(3, prev);
}
