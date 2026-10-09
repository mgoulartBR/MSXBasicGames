// Parte "estendida" da simulacao, compilada no segmento 6 (banco 2): IA dos rivais, noticias, ofertas, salvar/carregar.
// Chamada a partir do nucleo fixo (sim.c) por trampolim __banked.
#include "sim.h"
#include "data/db_data.h"

#define Rnd Sim_Rnd

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
			v = (u8)((u16)Sim_Quality(m) * g_Fit[g_Movies[m].cat][s] / 100);
			if (g_Movies[m].fsk18 && s < 3) v = 0;
			v = (u8)(v * (100 - (g_Game.plays[st][m] > 4 ? 60 : g_Game.plays[st][m] * 12)) / 100);
			if (v >= bestv) { bestv = v; best = m; }
		}
		if (best == NONE) { s++; continue; }
		for (k = 0; k < g_Movies[best].blocks; k++) { sl[s + k].movie = best; sl[s + k].part = k; }
		s += g_Movies[best].blocks;
	}
}


// ---------------------------------------------------------------- noticias
u8 Sim_NewsFresh(u8 age) __banked
{
	u8 f = (age >= 9) ? 10 : (u8)(100 - 10 * age);
	return f;
}

u16 Sim_NewsCost(u8 idx) __banked
{
	NewsRec r;
	Db_News(idx, &r);
	return (u16)(r.price / NEWS_COST_DIV);
}

u8 Sim_NewsQuality(void) __banked
{
	u16 sum = 0;
	u8 i;
	NewsRec r;
	for (i = 0; i < NEWS_SLATE; i++)
	{
		if (g_Game.news_slate[i].idx == NONE) continue;
		Db_News(g_Game.news_slate[i].idx, &r);
		sum += (u16)r.quality * Sim_NewsFresh(g_Game.news_slate[i].age) / 100;
	}
	return (u8)(sum / NEWS_SLATE);
}

// remove buracos do pool (itens ocupados vem primeiro, mantendo a ordem) - a lista na tela nunca tem linhas vazias no meio
static void PoolCompact(void)
{
	u8 i, n = 0;
	for (i = 0; i < NEWS_POOL; i++)
		if (g_Game.news_pool[i].idx != NONE) g_Game.news_pool[n++] = g_Game.news_pool[i];
	for (i = n; i < NEWS_POOL; i++) g_Game.news_pool[i].idx = NONE;
}

static u8 NewsHas(u8 idx)
{
	u8 i;
	for (i = 0; i < NEWS_POOL; i++) if (g_Game.news_pool[i].idx == idx) return 1;
	for (i = 0; i < NEWS_SLATE; i++) if (g_Game.news_slate[i].idx == idx) return 1;
	return 0;
}

static void NewsSpawn(u8 agency)
{
	u8 tries, idx, i, slot = NONE, oldest = 0;
	NewsRec r;
	for (i = 0; i < NEWS_POOL; i++) if (g_Game.news_pool[i].idx == NONE) { slot = i; break; }
	if (slot == NONE)                                        // pool cheio: substitui a mais velha
	{
		for (i = 1; i < NEWS_POOL; i++) if (g_Game.news_pool[i].age > g_Game.news_pool[oldest].age) oldest = i;
		slot = oldest;
	}
	for (tries = 0; tries < 20; tries++)
	{
		idx = Rnd(DB_NUM_NEWS);
		Db_News(idx, &r);
		if (r.agency != agency || NewsHas(idx)) continue;
		g_Game.news_pool[slot].idx = idx; g_Game.news_pool[slot].age = 0;
		return;
	}
}

// a cada hora cheia: envelhece, remove noticias velhas, agencias entregam novas
void Sim_ExtHour(void) __banked
{
	u8 i;
	for (i = 0; i < NEWS_POOL; i++)
		if (g_Game.news_pool[i].idx != NONE)
		{
			g_Game.news_pool[i].age++;
			if (g_Game.news_pool[i].age >= NEWS_MAX_AGE) g_Game.news_pool[i].idx = NONE;
		}
	PoolCompact();
	for (i = 0; i < NEWS_SLATE; i++)
		if (g_Game.news_slate[i].idx != NONE && g_Game.news_slate[i].age < 250) g_Game.news_slate[i].age++;
	for (i = 0; i < DB_NUM_AGENCIES; i++)
		if (g_Game.news_sub[i] && Rnd(2)) NewsSpawn(i);
	if (g_Game.t >= FIRST_SLOT_T && g_Game.t < FIRST_SLOT_T + NUM_SLOTS * 60)
		g_Game.news_f[(g_Game.t - FIRST_SLOT_T) / 60] = (u8)(70 + (u16)Sim_NewsQuality() * 30 / 100);
}

