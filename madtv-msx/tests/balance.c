// Autoplay de balanceamento: joga N dias com varias politicas e imprime estatisticas.
// Compilar/rodar: tests/balance.sh   (usa a MESMA src/sim.c do jogo, compilada no PC)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"

static int g_Trace, g_Smart;
static u8 g_ExpAud[NUM_SLOTS];     // audiencia observada ontem por slot (0,1M); 0 no dia 1
static u8 g_SnapAud[NUM_STATIONS][NUM_SLOTS];
static Slot g_SnapSlot[NUM_STATIONS][NUM_SLOTS];
typedef enum { P_IDLE, P_NAIVE, P_CAREFUL, P_SMART } Policy;
static const char* const k_PolName[] = { "idle (nada)", "naive (assina tudo)", "careful (assina so o viavel)", "smart (usa audiencia real)" };
static const u8 k_Prime[NUM_SLOTS] = { 3, 2, 4, 1, 5, 0, 6 };

static void BuyBest(i32 reserve, int max_owned)
{
	for (;;)
	{
		int own = 0, k;
		for (k = 0; k < DB_NUM_MOVIES; k++) own += g_Game.owned[k];
		if (own >= max_owned) return;
		int best = -1, bestv = -1, i;
		for (i = 0; i < DB_NUM_MOVIES; i++)
		{
			u16 p;
			int v;
			if (g_Game.owned[i]) continue;
			p = Sim_MoviePrice(i);
			if (g_Game.money - (i32)p < reserve) continue;
			v = Sim_Quality(i) * 100 / (p + 50);          // qualidade por preco
			if (g_Movies[i].fsk18) v /= 2;
			if (v > bestv) { bestv = v; best = i; }
		}
		if (best < 0) return;
		Sim_Buy((u8)best);
		if (bestv < 8) return;
	}
}

static void FillGrid(void)
{
	int k, i;
	for (i = 0; i < NUM_SLOTS; i++) { Sim_ClearSlot(i); g_Game.slot[0][i].ad = NONE; }
	for (k = 0; k < NUM_SLOTS; k++)
	{
		int s = k_Prime[k], best = -1, bestq = -1;
		if (g_Game.slot[0][s].movie != NONE) continue;
		for (i = 0; i < DB_NUM_MOVIES; i++)
		{
			int used = 0, j, q;
			if (!g_Game.owned[i]) continue;
			for (j = 0; j < NUM_SLOTS; j++) if (g_Game.slot[0][j].movie == i) used = 1;
			if (used || (g_Movies[i].fsk18 && s < 3)) continue;
			if (s + g_Movies[i].blocks > NUM_SLOTS) continue;
			q = Sim_Quality(i) - g_Game.plays[0][i] * 8;
			if (q > bestq) { bestq = q; best = i; }
		}
		if (best >= 0) Sim_PlaceMovie(s, (u8)best);
	}
}

static void PlaceAds(void)
{
	int c, k;
	for (c = 0; c < MAX_CONTRACTS; c++)
	{
		int need;
		if (g_Game.contract[c].ad == NONE) continue;
		need = (g_Game.contract[c].reps_left + g_Game.contract[c].days_left - 1) / (g_Game.contract[c].days_left ? g_Game.contract[c].days_left : 1);
		for (k = 0; k < NUM_SLOTS && need > 0; k++)
		{
			int s = k_Prime[k];
			if (g_Game.slot[0][s].ad != NONE) continue;
			if (g_Smart && g_ExpAud[s] && g_ExpAud[s] < g_Ads[g_Game.contract[c].ad].min_audience * 105 / 100) continue;
			g_Game.slot[0][s].ad = (u8)c; need--;
		}
	}
}


// Estima, pela audiencia de ontem, em quantos slots por dia o contrato seria cumprido
static int FeasibleSlots(const Ad* a)
{
	int s, n = 0;
	for (s = 0; s < NUM_SLOTS; s++) if (g_ExpAud[s] >= a->min_audience * 115 / 100) n++;
	return n;
}

