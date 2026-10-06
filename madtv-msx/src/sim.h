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
#define START_MONEY   1500       // k$
#define DAILY_UPKEEP  120        // k$/dia (torres/estudio - fixo ate existirem torres)
#define BANKRUPT_AT   (-2000)    // k$

typedef struct { u8 movie; u8 part; u8 ad; } Slot;          // movie=NONE: vazio; part = bloco dentro do filme; ad = indice de contrato
typedef struct { u8 ad; u8 reps_left; u8 days_left; } Contract; // ad=NONE: livre

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
	u8  game_over;                      // 0 = jogando, 1 = falencia
} Game;

extern Game g_Game;
extern const u8 g_Reach[NUM_STATIONS];   // alcance maximo (0,1 milhao)
extern char g_Msg[44];                  // ultima mensagem de evento
extern const char* const g_StationName[NUM_STATIONS];

enum { EV_NONE = 0, EV_SLOT = 1, EV_DAY = 2, EV_MSG = 4 };

void  Sim_Init(u16 seed);
u8    Sim_Tick(void);                   // avanca 1 minuto de jogo; retorna mascara EV_*
u16   Sim_MoviePrice(u8 idx);           // k$
u8    Sim_Buy(u8 idx);                  // 0 ok, 1 ja possui, 2 sem dinheiro
u8    Sim_SignAd(u8 offer);             // 0 ok, 1 sem espaco, 2 ja assinada
u8    Sim_PlaceMovie(u8 slot, u8 movie);// 0 ok, 1 nao cabe
void  Sim_ClearSlot(u8 slot);
void  Sim_PlaceAd(u8 slot, u8 contract);
u8    Sim_Quota(u8 station, u8 slot);   // % de audiencia
u8    Sim_Quality(u8 movie);            // 0..100
void  Sim_Msg(const char* s);
void  Sim_MsgNum(const char* a, i32 v, const char* b);