u8 Sim_NewsPick(u8 pi) __banked
{
	u8 s, idx;
	u16 c;
	if (g_Game.news_pool[pi].idx == NONE) return 2;
	idx = g_Game.news_pool[pi].idx;
	c = Sim_NewsCost(idx);
	if (g_Game.money < (i32)c) return 1;
	for (s = 0; s < NEWS_SLATE; s++) if (g_Game.news_slate[s].idx == NONE) break;
	if (s == NEWS_SLATE) s = 0;                              // sem vaga: troca o primeiro
	g_Game.money -= c; g_Game.day_cost += c;
	g_Game.news_slate[s] = g_Game.news_pool[pi];
	g_Game.news_pool[pi].idx = NONE;
	PoolCompact();
	return 0;
}

void Sim_NewsClear(u8 si) __banked
{
	g_Game.news_slate[si].idx = NONE;
}

void Sim_NewsToggle(u8 a) __banked
{
	g_Game.news_sub[a] ^= 1;
}


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


void Sim_ExtNewDay(void) __banked
{
	AiSchedule(1); AiSchedule(2);
	NewOffers();
}

void Sim_ExtInit(void) __banked
{
	u8 i, st;
	for (i = 0; i < NEWS_POOL; i++) g_Game.news_pool[i].idx = NONE;
	for (i = 0; i < NEWS_SLATE; i++) g_Game.news_slate[i].idx = NONE;
	for (i = 0; i < NUM_SLOTS; i++) g_Game.news_f[i] = 70;
	g_Game.news_sub[0] = 1;                          // comeca assinando so Politics
	for (i = 0; i < 4; i++) NewsSpawn(0);
	for (st = 0; st < 2; st++)                       // biblioteca de cada rival = permutacao embaralhada do catalogo
	{
		for (i = 0; i < DB_NUM_MOVIES; i++) g_Game.rival_lib[st][i] = i;
		for (i = DB_NUM_MOVIES - 1; i > 0; i--) { u8 j = Rnd(i + 1), tmp = g_Game.rival_lib[st][i]; g_Game.rival_lib[st][i] = g_Game.rival_lib[st][j]; g_Game.rival_lib[st][j] = tmp; }
	}
}

// ---------------------------------------------------------------- salvar / carregar por codigo
// Sem SRAM no cartucho ASCII8: o estado essencial vira um codigo de SAVE_CHARS caracteres (alfabeto de 32). Nao salvos:
// desgaste dos filmes (plays), noticias (pool/telejornal), ofertas do dia e a grade de hoje dos rivais - sao regerados.
static const char k_B32[] = "0123456789ABCDEFGHJKLMNPQRSTUVWX";
#define OWNED_BYTES ((DB_NUM_MOVIES + 7) / 8)

static u16 Fletcher16(const u8* b, u8 n)
{
	u16 s1 = 0, s2 = 0;
	u8 i;
	for (i = 0; i < n; i++) { s1 = (s1 + b[i]) % 255; s2 = (s2 + s1) % 255; }
	return (u16)((s2 << 8) | s1);
}

void Sim_SaveCode(char* out) __banked
{
	u8 b[SAVE_BYTES];
	u8 n = 0, i, s;
	u16 ck;
	u32 buf = 0;
	u8 bits = 0, o = 0;
	u16 debt = (u16)(g_Game.debt > 65535 ? 65535 : g_Game.debt);
	for (i = 0; i < SAVE_BYTES; i++) b[i] = 0;
	b[n++] = 4;                                                       // versao do formato
	b[n++] = (u8)g_Game.day; b[n++] = (u8)(g_Game.day >> 8);
	b[n++] = (u8)g_Game.t;   b[n++] = (u8)(g_Game.t >> 8);
	b[n++] = (u8)g_Game.money; b[n++] = (u8)(g_Game.money >> 8); b[n++] = (u8)(g_Game.money >> 16); b[n++] = (u8)(g_Game.money >> 24);
	b[n++] = (u8)debt; b[n++] = (u8)(debt >> 8);
	b[n++] = g_Game.image[0]; b[n++] = g_Game.image[1];
	for (i = 0; i < DB_NUM_MOVIES; i++) if (g_Game.owned[i]) b[n + (i >> 3)] |= (u8)(1 << (i & 7));
	n += OWNED_BYTES;
	b[n++] = (u8)(g_Game.news_sub[0] | (g_Game.news_sub[1] << 1) | (g_Game.news_sub[2] << 2));
	b[n++] = (u8)g_Game.seed; b[n++] = (u8)(g_Game.seed >> 8);
	for (i = 0; i < MAX_CONTRACTS; i++)
	{
		const Contract* c = &g_Game.contract[i];
		b[n++] = (u8)((c->ad == NONE ? 31 : c->ad) | ((c->reps_left & 7) << 5));
		b[n++] = c->days_left;
	}
	for (s = 0; s < NUM_SLOTS; s++) { b[n++] = g_Game.slot[0][s].movie; b[n++] = g_Game.slot[0][s].ad; }
	ck = Fletcher16(b, n);
	b[n++] = (u8)ck; b[n++] = (u8)(ck >> 8);
	for (i = 0; i < n; i++)                                           // base32
	{
		buf = (buf << 8) | b[i]; bits += 8;
		while (bits >= 5) { out[o++] = k_B32[(buf >> (bits - 5)) & 31]; bits -= 5; }
	}
	if (bits) out[o++] = k_B32[(buf << (5 - bits)) & 31];
	out[o] = 0;
}

