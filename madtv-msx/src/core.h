// Tipos de dados compartilhados (tabelas geradas + jogo)
#pragma once
#include "msxgl.h"

typedef struct {
	const char* title;
	u8 cat;       // indice de categoria Mad TV
	u8 year;      // ano - 1900
	u8 blocks;    // blocos de programa (1..5)
	u8 critics;   // 0..100
	u8 speed;     // 0..100
	u8 outcome;   // 0..100 (bilheteria)
	u8 price;     // price_mod * 100
	u8 fsk18;     // 1 = so apos 21h
} Movie;

typedef struct {
	const char* title;
	u8 min_audience; // milhoes * 10
	u8 reps;
	u8 days;
	u16 profit;
	u16 penalty;
} Ad;

typedef struct {
	const char* title;
	u8 genre;
	u8 price;     // preco * 10
} News;
