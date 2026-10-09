// Regras e estado do jogo (sem VDP). Reconstrucao baseada no manual do Mad TV + atributos do TVTower.
// Todas as formulas de audiencia/preco aqui sao PROPRIAS (ver docs/PORTING.md), nao as do original.
#pragma once
#include "msxgl.h"
#include "game_types.h"
#include "data/db_data.h"

#define NUM_STATIONS  3          // 0 = jogador (MadTV), 1 = FunTV, 2 = SunTV
#define NUM_SLOTS     7          // 18:00 .. 00:00
#define DAY_MINUTES   480        // 17:00 .. 01:00
#define FIRST_SLOT_T  60         // minuto (desde 17:00) em que o slot 0 (18:00) comeca
#define NONE          0xFF
#define MAX_CONTRACTS 4
#define NUM_OFFERS    6
// --- parametros de balanceamento (ajustados pelo autoplay: tests/balance.sh) ---
#ifndef START_MONEY
#define START_MONEY   2500       // k$
#endif
#ifndef PRICE_DIV
#define PRICE_DIV     20         // preco do filme = media(atributos)*price_mod/PRICE_DIV * blocos/2 + 50
#endif
#ifndef RIVAL_Q
#define RIVAL_Q       68         // % aplicado a audiencia dos rivais (dificuldade normal)
#endif
#ifndef DAILY_UPKEEP
#define DAILY_UPKEEP  100        // k$/dia (torres/estudio - fixo ate existirem torres)
#endif
#define BANKRUPT_AT   (-2000)    // k$
#ifndef RIVAL_LIB_START
#define RIVAL_LIB_START 5        // filmes iniciais de cada rival
#endif
#ifndef RIVAL_LIB_GROWTH_DAYS
#define RIVAL_LIB_GROWTH_DAYS 1  // +1 filme a cada N dias
#endif

#define NEWS_POOL   8            // noticias recebidas (aguardando escolha)
#define NEWS_SLATE  3            // itens por telejornal
#define NEWS_MAX_AGE 10          // horas ate a noticia sumir do pool
#ifndef NEWS_COST_DIV
#define NEWS_COST_DIV 4          // custo da noticia (k$) = price(x100)/NEWS_COST_DIV
#endif
#ifndef NEWS_FEE
#define NEWS_FEE      30         // k$/dia por agencia assinada
#endif
#ifndef NEWS_RIVAL_F
#define NEWS_RIVAL_F  80         // % de audiencia que o telejornal dos rivais dara (fixo)
#endif
#ifndef PENALTY_PCT
#define PENALTY_PCT   60         // % da multa do banco de dados efetivamente cobrada (TVTower e mais duro que nossas audiencias)
#endif
#define INTEREST_PCT  6          // juros diarios do credito do chefe (%)
#define CREDIT_BASE   3000       // limite de credito base (k$)
#define CREDIT_PER_IMAGE 40      // + k$ por ponto de Image
#define NUM_GIFTS     10
#define GIFT_MAX_STOCK 3
#define GIFT_DREAM    9          // "Dream trip": presente de casamento (nao conta como simpatia)
#define CAT_CULTURE   5          // indice da categoria Culture em db_data
#define OWNED_BYTES   ((DB_NUM_MOVIES + 7) / 8)
#define SAVE_BYTES    (50 + OWNED_BYTES)
#define SAVE_CHARS    ((SAVE_BYTES * 8 + 4) / 5)   // ceil(SAVE_BYTES*8/5)
#ifndef IMAGE_LOW
#define IMAGE_LOW     20         // Image abaixo disto: anunciantes pagam menos e o chefe fecha o credito
#endif
#define LOW_PROFIT_PCT 80
#define NUM_DIFF      3          // 0 easy, 1 normal, 2 hard

typedef struct { u8 movie; u8 part; u8 ad; } Slot;          // movie=NONE: vazio; part = bloco dentro do filme; ad = indice de contrato
typedef struct { u8 ad; u8 reps_left; u8 days_left; } Contract;
typedef struct { u8 idx; u8 age; } NewsItem;                    // idx = indice em g_NewsRec (NONE = vazio); age em horas // ad=NONE: livre

typedef struct {
	u16 day;
	u16 t;                              // minutos desde 17:00 do dia atual (0..479)
	i32 money;                          // k$
	u8  image[NUM_STATIONS];            // soma = 100
	u8  owned[DB_NUM_MOVIES];
	u8  plays[NUM_STATIONS][DB_NUM_MOVIES]; // exibicoes recentes (desgaste)
	Slot slot[NUM_STATIONS][NUM_SLOTS];
	u8  aud[NUM_STATIONS][NUM_SLOTS];   // audiencia medida hoje (0,1 milhao)
	u8  aud_done[NUM_SLOTS];            // 1 = slot ja medido
	Contract contract[MAX_CONTRACTS];
	u8  offer[NUM_OFFERS];              // ofertas do dia na Agencia de Publicidade (NONE = ja assinada)
	i16 day_income, day_cost;
	u8  game_over;                      // 0 = jogando, 1 = falencia ou Image zerado
	u8  won;                            // 1 = casou com a Betty
	u8  sym[NUM_STATIONS];              // simpatia da Betty por cada emissora (<= Image da emissora)
	u8  alive[NUM_STATIONS];            // 0 = emissora faliu (Image 0)
	u8  gift_have[NUM_GIFTS];           // presentes comprados e ainda nao dados (max 3 de cada)
	u8  gift_uses[NUM_GIFTS];           // quantas vezes cada presente ja foi dado a Betty (por qualquer pretendente): efeito cai
	u8  gift_today;                     // jogador deu presente hoje (senao a simpatia diminui)
	u8  last_gain;                      // pontos do ultimo presente dado
	u16 seed;                           // semente da partida (reconstroi as bibliotecas dos rivais ao carregar)
	i32 debt;                           // credito do chefe (k$)
	u8  news_sub[DB_NUM_AGENCIES];      // agencias assinadas
	NewsItem news_pool[NEWS_POOL];      // noticias disponiveis
	NewsItem news_slate[NEWS_SLATE];    // telejornal montado pelo jogador
	u8  news_f[NUM_SLOTS];              // fator (%) do telejornal que antecede cada slot (medido a cada hora cheia)
	u8  rival_lib[2][DB_NUM_MOVIES];    // biblioteca de cada rival: permutacao do catalogo; so os primeiros N sao "possuidos"
} Game;

