// Interface compartilhada entre o nucleo fixo (main.c, ui.c, sim.c) e os modulos de tela em segmentos banked (screens_*.c).
// Funcoes __banked vivem em segmentos do mapper mapeados em 8000h (banco 2); a chamada e feita por trampolim no codigo fixo.
#pragma once
#include "msxgl.h"
#include "ui.h"
#include "sim.h"
#include "data/db_data.h"

#define CONTENT_Y   28
#define ROW_H       10
#define MSG_Y       201
#define LIST_ROWS   11

enum { SCR_TITLE, SCR_HUB, SCR_GRID, SCR_AGENCY, SCR_ADS, SCR_NEWS, SCR_ARCHIVE, SCR_BOSS, SCR_RATINGS, SCR_SAVE, SCR_OFFICE, SCR_OVER };
enum { SPEED_PAUSE, SPEED_1, SPEED_2, SPEED_3, SPEED_COUNT };

// bits de "sujo" (o que precisa ser redesenhado)
#define D_HDR  1    // cabecalho
#define D_CON  2    // tela inteira (so ao entrar na tela)
#define D_MSG  4    // linha de mensagem
#define D_DAT  8    // so dados dinamicos (sem limpar a tela)
#define D_ROWS 16   // so as linhas g_RowA/g_RowB (cursor andou sem rolar)
#define D_LIST 32   // todas as linhas da lista (rolou / trocou de lista), sem limpar a tela
#define D_DET  64   // so o painel de detalhes

// estado compartilhado (RAM fixa; inicializado em main(), nunca confiar em zero-init)
extern u8 g_Screen, g_Speed, g_Dirty, g_RowA, g_RowB, g_Sel, g_First;
extern u8 g_StationCol[NUM_STATIONS];

// utilitarios do nucleo fixo (main.c)
void MarkRows(u8 a, u8 b);
u8   Nav(u8 ev, u8 n, u8 rows);          // 0 nada, 1 so linhas A/B, 2 rolou
void NavApply(u8 r);
void ClockStr(char* b, u16 t);           // t = minutos desde 17:00 -> "HH:MM"
void Money(u8 x, u8 y, i32 k);           // "$<k>k"
void Hint(const char* s);
void ClearContent(void);
void Goto(u8 screen);                    // troca de tela (reinicia cursor e marca D_CON)
void Header_Invalidate(void);

// telas (todas __banked). Convencao: X_Enter inicializa o estado; X_Draw = tela inteira; X_Input trata entrada;
// X_Row(r) uma linha; X_List() todas as linhas; X_Detail() painel de detalhes; X_Dyn() atualizacao horaria de dados.
void Grid_Enter(void) __banked;    void Grid_Draw(void) __banked;    void Grid_Input(u8 ev) __banked;
void Grid_Row(u8 r) __banked;      void Grid_List(void) __banked;    void Grid_Dyn(void) __banked;
void Agency_Enter(void) __banked;  void Agency_Draw(void) __banked;  void Agency_Input(u8 ev) __banked;
void Agency_Row(u8 r) __banked;    void Agency_List(void) __banked;  void Agency_Detail(void) __banked;
void Ads_Enter(void) __banked;     void Ads_Draw(void) __banked;     void Ads_Input(u8 ev) __banked;
void Ads_Row(u8 r) __banked;       void Ads_Detail(void) __banked;
void Ratings_Draw(void) __banked;  void Ratings_Dyn(void) __banked;  void Ratings_Input(u8 ev) __banked;

void News_Enter(void) __banked;    void News_Draw(void) __banked;    void News_Input(u8 ev) __banked;
void News_Row(u8 r) __banked;      void News_Dyn(void) __banked;     void News_Detail(void) __banked;
void Archive_Enter(void) __banked; void Archive_Draw(void) __banked; void Archive_Input(u8 ev) __banked;
void Archive_Row(u8 r) __banked;   void Archive_List(void) __banked;
void Boss_Enter(void) __banked;    void Boss_Draw(void) __banked;    void Boss_Input(u8 ev) __banked;
void Boss_Row(u8 r) __banked;
void Save_Enter(void) __banked;    void Save_EnterLoad(void) __banked; void Save_Draw(void) __banked; void Save_Input(u8 ev) __banked;
void Save_Row(u8 r) __banked;

// predio (hub navegavel) e escritorio
extern u8 g_BldSel;
void Building_Reset(void) __banked; void Building_Enter(void) __banked;  void Building_Leave(void) __banked;
void Building_Draw(void) __banked;  void Building_Input(u8 ev) __banked; void Building_Row(u8 r) __banked;
void Building_Detail(void) __banked; void Building_Dyn(void) __banked;   u8 Building_Frame(void) __banked;
void Office_Enter(void) __banked;   void Office_Draw(void) __banked;     void Office_Input(u8 ev) __banked;
void Office_Row(u8 r) __banked;
