#include "logic.h"

Game g_Game;
DayPlan g_Day;

static u8 s_Order[NEWS_COUNT];
static u8 s_Used[NEWS_COUNT];

//-----------------------------------------------------------------------------
// RNG (16-bit xorshift)
static u16 s_Rng = 0xACE1;

void Rand_Seed(u16 seed) { s_Rng = seed ? seed : 0xACE1; }

u16 Rand16(void)
{
	s_Rng ^= s_Rng << 7;
	s_Rng ^= s_Rng >> 9;
	s_Rng ^= s_Rng << 8;
	return s_Rng;
}

u8 Rand8(u8 n) { return (u8)(((u32)Rand16() * n) >> 16); }

static void shuffle(u8* a, u8 n)
{
	// same algorithm as Util.shuffleArray: swap each slot with a random one
	for (u8 i = 0; i < n; ++i)
	{
		u8 j = Rand8(n);
		u8 t = a[i];
		a[i] = a[j];
		a[j] = t;
	}
}

//-----------------------------------------------------------------------------
// Goals (Goal.allGoals): id, targetDay, targetLoyalty, targetReaders
typedef struct { u8 day; i8 loyalty; u16 readers; } GoalDef;
static const GoalDef s_Goals[GOAL_COUNT] = {
	{ 3, 20, 0 },      // first-state
	{ 5, 20, 400 },    // second-state
	{ 10, -30, 1000 }, // last-rebel
};

u8 Goal_TargetDay(u8 g) { return s_Goals[g].day; }
i8 Goal_TargetLoyalty(u8 g) { return s_Goals[g].loyalty; }
u16 Goal_TargetReaders(u8 g) { return s_Goals[g].readers; }

u8 Goal_ForDay(u8 day)
{
	for (u8 i = 0; i < GOAL_COUNT; ++i)
		if (s_Goals[i].day >= day)
			return i;
	return GOAL_NONE;
}

bool Goal_IsMet(u8 g)
{
	const GoalDef* d = &s_Goals[g];
	if (d->loyalty < 0 && g_Game.loyalty > d->loyalty)
		return FALSE;
	if (d->loyalty > 0 && g_Game.loyalty < d->loyalty)
		return FALSE;
	if (d->readers && g_Game.readers < (i16)d->readers)
		return FALSE;
	return TRUE;
}

i8 Game_LoyaltyDelta(void) { return g_Game.loyalty - g_Game.preLoyalty; }
i16 Game_ReadersDelta(void) { return g_Game.readers - g_Game.preReaders; }

u8 Goal_Status(u8 g)
{
	const GoalDef* d = &s_Goals[g];
	if (Goal_IsMet(g))
		return GS_MET;
	if (d->loyalty < 0 && g_Game.loyalty > d->loyalty)
		return Game_LoyaltyDelta() < 0 ? GS_WORKING : GS_NOT_WORKING;
	if (d->loyalty > 0 && g_Game.loyalty < d->loyalty)
		return Game_LoyaltyDelta() > 0 ? GS_WORKING : GS_NOT_WORKING;
	if (d->readers && g_Game.readers < (i16)d->readers)
		return Game_ReadersDelta() > 0 ? GS_WORKING : GS_NOT_WORKING;
	return GS_NONE;
}

//-----------------------------------------------------------------------------
void Game_Reset(void)
{
	g_Game.day = 1;
	g_Game.readers = READERS_START;
	g_Game.preReaders = READERS_START;
	g_Game.loyalty = 0;
	g_Game.preLoyalty = 0;
	g_Game.comments = 0;
	Mem_Set(0, s_Used, NEWS_COUNT);
	for (u8 i = 0; i < NEWS_COUNT; ++i)
		s_Order[i] = i;
	// Debug-only start state (never defined in release builds, see scripts/build.sh RT_DEFINES)
#ifdef DBG_START_DAY
	g_Game.day = DBG_START_DAY;
#endif
#ifdef DBG_LOYALTY
	g_Game.loyalty = g_Game.preLoyalty = DBG_LOYALTY;
#endif
#ifdef DBG_READERS
	g_Game.readers = g_Game.preReaders = DBG_READERS;
#endif
}

//-----------------------------------------------------------------------------
// Text helpers
void Str_Expand(char* dst, const char* src)
{
	const char* gov = g_Game.stateInControl ? "Republia" : "Democria";
	while (*src)
	{
		if (src[0] == '[' && src[1] == 'G' && src[2] == 'O' && src[3] == 'V' && src[4] == ']')
		{
			const char* g = gov;
			while (*g)
				*dst++ = *g++;
			src += 5;
		}
		else
			*dst++ = *src++;
	}
	*dst = 0;
}

bool News_IsWeather(u8 i)
{
	const char* b = g_News[i].blurb;
	return b[0] == 'W' && b[1] == 'e' && b[2] == 'a' && b[3] == 't' && b[4] == 'h' && b[5] == 'e' && b[6] == 'r' && b[7] == ':';
}

bool News_IsRebel(u8 i) { return g_News[i].blurb[0] == '*'; }

void News_MarkUsed(u8 i) { s_Used[i] = 1; }

// Blurb text; "a|b|c" picks a region by current goal status (NotWorking|WorkingTowards|Met)
void News_Blurb(char* dst, u8 i)
{
	const char* src = g_News[i].blurb;
	const char* p = src;
	bool variant = FALSE;
	while (*p)
		if (*p++ == '|')
			variant = TRUE;
	if (variant)
	{
		u8 g = Goal_ForDay(g_Game.day);
		u8 st = (g == GOAL_NONE) ? GS_WORKING : Goal_Status(g);
		if (st == GS_NONE)
			st = GS_WORKING;
		u8 want = st - 1;
		// advance to the wanted token
		while (want)
		{
			while (*src && *src != '|')
				++src;
			if (*src == '|')
				++src;
			--want;
		}
		// copy until next '|'
		char tmp[112];
		char* t = tmp;
		while (*src && *src != '|')
			*t++ = *src++;
		*t = 0;
		Str_Expand(dst, tmp);
	}
	else
		Str_Expand(dst, src);
}

