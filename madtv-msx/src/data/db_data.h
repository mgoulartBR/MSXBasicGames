// GERADO por tools/convert_db.py - NAO EDITAR. Dados derivados do TVTower (c) 2002-2024 TVTower-Team,
// adaptados/alterados para MSX (ver LICENSES.md).

#pragma once
#include "../game_types.h"

#define DB_NUM_CATEGORIES 8
#define DB_NUM_MOVIES 88
#define DB_NUM_ADS 24
#define DB_NUM_NEWS 115
#define DB_NEWS_TITLE 40
#define DB_NUM_AGENCIES 3

extern const char* const g_CategoryName[DB_NUM_CATEGORIES];
extern const Movie g_Movies[DB_NUM_MOVIES];
extern const Ad g_Ads[DB_NUM_ADS];
extern const char* const g_AgencyName[DB_NUM_AGENCIES];
extern const NewsRec g_NewsRec[DB_NUM_NEWS];     // SEGMENTO 4 (banco 3 so mapeado dentro de Db_News)
