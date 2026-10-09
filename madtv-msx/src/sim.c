#include "sim.h"
#include "data/db_data.h"

Game g_Game;
#ifdef BALANCE_STATS
long g_StatDone, g_StatFail, g_StatIncome, g_StatPenalty, g_StatMissed, g_StatSpots;
#define STAT(x) (x)
#else
#define STAT(x) ((void)0)
#endif
char g_Msg[44];
const u8 g_Reach[NUM_STATIONS] = { 120, 110, 100 };
const char* const g_StationName[NUM_STATIONS] = { "MadTV", "FunTV", "SunTV" };

// parcela do alcance que esta com a TV ligada em cada hora (%)
static const u8 s_TimeShare[NUM_SLOTS] = { 40, 55, 75, 90, 85, 60, 30 };
// afinidade categoria x hora (%) - projeto proprio guiado pelo manual (cultura = "matador de audiencia", etc.)
static const u8 s_Fit[DB_NUM_CATEGORIES][NUM_SLOTS] = {
	{ 75, 90, 100, 90,  70, 50, 35 },  // Lovestory
	{ 50, 60,  80, 100, 100, 85, 60 }, // Action
	{ 40, 55,  85, 100, 90,  60, 35 }, // Monumental
	{ 70, 85,  95,  95, 85,  70, 50 }, // Comedy
	{ 40, 55,  80, 100, 100, 90, 65 }, // Crime
	{ 30, 30,  35,  40, 40,  35, 25 }, // Culture
	{ 50, 65,  85, 100, 95,  85, 60 }, // SciFi
	{ 55, 70,  85,  90, 80,  60, 40 }, // Other
};

static u16 s_Rng = 0xACE1;
static u8 Rnd(u8 n)               // xorshift16, 0..n-1
{
	s_Rng ^= s_Rng << 7; s_Rng ^= s_Rng >> 9; s_Rng ^= s_Rng << 8;
	return (u8)((s_Rng >> 4) % n);
}

void Sim_Msg(const char* s)
{
	u8 i = 0;
	while (s[i] && i < sizeof(g_Msg) - 1) { g_Msg[i] = s[i]; i++; }
	g_Msg[i] = 0;
}

static void AppendStr(char* dst, const char* s)
{
	u8 n = 0, i = 0;
	while (dst[n]) n++;
	while (s[i] && n < sizeof(g_Msg) - 1) dst[n++] = s[i++];
	dst[n] = 0;
}

static void AppendInt(char* dst, i32 v)
{
	u8 n = 0, i;
	char tmp[11];
	while (dst[n]) n++;
	if (v < 0) { dst[n++] = '-'; v = -v; }
	i = 0;
	do { tmp[i++] = '0' + (u8)(v % 10); v /= 10; } while (v);
	while (i) dst[n++] = tmp[--i];
	dst[n] = 0;
}

u8 Sim_Quality(u8 m)
{
	const Movie* mv = &g_Movies[m];
	return (u8)(((u16)mv->critics * 3 + (u16)mv->outcome * 4 + (u16)mv->speed * 3) / 10);
}

u16 Sim_MoviePrice(u8 idx)        // k$ (formula propria)
{
	u16 avg = (u16)(((u16)g_Movies[idx].critics + g_Movies[idx].speed + g_Movies[idx].outcome) / 3);
	return (u16)((avg * g_Movies[idx].price) / PRICE_DIV * g_Movies[idx].blocks / 2 + 50);
}

void Sim_MsgNum(const char* a, i32 v, const char* b)
{
	Sim_Msg(a); AppendInt(g_Msg, v); AppendStr(g_Msg, b);
}

// ---------------------------------------------------------------- audiencia
static u8 Audience(u8 st, u8 s)
{
	const Slot* sl = &g_Game.slot[st][s];
	u16 a = (u16)g_Reach[st] * s_TimeShare[s] / 100;
	if (sl->movie == NONE)
		a = a * 5 / 100;                                   // programa de teste
	else
	{
		const Movie* mv = &g_Movies[sl->movie];
		u8 plays = g_Game.plays[st][sl->movie];
		u8 wear = (plays >= 5) ? 40 : (u8)(100 - 12 * plays);
		a = a * Sim_Quality(sl->movie) / 100;
		a = a * s_Fit[mv->cat][s] / 100;
		a = a * wear / 100;
		if (mv->fsk18 && s < 3) a = a * 40 / 100;          // FSK18 antes das 21h
	}
	a = a * (90 + Rnd(21)) / 100;
	if (st) a = a * RIVAL_Q / 100;
	return (u8)a;
}

u8 Sim_Quota(u8 st, u8 s)
{
	return (u8)((u16)g_Game.aud[st][s] * 100 / g_Reach[st]);
}

// ---------------------------------------------------------------- grade
void Sim_ClearSlot(u8 s)
{
	Slot* sl = g_Game.slot[0];
	u8 start, n, i, m;
	if (sl[s].movie == NONE) return;
	m = sl[s].movie;
	start = s - sl[s].part;
	n = g_Movies[m].blocks;
	for (i = 0; i < n && start + i < NUM_SLOTS; i++) { sl[start + i].movie = NONE; sl[start + i].part = 0; }
}