static void SignSmart(void)
{
	int o, c, committed = 0;
	for (c = 0; c < MAX_CONTRACTS; c++)
		if (g_Game.contract[c].ad != NONE) committed += (g_Game.contract[c].reps_left + g_Game.contract[c].days_left - 1) / (g_Game.contract[c].days_left ? g_Game.contract[c].days_left : 1);
	for (o = 0; o < NUM_OFFERS; o++)
	{
		const Ad* a;
		int per_day, free_ = 0, feasible;
		if (g_Game.offer[o] == NONE) continue;
		a = &g_Ads[g_Game.offer[o]];
		for (c = 0; c < MAX_CONTRACTS; c++) if (g_Game.contract[c].ad == NONE) free_++;
		if (!free_) return;
		feasible = FeasibleSlots(a);
		per_day = (a->reps + a->days - 1) / a->days;
		if (g_ExpAud[3] == 0) feasible = (a->min_audience <= 25) ? 3 : 0;      // dia 1: sem historico, aposta so em contratos leves
		if (per_day + committed > feasible) continue;
		if (a->profit < a->penalty / 3) continue;
		if (Sim_SignAd((u8)o) == 0) committed += per_day;
	}
}

static void SignAds(Policy pol)
{
	int o, c;
	if (pol == P_IDLE) return;
	if (pol == P_SMART) { SignSmart(); return; }
	for (o = 0; o < NUM_OFFERS; o++)
	{
		const Ad* a;
		int free_ = 0;
		if (g_Game.offer[o] == NONE) continue;
		a = &g_Ads[g_Game.offer[o]];
		for (c = 0; c < MAX_CONTRACTS; c++) if (g_Game.contract[c].ad == NONE) free_++;
		if (!free_) return;
		if (pol == P_CAREFUL)
		{
			// so assina se o filme de maior qualidade conhecido da chance de atingir a audiencia minima, e se cabem as exibicoes
			int bestq = 0, i;
			for (i = 0; i < DB_NUM_MOVIES; i++) if (g_Game.owned[i] && Sim_Quality(i) > bestq) bestq = Sim_Quality(i);
			if (bestq * 12 * 90 / 10000 * 10 / 10 < a->min_audience / 1) { /* aprox: audiencia ~ alcance*share*q */ }
			if ((int)a->min_audience > bestq * 120 * 90 / 10000 * 8 / 10) continue;
			if (a->reps > a->days * 3) continue;
		}
		Sim_SignAd((u8)o);
	}
}

typedef struct { int bankrupt, days; long money_end; int image_end; int contracts_done, contracts_failed; } Result;

