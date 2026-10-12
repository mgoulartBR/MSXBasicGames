// "Casca" visual (1.4): paleta global, paineis com relevo, HUD (cabecalho) e caixas de dica/mensagem. Segmento 14 (banco 2).
// Estilo inspirado no Mad TV original (painel cinza com relevo, campos azuis com texto preto, moldura marrom); desenhado em codigo.
// (Os icones $/relogio/coracao de src/data/art_icons.h deixaram de ser usados na 2.3: o console e todo desenhado em codigo.)
#include "app.h"

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

// ---------------------------------------------------------------- console inferior (2.3): HUD + dica + mensagem em y = 162..212 da tela
// Desenhado no buffer (pagina 2) nas linhas 0..25 (livres: o conteudo usa 26..186 e a fonte preta fica em 218..) e copiado com Ui_EndTo.
// Dica e mensagem usam as linhas 188..211 do buffer (mesmas da tela).
// Esquerda: "TV" da Betty (simpatia); centro: painel cinza com dia/hora e caixa; direita: "TV" das emissoras (Image) e velocidade.
#define CB      94                                         // (u8)(162 + 94) = 0: o console usa as linhas 0..25 do buffer
#define CY      162                                        // topo do console na tela
static u16 c_Day = 0xFFFF, c_T = 0xFFFF;
static i32 c_Money = 0x7FFFFFFF;
static u8  c_Image[NUM_STATIONS] = { 255, 255, 255 };
static u8  c_Speed = 255, c_Sym = 255;

static void CEnd(u8 x, u8 y, u8 w, u8 h) { Ui_EndTo(x, (u8)(y + CB), w, h, y); }
static void CBox(u8 x, u8 y, u8 w, u8 h, u8 face, u8 raised) { Chrome_Box(x, (u8)(y + CB), w, h, face, raised); }
static void CFill(u8 x, u8 y, u8 w, u8 h, u8 c) { Ui_Fill(x, (u8)(y + CB), w, h, c); }
static void CText(u8 x, u8 y, const char* s) { Ui_Text(x, (u8)(y + CB), s); }

void Hud_Reset(void) __banked
{
	c_Day = 0xFFFF; c_T = 0xFFFF; c_Money = 0x7FFFFFFF; c_Speed = 255; c_Sym = 255;
	c_Image[0] = c_Image[1] = c_Image[2] = 255;
	Ui_Begin();
	Ui_Fill(0, 0, 0, 1, C_WOOD_L); Ui_Fill(0, 1, 0, 1, C_WOOD_D);      // moldura no topo da tela (y = 0..1)
	Ui_EndTo(0, 0, 0, 2, 0);
	Ui_Begin();
	CFill(0, CY, 0, 26, C_WOOD_D);
	CBox(2, CY, 84, 26, C_WOOD_L, 1);  CFill(5, CY + 3, 78, 20, 1);          // TV da Betty (dia e simpatia)
	CBox(90, CY, 60, 26, C_FACE, 1);                                         // painel central (hora e caixa)
	CBox(93, CY + 2, 54, 10, C_BLUE, 0); CBox(93, CY + 14, 54, 10, C_BLUE, 0);
	CBox(154, CY, 100, 26, C_WOOD_L, 1); CFill(157, CY + 3, 94, 20, 1);      // TV das emissoras (Image) e velocidade
	CEnd(0, CY, 256, 26);
}

void Hud_Draw(void) __banked
{
	char b[8];
	u8 x, st;
	if (c_Day != g_Game.day)
	{
		c_Day = g_Game.day;
		Ui_Begin(); CFill(6, CY + 5, 74, 8, 1);
		Ui_Color(UI_YELLOW); CText(8, CY + 5, "Day"); Ui_Int(32, (u8)(CY + 5 + CB), g_Game.day);
		CEnd(6, CY + 5, 74, 8);
	}
	if (c_T != g_Game.t)
	{
		c_T = g_Game.t;
		Ui_Begin(); CFill(94, CY + 3, 52, 8, C_BLUE);
		Ui_Color(UI_BLACK); ClockStr(b, g_Game.t); CText(108, CY + 3, b);
		CEnd(94, CY + 3, 52, 8);
	}
	if (c_Money != g_Game.money)
	{
		c_Money = g_Game.money;
		Ui_Begin(); CFill(94, CY + 15, 52, 8, C_BLUE);
		Ui_Color(g_Game.money < 0 ? UI_RED : UI_BLACK);
		CText(96, CY + 15, "$"); x = Ui_Int(104, (u8)(CY + 15 + CB), (i16)g_Game.money); CText(x, CY + 15, "k");
		CEnd(94, CY + 15, 52, 8);
	}
	if (c_Sym != g_Game.sym[0])
	{
		c_Sym = g_Game.sym[0];
		Ui_Begin(); CFill(6, CY + 14, 74, 8, 1);
		Ui_Color(UI_RED); CText(8, CY + 14, "<3"); Ui_Color(UI_WHITE); x = Ui_Int(24, (u8)(CY + 14 + CB), g_Game.sym[0]); CText(x, CY + 14, "/100");
		CEnd(6, CY + 14, 74, 8);
	}
	if (c_Speed != g_Speed)
	{
		c_Speed = g_Speed;
		Ui_Begin(); CFill(208, CY + 14, 42, 8, 1);
		Ui_Color(g_Speed == SPEED_PAUSE ? UI_RED : UI_YELLOW);
		Ui_TextR(248, (u8)(CY + 14 + CB), g_Speed == SPEED_PAUSE ? "PAUSE" : g_Speed == SPEED_1 ? ">" : g_Speed == SPEED_2 ? ">>" : ">>>");
		CEnd(208, CY + 14, 42, 8);
	}
	for (st = 0; st < NUM_STATIONS; st++)
		if (c_Image[st] != g_Game.image[st])
		{
			u8 sx = (u8)(st == 1 ? 206 : 158), sy = (u8)(st == 2 ? CY + 14 : CY + 5);
			c_Image[st] = g_Game.image[st];
			Ui_Begin(); CFill(sx, sy, 46, 8, 1);
			Ui_Color(g_StationCol[st]); Ui_TextN(sx, (u8)(sy + CB), g_StationName[st], 3);
			Ui_Color(UI_WHITE); Ui_Int((u8)(sx + 22), (u8)(sy + CB), g_Game.image[st]);
			CEnd(sx, sy, 46, 8);
		}
}

// ---------------------------------------------------------------- linhas de dica (cinza, relevo) e mensagem (campo azul, texto preto)
void Chrome_Hint(const char* s) __banked
{
	Ui_Begin();
	Chrome_Box(0, 188, 256, 12, C_FACE, 1);
	Ui_Color(UI_BLACK); Ui_Text(4, 190, s);
	Ui_EndTo(0, 188, 256, 12, 188);
}

void Chrome_Msg(const char* s) __banked
{
	Ui_Begin();
	Chrome_Box(0, 200, 256, 12, C_BLUE, 0);
	Ui_Color(UI_BLACK); Ui_Text(4, 202, s);
	Ui_EndTo(0, 200, 256, 12, 200);
}
