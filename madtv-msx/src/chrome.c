// "Casca" visual (1.4): paleta global, paineis com relevo, HUD (cabecalho) e caixas de dica/mensagem. Segmento 14 (banco 2).
// Estilo inspirado no Mad TV original (painel cinza com relevo, campos azuis com texto preto, moldura marrom); desenhado em codigo.
// Os icones ($, relogio, coracao) vem de src/data/art_icons.h, gerado LOCALMENTE a partir da copia do jogo original (nao versionado);
// sem esse arquivo (ART_ICONS 0) o HUD usa simbolos de texto.
#include "app.h"
#include "data/art_icons.h"

// paleta global: indices mantem o "papel" das cores do MSX (1 = preto = fundo, 14 = cinza, 15 = branco ...)
static const u16 k_Pal[16] = {
	RGB16(2,2,2), RGB16(0,0,0), RGB16(2,5,2), RGB16(4,6,4), RGB16(1,2,3), RGB16(2,4,5), RGB16(3,1,0), RGB16(3,5,6),
	RGB16(6,1,1), RGB16(7,3,3), RGB16(6,5,0), RGB16(7,7,0), RGB16(4,2,0), RGB16(4,1,1), RGB16(5,5,5), RGB16(7,7,7)
};
#define C_SHADOW  0      // cinza escuro (sombra do relevo)
#define C_BLUE    5      // campo "LCD"
#define C_FACE    14     // face do painel
#define C_LIGHT   15     // luz do relevo
#define C_WOOD_D  6
#define C_WOOD_L  12

void Chrome_Palette(void) __banked
{
	u8 i;
	for (i = 0; i < 16; i++) VDP_SetPaletteEntry(i, k_Pal[i]);
}

// caixa com relevo: raised = luz em cima/esquerda e sombra embaixo/direita (afundada = o contrario)
void Chrome_Box(u8 x, u8 y, u8 w, u8 h, u8 face, u8 raised) __banked
{
	u8 hi = raised ? C_LIGHT : C_SHADOW, lo = raised ? C_SHADOW : C_LIGHT;
	if (!(x & 1) && !(w & 1)) Ui_Fill(x, y, w, h, face); else Ui_FillPx(x, y, w, h, face);   // HMMV (rapido) quando alinhado a byte; LMMV e ~3x mais lento
	Ui_FillPx(x, y, w, 1, hi); Ui_FillPx(x, y, 1, h, hi);
	Ui_FillPx(x, (u8)(y + h - 1), w, 1, lo); Ui_FillPx((u8)(x + w - 1), y, 1, h, lo);
}

#if ART_ICONS
static void Icon(const u8* d, u8 x, u8 y, u8 w, u8 h)       // bitmap 4 bpp direto no buffer (pagina 2): 2 pixels por byte
{
	u8 r;
	for (r = 0; r < h; r++) VDP_WriteVRAM_128K(d + r * (w >> 1), (u16)((u16)(y + r) * 128 + (x >> 1)), 1, (u16)(w >> 1));
}
#endif

#if ART_SCENES
typedef struct { u8 seg, stride, h, rps; } SceneDef;
static const SceneDef k_Scn[] = SCN_INIT;
void Scene_Draw(u8 id, u8 dx, u8 dy) __banked           // linhas de `stride` bytes em segmentos de dados mapeados no banco 3 (rps linhas por segmento)
{
	const SceneDef* d = &k_Scn[id];
	u8 r, seg = 0xFF, s, prev = GET_BANK_SEGMENT(3);
	for (r = 0; r < d->h; r++)
	{
		s = (u8)(d->seg + r / d->rps);
		if (s != seg) { SET_BANK_SEGMENT(3, s); seg = s; }
		Ui_BlitRow((const u8*)(0xA000 + (u16)(r % d->rps) * d->stride), dx, (u8)(dy + r), d->stride);
	}
	SET_BANK_SEGMENT(3, prev);
}
#else
void Scene_Draw(u8 id, u8 dx, u8 dy) __banked { (void)id; (void)dx; (void)dy; }
#endif

#if ART_SCENES
void Room_Frame(u8 id) __banked                          // cena a esquerda + painel cinza a direita (lista) + painel inferior (detalhes)
{
	if (g_BufScene != id)                                  // a cena fica no buffer: so e repintada ao entrar na sala
	{
		ClearContent();
		Scene_Draw(id, 0, CONTENT_Y - 2);
		g_BufScene = id;
	}
	Chrome_Box(122, CONTENT_Y - 2, 134, 112, 14, 1);
}
#else
void Room_Frame(u8 id) __banked { (void)id; }
#endif

// ---------------------------------------------------------------- HUD (cabecalho): so os campos que mudaram sao redesenhados
static u16 c_Day = 0xFFFF, c_T = 0xFFFF;
static i32 c_Money = 0x7FFFFFFF;
static u8  c_Image[NUM_STATIONS] = { 255, 255, 255 };
static u8  c_Speed = 255, c_Sym = 255;
static const u8 k_StFace[NUM_STATIONS] = { 8, 2, 5 };     // vermelho, verde, azul (como os botoes de emissora do original)

#define F1Y 3                                              // linha 1 dos campos (dia, hora, dinheiro, Betty, velocidade)
#define F2Y 13                                             // linha 2 (emissoras)

static void Field(u8 x, u8 y, u8 w, u8 face) { Chrome_Box(x, y, w, 10, face, 0); }