u8 Sim_PlaceMovie(u8 s, u8 m)
{
	Slot* sl = g_Game.slot[0];
	u8 n = g_Movies[m].blocks, i;
	if (s + n > NUM_SLOTS) return 1;
	for (i = 0; i < n; i++) Sim_ClearSlot(s + i);
	for (i = 0; i < n; i++) { sl[s + i].movie = m; sl[s + i].part = i; }
	return 0;
}

void Sim_PlaceAd(u8 s, u8 c)
{
	g_Game.slot[0][s].ad = c;
}

// tamanho da biblioteca do rival: comeca pequeno e cresce com os dias (rivais tambem tem caixa limitado)
static u8 RivalLib(void)
{
	u16 n = RIVAL_LIB_START + g_Game.day / RIVAL_LIB_GROWTH_DAYS;
	return (u8)(n > DB_NUM_MOVIES ? DB_NUM_MOVIES : n);
}

static void AiSchedule(u8 st)
{
	Slot* sl = g_Game.slot[st];
	u8 s = 0, k, best, bestv, m, v, tries;
	for (k = 0; k < NUM_SLOTS; k++) { sl[k].movie = NONE; sl[k].part = 0; sl[k].ad = NONE; }
	while (s < NUM_SLOTS)
	{
		best = NONE; bestv = 0;
		for (tries = 0; tries < 6; tries++)
		{
			m = g_Game.rival_lib[st - 1][Rnd(RivalLib())];     // so filmes da biblioteca do rival
			if (s + g_Movies[m].blocks > NUM_SLOTS) continue;
			v = (u8)((u16)Sim_Quality(m) * s_Fit[g_Movies[m].cat][s] / 100);
			if (g_Movies[m].fsk18 && s < 3) v = 0;
			v = (u8)(v * (100 - (g_Game.plays[st][m] > 4 ? 60 : g_Game.plays[st][m] * 12)) / 100);
			if (v >= bestv) { bestv = v; best = m; }
		}
		if (best == NONE) { s++; continue; }
		for (k = 0; k < g_Movies[best].blocks; k++) { sl[s + k].movie = best; sl[s + k].part = k; }
		s += g_Movies[best].blocks;
	}
}

// ---------------------------------------------------------------- dia
static void NewOffers(void)
{
	u8 i, j, a, dup;
	for (i = 0; i < NUM_OFFERS; i++)
	{
		do {
			a = Rnd(DB_NUM_ADS); dup = 0;
			for (j = 0; j < i; j++) if (g_Game.offer[j] == a) dup = 1;
			for (j = 0; j < MAX_CONTRACTS; j++) if (g_Game.contract[j].ad == a) dup = 1;
		} while (dup);
		g_Game.offer[i] = a;
	}
}

static void StartDay(void)
{
	u8 st, m;
	g_Game.t = 0;
	g_Game.day++;
	g_Game.day_income = 0; g_Game.day_cost = 0;
	for (st = 0; st < NUM_SLOTS; st++) g_Game.aud_done[st] = 0;
	for (st = 0; st < NUM_STATIONS; st++)
		for (m = 0; m < NUM_SLOTS; m++) g_Game.aud[st][m] = 0;
	AiSchedule(1); AiSchedule(2);
	NewOffers();
}

void Sim_Init(u16 seed)
{
	u16 n;
	u8 i, st, s;
	s_Rng = seed ? seed : 0xACE1;
	for (n = 0; n < sizeof(Game); n++) ((u8*)&g_Game)[n] = 0;
	g_Game.money = START_MONEY;
	g_Game.image[0] = 34; g_Game.image[1] = 33; g_Game.image[2] = 33;
	for (st = 0; st < NUM_STATIONS; st++)
		for (s = 0; s < NUM_SLOTS; s++) { g_Game.slot[st][s].movie = NONE; g_Game.slot[st][s].ad = NONE; }
	for (i = 0; i < MAX_CONTRACTS; i++) g_Game.contract[i].ad = NONE;
	for (st = 0; st < 2; st++)                 // biblioteca de cada rival = permutacao embaralhada do catalogo
	{
		for (i = 0; i < DB_NUM_MOVIES; i++) g_Game.rival_lib[st][i] = i;
		for (i = DB_NUM_MOVIES - 1; i > 0; i--) { u8 j = Rnd(i + 1), tmp = g_Game.rival_lib[st][i]; g_Game.rival_lib[st][i] = g_Game.rival_lib[st][j]; g_Game.rival_lib[st][j] = tmp; }
	}
	g_Game.day = 0;
	StartDay();
	Sim_Msg("Welcome, program director!");
}

