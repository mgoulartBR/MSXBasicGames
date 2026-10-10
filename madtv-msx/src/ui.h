// Camada de video/UI (Screen 5): texto via fonte em VRAM (copia VDP), retangulos, barras, entrada.
#pragma once
#include "msxgl.h"

// Cores do texto = variantes da fonte pre-carregadas em VRAM (pagina 1)
enum { UI_WHITE, UI_YELLOW, UI_RED, UI_GRAY, UI_GREEN, UI_CYAN, UI_NUM_COLORS };

#define UI_BG        COLOR_BLACK
#define UI_PANEL     COLOR_DARK_BLUE

void Ui_Init(void);
void Ui_Color(u8 uicolor);
void Ui_Text(u8 x, u8 y, const char* s);
u8   Ui_Int(u8 x, u8 y, i16 v);          // numero com sinal; retorna x logo apos o texto
void Ui_TextN(u8 x, u8 y, const char* s, u8 maxchars);
void Ui_Dec1(u8 x, u8 y, u8 v10);       // v10 = valor x10 -> "d.d"
void Ui_TextR(u8 x, u8 y, const char* s); // alinhado a direita em x
void Ui_Fill(u8 x, u8 y, u8 w, u8 h, u8 col);
void Ui_Clear(void);

// Desenho "atomico": entre Ui_Begin() e Ui_End() tudo e desenhado na pagina 2 da VRAM (fora da tela) e so entao
// copiado de uma vez para a area visivel -> nunca aparece o estado intermediario "apagado".
// Ui_Begin/Ui_End podem ser aninhados; a copia ocorre no Ui_End mais externo (usa o retangulo dele).
void Ui_Begin(void);
void Ui_End(u8 x, u8 y, u8 w, u8 h);
u16  Ui_DirectBegin(void);     // desenha direto na tela mesmo dentro de Begin/End (retorna estado p/ Ui_DirectEnd)
void Ui_DirectEnd(u16 saved);
void Ui_Bar(u8 x, u8 y, u8 w, u8 h, u8 pct, u8 col);

// Entrada (teclado + joystick 1). Eventos "novos" desde o ultimo Input_Poll.
#define IN_UP    (1 << 0)
#define IN_DOWN  (1 << 1)
#define IN_LEFT  (1 << 2)
#define IN_RIGHT (1 << 3)
#define IN_OK    (1 << 4)   // Enter / Espaco / botao A
#define IN_BACK  (1 << 5)   // ESC / botao B
#define IN_SPEED (1 << 6)   // TAB: alterna velocidade
#define IN_PAUSE (1 << 7)   // P
u8 Input_Poll(void);

// Tecla digitada neste frame (borda): '0'-'9', 'A'-'Z' ou 8 (BS); 0 se nenhuma. Varre a matriz do teclado (so usar na tela de codigo).
u8 Input_TypedChar(void);

// ---- ponteiro (mouse). O MSX nao distingue "mouse" de "joystick" na porta: o modo e escolhido pelo jogador (tecla M).
// Modo 3 = AUTOMATICO (padrao: procura um mouse nas portas 1 e 2; o cursor ja aparece), 1 = mouse na porta 1 (joystick passa a
// ser lido na porta 2), 2 = mouse na porta 2, 0 = desligado (cursor escondido). Tecla M cicla 3 -> 1 -> 2 -> 0 -> 3.
#define PTR_MOVED  1
#define PTR_LEFT   2     // botao esquerdo apertado agora (borda)
#define PTR_RIGHT  4     // botao direito apertado agora (borda)
extern u8 g_PtrX, g_PtrY, g_PtrMode, g_PtrInject, g_PtrFound, g_PtrSpeed;   // g_PtrSpeed: 0 lento, 1 normal, 2 rapido, 3 muito rapido
extern u8 g_InExtra;           // bit 0 = tecla M apertada agora (borda); preenchido por Input_Poll
u8   Pointer_Update(void);     // le o mouse (se ativo), move o cursor-sprite; retorna PTR_*
void Pointer_Cycle(void);      // M: desligado -> porta 1 -> porta 2 -> desligado
void Ui_SpriteSetup(void);     // tabelas/registradores de sprites + cursor (chamado por Ui_Init)
// layout da tabela de atributos (4 bytes por sprite): 0 = seta branca, 1 = contorno, 2 = jogador, 3-4 = rivais, 5 = elevador
#define SPR_PAT_LO  0xF000     // padroes em 1F000h (pagina 3 da VRAM)
#define SPR_COL_LO  0xF800     // cores em 1F800h
#define SPR_ATT_LO  0xFA00     // atributos em 1FA00h (R#5 = F7h: o VDP ignora os bits 8-7)
#define SPR_HIDE_Y  224