void Hud_Reset(void) __banked
{
	u8 i;
	c_Day = 0xFFFF; c_T = 0xFFFF; c_Money = 0x7FFFFFFF; c_Speed = 255; c_Sym = 255;
	for (i = 0; i < NUM_STATIONS; i++) c_Image[i] = 255;
	Ui_Begin();
	Ui_Fill(0, 0, 256, 1, C_WOOD_L); Ui_Fill(0, 1, 256, 1, C_WOOD_D);
	Ui_Fill(0, 2, 256, 21, C_FACE); Ui_FillPx(0, 2, 256, 1, C_LIGHT);
	Ui_Fill(0, 23, 256, 1, C_WOOD_D);
	Field(4, F1Y, 42, C_BLUE);                                    // dia
	Field(62, F1Y, 36, C_BLUE);                                   // hora
	Field(112, F1Y, 58, C_BLUE);                                  // dinheiro
	Field(186, F1Y, 28, C_BLUE);                                  // Betty
	for (i = 0; i < NUM_STATIONS; i++) Field((u8)(4 + i * 84), F2Y, 80, k_StFace[i]);
#if ART_ICONS
	Icon(k_Art_clock, 50, F1Y, ART_CLOCK_W, ART_CLOCK_H);
	Icon(k_Art_dollar, 101, F1Y, ART_DOLLAR_W, ART_DOLLAR_H);
	Icon(k_Art_heart, 174, F1Y + 1, ART_HEART_W, ART_HEART_H);
#else
	Ui_Color(UI_BLACK); Ui_Text(50, F1Y + 1, "T"); Ui_Text(103, F1Y + 1, "$"); Ui_Text(176, F1Y + 1, "<3");
#endif
	Ui_End(0, 0, 256, 24);
}

void Hud_Draw(void) __banked
{
	char b[8];
	u8 st, x;
	if (c_Day != g_Game.day)
	{
		c_Day = g_Game.day;
		Ui_Begin(); Ui_Fill(6, F1Y + 1, 38, 8, C_BLUE);
		Ui_Color(UI_BLACK); Ui_Text(7, F1Y + 1, "Day"); Ui_Int(28, F1Y + 1, g_Game.day);
		Ui_End(6, F1Y + 1, 38, 8);
	}
	if (c_T != g_Game.t)
	{
		c_T = g_Game.t;
		Ui_Begin(); Ui_Fill(64, F1Y + 1, 32, 8, C_BLUE);
		Ui_Color(UI_BLACK); ClockStr(b, g_Game.t); Ui_Text(66, F1Y + 1, b);
		Ui_End(64, F1Y + 1, 32, 8);
	}
	if (c_Money != g_Game.money)
	{
		c_Money = g_Game.money;
		Ui_Begin(); Ui_Fill(114, F1Y + 1, 54, 8, C_BLUE);
		Ui_Color(g_Game.money < 0 ? UI_RED : UI_BLACK);
		x = Ui_Int(116, F1Y + 1, (i16)g_Game.money); Ui_Text(x, F1Y + 1, "k");
		Ui_End(114, F1Y + 1, 54, 8);
	}
	if (c_Sym != g_Game.sym[0])
	{
		c_Sym = g_Game.sym[0];
		Ui_Begin(); Ui_Fill(188, F1Y + 1, 24, 8, C_BLUE);
		Ui_Color(UI_BLACK); Ui_Int(190, F1Y + 1, g_Game.sym[0]);
		Ui_End(188, F1Y + 1, 24, 8);
	}
	if (c_Speed != g_Speed)
	{
		c_Speed = g_Speed;
		Ui_Begin();
		Chrome_Box(218, F1Y, 34, 10, g_Speed == SPEED_PAUSE ? 8 : C_BLUE, 1);
		Ui_Color(UI_BLACK);
		Ui_TextR(248, F1Y + 1, g_Speed == SPEED_PAUSE ? "PAUSE" : g_Speed == SPEED_1 ? ">" : g_Speed == SPEED_2 ? ">>" : ">>>");
		Ui_End(218, F1Y, 34, 10);
	}
	for (st = 0; st < NUM_STATIONS; st++)
		if (c_Image[st] != g_Game.image[st])
		{
			u8 fx = (u8)(4 + st * 84);
			c_Image[st] = g_Game.image[st];
			Ui_Begin(); Ui_Fill((u8)(fx + 2), F2Y + 1, 76, 8, k_StFace[st]);
			Ui_Color(UI_BLACK);
			Ui_Text((u8)(fx + 4), F2Y + 1, g_StationName[st]);
			Ui_Int((u8)(fx + 48), F2Y + 1, g_Game.image[st]);
			Ui_End((u8)(fx + 2), F2Y + 1, 76, 8);
		}
}

// ---------------------------------------------------------------- linhas de dica (cinza, relevo) e mensagem (campo azul, texto preto)
void Chrome_Hint(const char* s) __banked
{
	Ui_Begin();
	Chrome_Box(0, 186, 256, 14, C_FACE, 1);
	Ui_Color(UI_BLACK); Ui_Text(4, 190, s);
	Ui_End(0, 186, 256, 14);
}

void Chrome_Msg(const char* s) __banked
{
	Ui_Begin();
	Chrome_Box(0, 200, 256, 12, C_BLUE, 0);
	Ui_Color(UI_BLACK); Ui_Text(4, 202, s);
	Ui_End(0, 200, 256, 12);
}
