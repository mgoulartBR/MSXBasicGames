// GERADO por tools/convert_db.py - NAO EDITAR. Dados derivados do TVTower (c) 2002-2024 TVTower-Team,
// adaptados/alterados para MSX (ver LICENSES.md).

#pragma once
#include "../game_types.h"

#define DB_NUM_CATEGORIES 8
#define DB_NUM_MOVIES 88
#define DB_NUM_ADS 24
#define DB_NUM_NEWS 24

extern const char* const g_CategoryName[DB_NUM_CATEGORIES];
extern const Movie g_Movies[DB_NUM_MOVIES];
extern const Ad g_Ads[DB_NUM_ADS];
extern const News g_News[DB_NUM_NEWS];