void News_Head(char* dst, u8 i)
{
	if (g_News[i].head)
		Str_Expand(dst, g_News[i].head);
	else
		dst[0] = 0;
}

//-----------------------------------------------------------------------------
// Day generation (port of Day.as)
void Day_Generate(u8 dayIndex)
{
	u8 crit[16];
	u8 nonc[NEWS_COUNT];
	u8 nCrit = 0, nNon = 0;
	u8 weather = 0xFF;
	bool rebel = FALSE;
	u8 i;

	shuffle(s_Order, NEWS_COUNT);

	for (i = 0; i < NEWS_COUNT; ++i)
	{
		u8 it = s_Order[i];
		const NewsDef* n = &g_News[it];
		if (s_Used[it])
			continue;
		if (News_IsWeather(it))
		{
			if (weather == 0xFF)
				weather = it;
			continue;
		}
		if (n->dayStart || n->dayEnd)
		{
			i8 di = (i8)dayIndex;
			if ((di >= n->dayStart && di <= n->dayEnd) || (di == n->dayStart && n->dayEnd < 0))
			{
				if (News_IsRebel(it))
					rebel = TRUE;
				if (nCrit < 16)
					crit[nCrit++] = it;
			}
		}
		else
			nonc[nNon++] = it;
	}

	// coalesce: critical, weather, non-critical
	u8 all[NEWS_COUNT + 17];
	u8 total = 0;
	for (i = 0; i < nCrit; ++i)
		all[total++] = crit[i];
	if (weather != 0xFF)
		all[total++] = weather;
	for (i = 0; i < nNon; ++i)
		all[total++] = nonc[i];

	// first 7..9 entries (8..9 if a rebel message is present)
	u8 rnd = rebel ? 2 : 3;
	u8 num = (MAX_DAY_ITEMS - rnd) + Rand8(rnd);
	if (num > total)
		num = total;
	shuffle(all, num);

	g_Day.count = num;
	for (i = 0; i < num; ++i)
	{
		g_Day.item[i] = all[i];
		// appearTime = rand * (0.75 * i / n) * dayDuration  (halved on day 1)
		u32 t = (u32)27000 * i / num;
		if (dayIndex == 1)
			t >>= 1;
		g_Day.appear[i] = (u16)(((u32)Rand16() * t) >> 16);
	}
}

//-----------------------------------------------------------------------------
// Paper summary / readership
void Summary_Init(PaperSummary* s)
{
	s->interesting = 0;
	s->articles = 0;
	s->coverage19 = 0;
	s->loyalty = 0;
}

void Summary_Add(PaperSummary* s, u8 item, u8 size)
{
	static const u8 mult[3] = { 1, 3, 6 };
	static const u8 area[3] = { 2, 4, 9 };
	s->articles++;
	if (g_News[item].interesting)
		s->interesting++;
	s->loyalty += (i16)g_News[item].loyalty * mult[size];
	s->coverage19 += area[size];
}

static i16 floor_div(i16 a, i16 b)
{
	i16 q = a / b;
	if ((a % b) < 0)
		--q;
	return q;
}

static i16 bonus10(i16 readers)
{
	// getReadershipBonus() * 10
	return 10 + floor_div(readers - READERS_START, READERS_BONUS_STEP);
}

void Readership_Apply(const PaperSummary* s)
{
	g_Game.preLoyalty = g_Game.loyalty;
	g_Game.preReaders = g_Game.readers;
	g_Game.comments = 0;

	// curLoyalty += effect * bonus (Number), then stored as int (truncation)
	i32 sum = (i32)g_Game.loyalty * 10 + (i32)s->loyalty * bonus10(g_Game.preReaders);
	i32 l = sum / 10;
	if (l > STAT_MAX) l = STAT_MAX;
	if (l < -STAT_MAX) l = -STAT_MAX;
	g_Game.loyalty = (i8)l;

	i32 r = g_Game.readers;
	if (s->coverage19 == 0)
	{
		g_Game.comments |= CMT_BLANK;
		r = r / 2;
	}
	else if (s->coverage19 * 4 < 3 * 19) // coverage < 0.75
	{
		g_Game.comments |= CMT_TOO_FEW;
		r = r * 3 / 4;
	}
	if (s->interesting < 2)
	{
		g_Game.comments |= CMT_FEW_INTEREST;
		r = r * 9 / 10;
	}
	else if (s->interesting > 2)
	{
		g_Game.comments |= CMT_MANY_INTEREST;
		r = r * 5 / 4;
	}
	if (r > 30000) r = 30000;
	g_Game.readers = (i16)r;

	if (g_Game.loyalty > g_Game.preLoyalty)
		g_Game.comments |= CMT_LOYALTY_UP;
	if (g_Game.loyalty < g_Game.preLoyalty)
		g_Game.comments |= CMT_LOYALTY_DOWN;

	// NOTE: the original has a copy/paste bug in this branch (the "decreasing readership"
	// comment is unreachable because the same condition is tested twice). Kept as-is.
	if (bonus10(g_Game.readers) > bonus10(g_Game.preReaders))
		g_Game.comments |= CMT_INFLUENCE_UP;
}