u8 Sim_LoadCode(const char* code) __banked
{
	u8 b[SAVE_BYTES + 2];
	u8 n = 0, i, s, c, v, bits = 0;
	u16 ck, day, seed;
	u32 buf = 0;
	u8 len = 0;
	while (code[len]) len++;
	if (len != SAVE_CHARS) return 1;
	for (i = 0; i < len; i++)
	{
		c = (u8)code[i];
		if (c >= 'a' && c <= 'z') c -= 32;
		if (c == 'O') c = '0';
		if (c == 'I') c = '1';
		for (v = 0; v < 32 && k_B32[v] != c; v++) {}
		if (v == 32) return 1;
		buf = (buf << 5) | v; bits += 5;
		if (bits >= 8) { if (n < SAVE_BYTES) b[n++] = (u8)(buf >> (bits - 8)); bits -= 8; }
	}
	if (n != SAVE_BYTES) return 1;
	ck = Fletcher16(b, SAVE_BYTES - 2);
	if ((u8)ck != b[SAVE_BYTES - 2] || (u8)(ck >> 8) != b[SAVE_BYTES - 1]) return 2;
	if (b[0] != 4) return 3;
	// validacao semantica antes de aplicar
	n = 12;
	if (b[11] + b[12] > 100) return 3;
	n = 13 + OWNED_BYTES + 1 + 2;                                     // inicio dos contratos
	for (i = 0; i < MAX_CONTRACTS; i++) { c = b[n + i * 2] & 31; if (c != 31 && c >= DB_NUM_ADS) return 3; }
	n += MAX_CONTRACTS * 2;
	for (s = 0; s < NUM_SLOTS; s++) { if (b[n + s * 2] != NONE && b[n + s * 2] >= DB_NUM_MOVIES) return 3; if (b[n + s * 2 + 1] != NONE && b[n + s * 2 + 1] >= MAX_CONTRACTS) return 3; }
	day = (u16)(b[1] | (b[2] << 8));
	if (day == 0 || (u16)(b[3] | (b[4] << 8)) >= DAY_MINUTES) return 3;
	seed = (u16)(b[13 + OWNED_BYTES + 1] | (b[13 + OWNED_BYTES + 2] << 8));
	// aplica
	Sim_Init(seed);
	g_Game.day = day;
	g_Game.t = (u16)(b[3] | (b[4] << 8));
	g_Game.money = (i32)((u32)b[5] | ((u32)b[6] << 8) | ((u32)b[7] << 16) | ((u32)b[8] << 24));
	g_Game.debt = (u16)(b[9] | (b[10] << 8));
	g_Game.image[0] = b[11]; g_Game.image[1] = b[12]; g_Game.image[2] = (u8)(100 - b[11] - b[12]);
	for (i = 0; i < DB_NUM_MOVIES; i++) g_Game.owned[i] = (b[13 + (i >> 3)] >> (i & 7)) & 1;
	n = 13 + OWNED_BYTES;
	for (i = 0; i < DB_NUM_AGENCIES; i++) g_Game.news_sub[i] = (b[n] >> i) & 1;
	n += 3;
	for (i = 0; i < MAX_CONTRACTS; i++)
	{
		c = b[n + i * 2] & 31;
		g_Game.contract[i].ad = (c == 31) ? NONE : c;
		g_Game.contract[i].reps_left = (u8)(b[n + i * 2] >> 5);
		g_Game.contract[i].days_left = b[n + i * 2 + 1];
	}
	n += MAX_CONTRACTS * 2;
	for (s = 0; s < NUM_SLOTS; s++)
	{
		Slot* sl = &g_Game.slot[0][s];
		sl->movie = b[n + s * 2]; sl->ad = b[n + s * 2 + 1];
		sl->part = (s && sl->movie != NONE && g_Game.slot[0][s - 1].movie == sl->movie && g_Game.slot[0][s - 1].part + 1 < g_Movies[sl->movie].blocks)
			? (u8)(g_Game.slot[0][s - 1].part + 1) : 0;
	}
	AiSchedule(1); AiSchedule(2);
	NewOffers();
	return 0;
}