u8 Sim_Buy(u8 idx)
{
	u16 p = Sim_MoviePrice(idx);
	if (g_Game.owned[idx]) return 1;
	if (g_Game.money < (i32)p) return 2;
	g_Game.money -= p; g_Game.day_cost += p;
	g_Game.owned[idx] = 1;
	return 0;
}

u8 Sim_SignAd(u8 o)
{
	u8 i, a = g_Game.offer[o];
	if (a == NONE) return 2;
	for (i = 0; i < MAX_CONTRACTS; i++)
		if (g_Game.contract[i].ad == NONE)
		{
			g_Game.contract[i].ad = a;
			g_Game.contract[i].reps_left = g_Ads[a].reps;
			g_Game.contract[i].days_left = g_Ads[a].days;
			g_Game.offer[o] = NONE;
			return 0;
		}
	return 1;
}

// fim do slot s (:55): mede audiencias, usa o anuncio, redistribui o Image
static void EndSlot(u8 s)
{
	u8 st, best = 0, worst = 0, c;
	u8 q[NUM_STATIONS];
	for (st = 0; st < NUM_STATIONS; st++)
	{
		g_Game.aud[st][s] = Audience(st, s);
		q[st] = Sim_Quota(st, s);
	}
	g_Game.aud_done[s] = 1;
	// anuncio do jogador
	c = g_Game.slot[0][s].ad;
	if (c != NONE && g_Game.contract[c].ad != NONE)
	{
		Contract* ct = &g_Game.contract[c];
		const Ad* ad = &g_Ads[ct->ad];
		if (g_Game.aud[0][s] >= ad->min_audience && ct->reps_left)
		{
			ct->reps_left--;
			STAT(g_StatSpots++);
			if (ct->reps_left == 0)
			{
				STAT(g_StatDone++); STAT(g_StatIncome += ad->profit);
				g_Game.money += ad->profit; g_Game.day_income += ad->profit;
				Sim_MsgNum("Contract done! +", ad->profit, "k");
				ct->ad = NONE;
			}
			else Sim_Msg("Spot aired OK");
		}
		else { Sim_Msg("Spot missed audience!"); STAT(g_StatMissed++); }
	}
	// Image: maior quota tira 1 ponto da menor
	for (st = 1; st < NUM_STATIONS; st++)
	{
		if (q[st] > q[best]) best = st;
		if (q[st] < q[worst]) worst = st;
	}
	if (best != worst && g_Game.image[worst] > 0) { g_Game.image[best]++; g_Game.image[worst]--; }
}

static void EndDay(void)
{
	u8 i, st, m;
	Contract* ct;
	// desgaste: filmes exibidos hoje contam 1 exibicao; os demais "descansam"
	for (st = 0; st < NUM_STATIONS; st++)
	{
		u8 shown[DB_NUM_MOVIES];
		for (m = 0; m < DB_NUM_MOVIES; m++) shown[m] = 0;
		for (i = 0; i < NUM_SLOTS; i++)
			if (g_Game.slot[st][i].movie != NONE && g_Game.slot[st][i].part == 0) shown[g_Game.slot[st][i].movie] = 1;
		for (m = 0; m < DB_NUM_MOVIES; m++)
		{
			if (shown[m]) { if (g_Game.plays[st][m] < 250) g_Game.plays[st][m]++; }
			else if (g_Game.plays[st][m] && Rnd(3) == 0) g_Game.plays[st][m]--;
		}
	}
	// contratos
	for (i = 0; i < MAX_CONTRACTS; i++)
	{
		ct = &g_Game.contract[i];
		if (ct->ad == NONE) continue;
		if (ct->days_left) ct->days_left--;
		if (ct->days_left == 0 && ct->reps_left)
		{
			STAT(g_StatFail++); STAT(g_StatPenalty += g_Ads[ct->ad].penalty);
			g_Game.money -= g_Ads[ct->ad].penalty; g_Game.day_cost += g_Ads[ct->ad].penalty;
			Sim_MsgNum("Contract failed! -", g_Ads[ct->ad].penalty, "k");
			ct->ad = NONE;
			for (st = 0; st < NUM_SLOTS; st++) if (g_Game.slot[0][st].ad == i) g_Game.slot[0][st].ad = NONE;
		}
	}
	g_Game.money -= DAILY_UPKEEP; g_Game.day_cost += DAILY_UPKEEP;
	if (g_Game.money < BANKRUPT_AT) g_Game.game_over = 1;
	StartDay();
}

u8 Sim_Tick(void)
{
	u8 ev = EV_NONE, s;
	u16 rel;
	if (g_Game.game_over) return EV_NONE;
	g_Game.t++;
	if (g_Game.t >= FIRST_SLOT_T)
	{
		rel = g_Game.t - FIRST_SLOT_T;           // minutos desde 18:00
		s = (u8)(rel / 60);
		if (s < NUM_SLOTS && (rel % 60) == 55) { EndSlot(s); ev |= EV_SLOT | EV_MSG; }
	}
	if (g_Game.t >= DAY_MINUTES) { EndDay(); ev |= EV_DAY | EV_MSG; }
	return ev;
}
