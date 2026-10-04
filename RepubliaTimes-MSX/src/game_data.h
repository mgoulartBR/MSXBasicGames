// Shared data declarations (generated tables live in src/data/*.c)
#pragma once
#include "msxgl.h"

typedef struct
{
	i8 dayStart;
	i8 dayEnd;
	i8 loyalty;       // +1 / 0 / -1
	u8 interesting;
	const char* blurb;
	const char* head; // 0 = no article text (rebel messages)
} NewsDef;

#define NEWS_COUNT 71
extern const NewsDef g_News[NEWS_COUNT];

// Font record: [advance, row0..row7] for chars 32..126
extern const unsigned char g_Font_Silk[];
