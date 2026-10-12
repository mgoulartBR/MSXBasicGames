// Rivais e emissora (2.8): escolha da emissora, sabotagem, Sammy e disputa do catalogo. Segmento 38 (banco 2); separado de sim_ext.c (segmento 6 estourou 8 KB).
#include "sim.h"
#include "data/db_data.h"

u8 g_Station;
static const u8 k_StartImg[3][3] = { { 34, 33, 33 }, { 29, 35, 36 }, { 42, 29, 29 } };     // por emissora escolhida (soma 100); [0] = jogador
static const i16 k_StartCash[3] = { 0, 500, -500 };

void Station_Apply(void) __banked                              // [0] = a escolhida; [1] e [2] = as outras em ordem
{
	u8 i, k = 0;
	g_StationName[0] = g_StName3[g_Station];
	for (i = 0; i < 3; i++) if (i != g_Station) g_StationName[++k] = g_StName3[i];
#ifndef HOST_BUILD
	{ static const u8 k_Col[3] = { 2, 4, 5 };                      // UI_RED, UI_GREEN, UI_CYAN (identidade da emissora)
	  extern u8 g_StationCol[3];
	  g_StationCol[0] = k_Col[g_Station]; k = 0;
	  for (i = 0; i < 3; i++) if (i != g_Station) g_StationCol[++k] = k_Col[i]; }
#endif
}

void Sim_MsgWho(u8 st, u8 what) __banked
{
	static const char* const k_What[3] = { " just signed that one!", " went bankrupt!", " won the Sammy for best news." };
	char t[40];
	const char *a = g_StationName[st], *b = k_What[what];
	u8 n = 0;
	while (*a && n < 8) t[n++] = *a++;
	while (*b && n < 38) t[n++] = *b++;
	t[n] = 0;
	Sim_Msg(t);
}

void Station_Init(void) __banked
{
	Station_Apply();
	g_Game.money += k_StartCash[g_Station];
	g_Game.image[0] = k_StartImg[g_Station][0]; g_Game.image[1] = k_StartImg[g_Station][1]; g_Game.image[2] = k_StartImg[g_Station][2];
}

// Sabotagem dos rivais (fim do dia): so quem esta na frente (Image maior que a sua) age; chance sobe com a dificuldade.
// Efeitos: campanha de difamacao (-2 Image), telejornal envelhecido (+6 h nas noticias) ou contrato de anuncio roubado (sem multa).
void Sim_RivalDay(void) __banked
{
	u8 st, i, k;
	for (st = 1; st < NUM_STATIONS; st++)
	{
		if (!g_Game.alive[st] || g_Game.image[st] <= g_Game.image[0]) continue;
		if (Sim_Rnd(100) >= (u8)(SABOTAGE_PCT + 6 * g_Diff)) continue;
		k = Sim_Rnd(3);
		if (k == 0)
		{
			g_Game.image[0] = (u8)(g_Game.image[0] > 2 ? g_Game.image[0] - 2 : 0);
			Sim_Msg("Smear campaign! Your Image -2.");
		}
		else if (k == 1)
		{
			for (i = 0; i < NEWS_SLATE; i++) if (g_Game.news_slate[i].idx != NONE) g_Game.news_slate[i].age = (u8)(g_Game.news_slate[i].age < 244 ? g_Game.news_slate[i].age + 6 : 250);
			Sim_Msg("Rival bribed your reporters: newscast stale!");
		}
		else
		{
			for (i = 0; i < MAX_CONTRACTS; i++)
				if (g_Game.contract[i].ad != NONE)
				{
					u8 s;
					g_Game.contract[i].ad = NONE;
					for (s = 0; s < NUM_SLOTS; s++) if (g_Game.slot[0][s].ad == i) g_Game.slot[0][s].ad = NONE;
					Sim_Msg("A rival poached one of your ad contracts!");
					break;
				}
			if (i == MAX_CONTRACTS) Sim_Msg("A rival spy was seen in the lobby.");
		}
		return;                                  // no maximo uma sabotagem por dia
	}
}

// Sammy (premio de melhor telejornal): a cada SAMMY_DAYS dias, no fim do dia, o telejornal do jogador (qualidade x frescor) disputa com
// um sorteio de cada rival vivo. Vencedor: +5 Image (rival: +3); o jogador ainda recebe SAMMY_PRIZE k. Sem estado novo (o savegame nao muda).
void Sim_SammyDay(void) __banked
{
	u8 q = Sim_NewsQuality(), st, best = 0, who = 0, s;
	if (g_Game.day % SAMMY_DAYS) return;
	for (st = 1; st < NUM_STATIONS; st++)
		if (g_Game.alive[st])
		{
			s = (u8)(SAMMY_RIVAL_MIN + Sim_Rnd(SAMMY_RIVAL_SPAN) + 5 * g_Diff);
			if (s > best) { best = s; who = st; }
		}
	if (!who) best = SAMMY_RIVAL_MIN;                     // sem rivais: basta um telejornal minimo
	if (q > best)
	{
		g_Game.image[0] = (u8)(g_Game.image[0] + 5 > 100 ? 100 : g_Game.image[0] + 5);
		g_Game.money += SAMMY_PRIZE; g_Game.day_income += SAMMY_PRIZE;
		Sim_MsgNum("You won the Sammy! Image +5, prize $", SAMMY_PRIZE, "k");
		g_JingleReq = 5;
	}
	else if (who)
	{
		g_Game.image[who] = (u8)(g_Game.image[who] + 3 > 100 ? 100 : g_Game.image[who] + 3);
		Sim_MsgWho(who, 2);
	}
	else Sim_Msg("Nobody won the Sammy: your news were too weak.");
}

// Rivais disputam o catalogo (2.7): a cada TAKEN_WINDOW dias, uma fracao dos filmes e "levada" por FunTV ou SunTV e nao pode ser comprada.
// Sem estado novo: e uma funcao deterministica do filme e da janela de dias (o savegame nao muda). Filmes de elenco forte sao mais disputados.
u8 Sim_TakenBy(u8 m) __banked
{
	u16 x;
	u8 who;
	if (m >= DB_NUM_MOVIES || g_Game.owned[m]) return 0;
	x = (u16)(m * 251u + (g_Game.day / TAKEN_WINDOW) * 97u + 13u);
	x ^= (u16)(x >> 5); x = (u16)(x * 1231u); x ^= (u16)(x >> 7);
	if ((u8)(x % 100) >= (u8)(TAKEN_BASE_PCT + Db_Stars(m) * TAKEN_PER_STAR)) return 0;
	who = (u8)(1 + ((x >> 9) & 1));
	if (!g_Game.alive[who]) who = (u8)(3 - who);
	return g_Game.alive[who] ? who : 0;
}