static Result Play(Policy pol, u16 seed, int max_days)
{
	Result r;
	int day = 1, t;
	memset(&r, 0, sizeof r);
	memset(g_ExpAud, 0, sizeof g_ExpAud);
	Sim_Init(seed);
	while (!g_Game.game_over && g_Game.day <= max_days)
	{
		g_Smart = (pol == P_SMART);
		BuyBest(pol == P_IDLE ? 1000000 : (pol == P_SMART ? 1000 : 500), pol == P_SMART ? (RIVAL_LIB_START + (int)g_Game.day / RIVAL_LIB_GROWTH_DAYS + 2) : 99);
		SignAds(pol);
		FillGrid();
		PlaceAds();
		if (g_Trace)
		{
			int i, own = 0, nc = 0, sl = 0;
			for (i = 0; i < DB_NUM_MOVIES; i++) own += g_Game.owned[i];
			for (i = 0; i < MAX_CONTRACTS; i++) nc += g_Game.contract[i].ad != NONE;
			for (i = 0; i < NUM_SLOTS; i++) sl += g_Game.slot[0][i].movie != NONE;
			printf("dia %2d inicio: caixa=%6ld filmes=%2d contratos=%d slots_com_filme=%d", g_Game.day, (long)g_Game.money, own, nc, sl);
		}
		for (t = 0; t < DAY_MINUTES && !g_Game.game_over; t++)
		{
			if (t == DAY_MINUTES - 1) { int q; for (q = 0; q < NUM_SLOTS; q++) g_ExpAud[q] = g_Game.aud[0][q]; }
			if (g_Trace && t == DAY_MINUTES - 1) memcpy(g_SnapAud, g_Game.aud, sizeof g_SnapAud), memcpy(g_SnapSlot, g_Game.slot, sizeof g_SnapSlot);
			Sim_Tick();
		}
		if (g_Trace)
		{
			int s2, n = 0, sum = 0;
			for (s2 = 0; s2 < NUM_SLOTS; s2++) { sum += g_Game.aud[0][s2]; n++; }
			printf("  -> caixa=%6ld  img=%d,%d,%d\n", (long)g_Game.money, g_Game.image[0], g_Game.image[1], g_Game.image[2]);
			if (g_Game.day <= 3)
			{
				int c;
				printf("     minha audiencia por slot (0,1M):");
				for (s2 = 0; s2 < NUM_SLOTS; s2++) printf(" %d(%s)", g_SnapAud[0][s2], g_SnapSlot[0][s2].movie == NONE ? "-" : g_Movies[g_SnapSlot[0][s2].movie].title);
				printf("\n     rivais prime(slot3): FunTV=%d SunTV=%d\n     contratos:", g_SnapAud[1][3], g_SnapAud[2][3]);
				for (c = 0; c < MAX_CONTRACTS; c++) if (g_Game.contract[c].ad != NONE) printf(" [%s min=%d reps=%d dias=%d pay=%d pen=%d]", g_Ads[g_Game.contract[c].ad].title, g_Ads[g_Game.contract[c].ad].min_audience, g_Game.contract[c].reps_left, g_Game.contract[c].days_left, g_Ads[g_Game.contract[c].ad].profit, g_Ads[g_Game.contract[c].ad].penalty);
				printf("\n");
			}
		}
		day++;
	}
	r.bankrupt = g_Game.game_over;
	r.days = g_Game.day;
	r.money_end = g_Game.money;
	r.image_end = g_Game.image[0];
	return r;
}

static int cmp_l(const void* a, const void* b) { long x = *(const long*)a, y = *(const long*)b; return x < y ? -1 : x > y; }

extern long g_StatDone, g_StatFail, g_StatIncome, g_StatPenalty, g_StatMissed, g_StatSpots;
int main(int argc, char** argv)
{
	int days = argc > 1 ? atoi(argv[1]) : 30, runs = argc > 2 ? atoi(argv[2]) : 200, p, s;
	if (argc > 3 && !strcmp(argv[3], "trace")) { g_Trace = 1; Play(argc > 4 ? (Policy)atoi(argv[4]) : P_SMART, 1000, days); return 0; }
	printf("Autoplay: %d dias, %d partidas por politica (caixa inicial %dk, custo diario %dk)\n\n", days, runs, START_MONEY, DAILY_UPKEEP);
	for (p = P_IDLE; p <= P_SMART; p++)
	{
		long* m = malloc(sizeof(long) * runs);
		int bk = 0; long img = 0;
		for (s = 0; s < runs; s++)
		{
			Result r = Play((Policy)p, (u16)(1000 + s * 37), days);
			bk += r.bankrupt; m[s] = r.money_end; img += r.image_end;
		}
		qsort(m, runs, sizeof(long), cmp_l);
		printf("%-30s falencia: %3d%%  caixa final (k$) p10=%6ld mediana=%6ld p90=%6ld  Image medio MadTV=%ld\n",
			k_PolName[p], bk * 100 / runs, m[runs / 10], m[runs / 2], m[runs * 9 / 10], img / runs);
		free(m);
		printf("   contratos cumpridos=%ld falhos=%ld  exibicoes ok=%ld sem audiencia=%ld  receita=%ldk multas=%ldk\n", g_StatDone/runs, g_StatFail/runs, g_StatSpots/runs, g_StatMissed/runs, g_StatIncome/runs, g_StatPenalty/runs);
		g_StatDone = g_StatFail = g_StatSpots = g_StatMissed = g_StatIncome = g_StatPenalty = 0;
	}
	return 0;
}