extern Game g_Game;
extern u8 g_Diff;                       // dificuldade (0..2): muda caixa inicial e forca dos rivais
extern const u8 g_Reach[NUM_STATIONS];   // alcance maximo (0,1 milhao)
extern char g_Msg[44];                  // ultima mensagem de evento
extern const char* const g_StationName[NUM_STATIONS];

enum { EV_NONE = 0, EV_SLOT = 1, EV_DAY = 2, EV_MSG = 4 };

void  Sim_Init(u16 seed);
u8    Sim_Tick(void);                   // avanca 1 minuto de jogo; retorna mascara EV_*
u16   Sim_MoviePrice(u8 idx);           // k$
u8    Sim_Buy(u8 idx);                  // 0 ok, 1 ja possui, 2 sem dinheiro
u16   Sim_Penalty(u8 ad);              // multa efetiva (k$)
u8    Sim_SignAd(u8 offer);             // 0 ok, 1 sem espaco, 2 ja assinada
u8    Sim_PlaceMovie(u8 slot, u8 movie);// 0 ok, 1 nao cabe
void  Sim_ClearSlot(u8 slot);
void  Sim_PlaceAd(u8 slot, u8 contract);
u8    Sim_Quota(u8 station, u8 slot);   // % de audiencia
u8    Sim_Quality(u8 movie);            // 0..100
void  Sim_Msg(const char* s);
// noticias
void  Db_News(u8 idx, NewsRec* out);        // db.c (acesso ao segmento 4)
u8    Sim_NewsFresh(u8 age) __banked;                // frescor 0..100
u8    Sim_NewsQuality(void) __banked;                // 0..100 do telejornal atual (frescor x qualidade)
u16   Sim_NewsCost(u8 idx) __banked;                 // k$
u8    Sim_NewsPick(u8 pool_i) __banked;              // 0 ok, 1 sem dinheiro, 2 vazio
void  Sim_NewsClear(u8 slate_i) __banked;
void  Sim_NewsToggle(u8 agency) __banked;
// partes banked da simulacao (sim_ext.c)
void  Sim_ExtInit(void) __banked;           // noticias iniciais + bibliotecas dos rivais
void  Sim_ExtNewDay(void) __banked;         // IA dos rivais + ofertas do dia
void  Sim_ExtHour(void) __banked;           // hora cheia: noticias + telejornal
u8    Sim_Rnd(u8 n);                        // 0..n-1
extern u16 g_Rng;
extern const u8 g_Fit[DB_NUM_CATEGORIES][NUM_SLOTS];
// Betty e presentes (sim_ext.c)
u16   Sim_GiftCost(u8 g) __banked;          // k$
u8    Sim_GiftEffect(u8 g) __banked;        // pontos de simpatia que o presente daria agora (cai 30%/uso)
u8    Sim_BuyGift(u8 g) __banked;           // 0 ok, 1 sem dinheiro, 2 estoque cheio
u8    Sim_GiveGift(u8 g) __banked;          // 0 ok (ganho em g_Game.last_gain), 1 sem estoque, 2 simpatia ja no teto (Image), 3 reservado p/ casamento
u8    Sim_Propose(void) __banked;           // 0 casou, 1 simpatia < 100, 2 ainda ha rivais, 3 falta a Dream trip
void  Sim_BettyDay(void) __banked;          // fim do dia: decaimento, pretendentes rivais, recuperacao dos usos
// credito / arquivo
i32   Sim_CreditLimit(void);
u8    Sim_Borrow(i32 k);                    // 0 ok, 1 acima do limite
u8    Sim_Repay(i32 k);                     // 0 ok, 1 sem dinheiro, 2 sem divida
u16   Sim_MovieValue(u8 idx);               // valor de revenda (k$)
u8    Sim_Sell(u8 idx);                     // 0 ok, 1 nao possui, 2 esta na grade
// salvar/carregar por codigo (sem SRAM no cartucho ASCII8)
void  Sim_SaveCode(char* out) __banked;              // SAVE_CHARS caracteres + '\0'
u8    Sim_LoadCode(const char* code) __banked;       // 0 ok, 1 tamanho/caracter invalido, 2 checksum, 3 dados invalidos
void  Sim_MsgNum(const char* a, i32 v, const char* b);
