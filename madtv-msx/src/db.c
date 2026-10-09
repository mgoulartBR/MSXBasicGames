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
