// Platform-independent game rules, ported from the original Flixel/AS3 sources
// (GameStatus, Readership, Goal, Day, PaperSummary, NewsItem, MorningState texts).
#pragma once
#include "msxgl.h"
#include "../game_data.h"

// Time unit: 1/600 s (lcm of 50 and 60 Hz) so the day lasts 60 s on PAL and NTSC.
#define TIME_UNITS_PER_SEC 600
#define DAY_DURATION       36000   // Const.dayDuration = 60 s
#define STAT_MAX           30      // Const.statMax
#define READERS_START      200     // Const.readershipStartCount
#define READERS_BONUS_STEP 100     // Const.readershipBonusThresh
#define MAX_DAY_ITEMS      10

enum { GOAL_FIRST = 0, GOAL_SECOND, GOAL_LAST, GOAL_COUNT, GOAL_NONE = 0xFF };
enum { GS_NONE = 0, GS_NOT_WORKING, GS_WORKING, GS_MET };

// Article sizes (cells wide x high on the 4x5 paper grid): S=1x2, M=2x2, B=3x3
enum { SIZE_S = 0, SIZE_M = 1, SIZE_B = 2 };

// Readership comment flags (original produced strings; we render them from flags)
#define CMT_BLANK        0x01
#define CMT_TOO_FEW      0x02
#define CMT_FEW_INTEREST 0x04
#define CMT_MANY_INTEREST 0x08
#define CMT_LOYALTY_UP   0x10
#define CMT_LOYALTY_DOWN 0x20
#define CMT_INFLUENCE_UP 0x40

typedef struct
{
	u8 day;
	bool stateInControl;
	bool wonOnce;
	i16 readers;
	i16 preReaders;
	i8 loyalty;
	i8 preLoyalty;
	u8 comments;
} Game;

typedef struct
{
	u8 count;
	u8 item[MAX_DAY_ITEMS];
	u16 appear[MAX_DAY_ITEMS]; // 1/600 s units
} DayPlan;

typedef struct
{
	u8 interesting;
	u8 articles;
	u8 coverage19; // coverage in 1/19ths of the paper (maxArticleArea = 4*5-1)
	i16 loyalty;
} PaperSummary;

extern Game g_Game;
extern DayPlan g_Day;

// RNG
void Rand_Seed(u16 seed);
u16 Rand16(void);
u8 Rand8(u8 n); // 0..n-1

// Game / goals
void Game_Reset(void);
u8 Goal_ForDay(u8 day);                 // GOAL_NONE if beyond last goal
bool Goal_IsMet(u8 goal);
u8 Goal_Status(u8 goal);
i8 Goal_TargetLoyalty(u8 goal);
u16 Goal_TargetReaders(u8 goal);
u8 Goal_TargetDay(u8 goal);
i8 Game_LoyaltyDelta(void);
i16 Game_ReadersDelta(void);

// News
bool News_IsWeather(u8 i);
bool News_IsRebel(u8 i);
void News_Blurb(char* dst, u8 i);       // resolves "a|b|c" variants and [GOV]
void News_Head(char* dst, u8 i);
void News_MarkUsed(u8 i);
void Day_Generate(u8 dayIndex);

// Paper summary / readership
void Summary_Init(PaperSummary* s);
void Summary_Add(PaperSummary* s, u8 item, u8 size);
void Readership_Apply(const PaperSummary* s);

// Text builders
typedef struct
{
	bool gameOver;
	bool rebelsWon;
} MorningResult;

void Str_Expand(char* dst, const char* src);
// NOTE: Text_Morning/Text_Night live in ROM segment 6 (see src/data/seg_s6_b3.c):
// map it with Seg_Set(SEG_TEXT) around the call.
void Text_Morning(char* dst, MorningResult* res);
void Text_Night(char* dst);
