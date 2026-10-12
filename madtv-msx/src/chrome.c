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
	if (!(x & 1) && !(w & 1))                                   // alinhado a byte: HMMV (rapido) para o miolo e para as linhas horizontais
	{
		Ui_Fill(x, y, w, h, face);
		Ui_Fill(x, y, w, 1, hi); Ui_Fill(x, (u8)(y + h - 1), w, 1, lo);
	}
	else { Ui_FillPx(x, y, w, h, face); Ui_FillPx(x, y, w, 1, hi); Ui_FillPx(x, (u8)(y + h - 1), w, 1, lo); }   // LMMV e ~3x mais lento
	Ui_FillPx(x, y, 1, h, hi); Ui_FillPx((u8)(x + w - 1), y, 1, h, lo);
}


#if ART_SCENES
typedef struct { u8 seg, stride, h, rps; } SceneDef;
static const SceneDef k_Scn[] = SCN_INIT;
void Scene_Part(u8 id, u8 sbyte, u8 nbytes, u8 dx, u8 dy) __banked   // so as colunas [sbyte, sbyte+nbytes) de cada linha (1 byte = 2 pixels)
{
	const SceneDef* d = &k_Scn[id];
	u8 r = 0, seg = d->seg, n, prev = GET_BANK_SEGMENT(3);
	while (r < d->h)                                             // um segmento por vez: sem divisoes/multiplicacoes por linha
	{
		const u8* p = (const u8*)(0xA000 + sbyte);
		SET_BANK_SEGMENT(3, seg++);
		for (n = 0; n < d->rps && r < d->h; n++, r++, p += d->stride) Ui_BlitRow(p, dx, (u8)(dy + r), nbytes);
	}
	SET_BANK_SEGMENT(3, prev);
}

void Scene_Draw(u8 id, u8 dx, u8 dy) __banked           // linhas de `stride` bytes em segmentos de dados mapeados no banco 3 (rps linhas por segmento)
{
	Scene_Part(id, 0, k_Scn[id].stride, dx, dy);
}
#else
void Scene_Draw(u8 id, u8 dx, u8 dy) __banked { (void)id; (void)dx; (void)dy; }
void Scene_Part(u8 id, u8 sbyte, u8 nbytes, u8 dx, u8 dy) __banked { (void)id; (void)sbyte; (void)nbytes; (void)dx; (void)dy; }
#endif

#if ART_SCENES
void Room_Frame(u8 id) __banked                          // cena a esquerda + painel cinza a direita (lista) + painel inferior (detalhes)
{
	if (g_BufScene != id)                                  // a cena fica no buffer: so e repintada ao entrar na sala
	{
		Ui_Fill(120, CONTENT_Y - 2, 2, 112, UI_BG);        // so a fresta entre a cena e o painel: o resto e coberto por Scene_Draw e pelos paineis (ClearContent custava 2 jiffies)
		Scene_Draw(id, 0, CONTENT_Y - 2);
		g_BufScene = id;
	}
	Chrome_Box(122, CONTENT_Y - 2, 134, 112, 14, 1);
}
#else
void Room_Frame(u8 id) __banked { (void)id; }
#endif

// ---------------------------------------------------------------- micro-fonte 3x5 (celula 4x6): A-Z, 0-9 e "/" (rotulos das portas, detalhes da grade)
// Guardada em VRAM (pagina 1, linhas 500..505, livres) e copiada com LMMM/TIMP (cor 1 = preto; 0 = transparente).
static const u8 k_Micro[37][5] = {
	{2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},
	{6,5,5,5,5},{2,5,5,5,2},{6,5,6,4,4},{2,5,5,7,3},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7},
	{7,5,5,5,7},{2,6,2,2,7},{6,1,2,4,7},{6,1,2,1,6},{5,5,7,1,1},{7,4,6,1,6},{3,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,6},{1,1,2,4,4}
};
#define MICRO_ROW 500                                      // preta; a branca (cor 15) fica em MICRO_ROW + 6
void Micro_Init(void) __banked
{
	u8 buf[74], r, g;
	VDP_CommandWait();
	u8 v, k;
	for (v = 0; v < 2; v++)                                                           // 0 = preta (cor 1), 1 = branca (cor 15)
	{
		k = v ? 0xF : 0x1;
		for (r = 0; r < 6; r++)
		{
			for (g = 0; g < 37; g++)
			{
				u8 b = (r < 5) ? k_Micro[g][r] : 0;
				buf[g * 2]     = (u8)(((b & 4) ? (k << 4) : 0) | ((b & 2) ? k : 0));       // pixels 0 e 1
				buf[g * 2 + 1] = (u8)((b & 1) ? (k << 4) : 0);                            // pixel 2 (o 3o fica vazio: espaco entre letras)
			}
			VDP_WriteVRAM_128K(buf, (u16)(0x8000 + (u16)(MICRO_ROW - 256 + v * 6 + r) * 128), 0, 74);
		}
	}
}

static void MicroDraw(u8 x, u8 y, const char* s, u16 row)
{
	for (; *s; s++, x += 4)
	{
		u8 c = (u8)*s, g = (c >= 'A' && c <= 'Z') ? (u8)(c - 'A') : (c >= 'a' && c <= 'z') ? (u8)(c - 'a') : (c >= '0' && c <= '9') ? (u8)(26 + c - '0') : (c == '/') ? 36 : 0xFF;
		if (g != 0xFF) VDP_CommandLMMM((u16)g * 4, row, x, (u16)y + 512, 4, 6, VDP_OP_TIMP);
	}
}
void Chrome_Micro(u8 x, u8 y, const char* s) __banked  { MicroDraw(x, y, s, MICRO_ROW); }       // dentro de Ui_Begin (buffer na pagina 2); s tem de estar na RAM
void Chrome_MicroW(u8 x, u8 y, const char* s) __banked { MicroDraw(x, y, s, MICRO_ROW + 6); }    // idem, branca (para fundos escuros)

// ---------------------------------------------------------------- console inferior (2.3): HUD + dica + mensagem em y = 162..212 da tela
// Desenhado no buffer (pagina 2) nas linhas 0..25 (livres: o conteudo usa 26..186 e a fonte preta fica em 218..) e copiado com Ui_EndTo.
// Dica e mensagem usam as linhas 188..211 do buffer (mesmas da tela).
// Esquerda: "TV" da Betty (simpatia); centro: painel cinza com dia/hora e caixa; direita: "TV" das emissoras (Image) e velocidade.
#define CB      94                                         // (u8)(162 + 94) = 0: o console usa as linhas 0..25 do buffer
#define CY      162                                        // topo do console na tela
static u16 c_T = 0xFFFF;
static i32 c_Money = 0x7FFFFFFF;
static u8  c_Image[NUM_STATIONS] = { 255, 255, 255 };
static u8  c_Speed = 255, c_Sym = 255, c_Air = 254;
static u16 c_Day = 0xFFFF;
static char s_Tick[64];                                    // manchete do ticker (copia na RAM): "   titulo   "
static u8  s_TickLen, s_TickPos, s_TickKey = 0xFF;

static void CEnd(u8 x, u8 y, u8 w, u8 h) { Ui_EndTo(x, (u8)(y + CB), w, h, y); }
static void CBox(u8 x, u8 y, u8 w, u8 h, u8 face, u8 raised) { Chrome_Box(x, (u8)(y + CB), w, h, face, raised); }
static void CFill(u8 x, u8 y, u8 w, u8 h, u8 c) { Ui_Fill(x, (u8)(y + CB), w, h, c); }
static void CText(u8 x, u8 y, const char* s) { Ui_Text(x, (u8)(y + CB), s); }

void Hud_Reset(void) __banked
{
	Micro_Init();
	c_Day = 0xFFFF; c_T = 0xFFFF; c_Money = 0x7FFFFFFF; c_Speed = 255; c_Sym = 255; c_Air = 254; s_TickKey = 0xFF; s_TickPos = 0;
	c_Image[0] = c_Image[1] = c_Image[2] = 255;
	Ui_Begin();
	Ui_Fill(0, 0, 0, 1, C_WOOD_L); Ui_Fill(0, 1, 0, 1, C_WOOD_D);      // moldura no topo da tela (y = 0..1)
	Ui_EndTo(0, 0, 0, 2, 0);
	Ui_Begin();
	CFill(0, CY, 0, 26, C_WOOD_D);
	CBox(2, CY, 84, 26, C_WOOD_L, 1);  CFill(5, CY + 3, 78, 20, 1);          // TV da esquerda: no ar agora + Image das emissoras
	CBox(90, CY, 60, 26, C_FACE, 1);                                         // painel central (hora e caixa)
	CBox(93, CY + 2, 54, 10, C_BLUE, 0); CBox(93, CY + 14, 54, 10, C_BLUE, 0);
	CBox(154, CY, 100, 26, C_WOOD_L, 1); CFill(157, CY + 3, 94, 20, 1);      // TV da direita: ticker de noticias + dia/Betty/velocidade
	CEnd(0, CY, 256, 26);
}

static u8 OnAir(void)                                      // filme no ar agora (indice) ou NONE
{
	u16 rel;
	if (g_Game.t < FIRST_SLOT_T) return NONE;
	rel = (u16)((g_Game.t - FIRST_SLOT_T) / 60);
	return rel < NUM_SLOTS ? g_Game.slot[0][rel].movie : NONE;
}

static u8 PutNum(char* b, u8 n, u8 v)                      // escreve v em decimal em b[n..]; devolve o novo n
{
	if (v >= 100) b[n++] = (char)('0' + v / 100);
	if (v >= 10) b[n++] = (char)('0' + v / 10 % 10);
	b[n++] = (char)('0' + v % 10);
	return n;
}

static void TickBuild(void)                                // monta o texto do ticker a partir do telejornal (ou um slogan)
{
	u8 i, n = 0, key = 0;
	NewsRec rec;
	for (i = 0; i < NEWS_SLATE; i++) key = (u8)(key * 3 + g_Game.news_slate[i].idx);
	if (key == s_TickKey) return;
	s_TickKey = key; s_TickPos = 0;
	s_Tick[n++] = ' '; s_Tick[n++] = ' '; s_Tick[n++] = ' ';
	for (i = 0; i < NEWS_SLATE && n < 4; i++)
		if (g_Game.news_slate[i].idx != NONE)
		{
			u8 k;
			Db_News(g_Game.news_slate[i].idx, &rec);
			for (k = 0; rec.title[k] && n < 58; k++) s_Tick[n++] = rec.title[k];
			break;
		}
	if (n == 3) { const char* t = "Mad-TV: the station that never sleeps"; for (i = 0; t[i]; i++) s_Tick[n++] = t[i]; }
	s_Tick[n++] = ' '; s_Tick[n++] = ' '; s_Tick[n++] = ' ';
	s_TickLen = n;
}

void Hud_Tick(void) __banked                               // chamado ~4x por segundo: rola o ticker 1 caractere
{
	char w[16];
	u8 i;
	TickBuild();
	for (i = 0; i < 15; i++) w[i] = s_Tick[(u8)((s_TickPos + i) % s_TickLen)];
	w[15] = 0;
	if (++s_TickPos >= s_TickLen) s_TickPos = 0;
	Ui_Begin(); CFill(158, CY + 5, 92, 8, 1);
	Ui_Color(UI_WHITE); CText(158, CY + 5, w);
	CEnd(158, CY + 5, 92, 8);
}

void Hud_Draw(void) __banked
{
	char b[24];
	u8 x, st, air, n;
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
	air = OnAir();
	if (c_Air != air)                                        // TV da esquerda, linha 1: o que esta no ar
	{
		char t[14];
		u8 k = 0;
		c_Air = air;
		if (air != NONE) { const char* m = (*Mov(air)).title; while (m[k] && k < 13) { t[k] = m[k]; k++; } }
		else { const char* m = "(off air)"; while (m[k]) { t[k] = m[k]; k++; } }
		t[k] = 0;
		Ui_Begin(); CFill(6, CY + 4, 76, 8, 1);
		Ui_Color(air != NONE ? UI_YELLOW : UI_GRAY); CText(7, CY + 4, t);
		CEnd(6, CY + 4, 76, 8);
	}
	for (st = 0; st < NUM_STATIONS; st++)
		if (c_Image[st] != g_Game.image[st]) break;
	if (st < NUM_STATIONS)                                   // TV da esquerda, linha 2: Image das 3 emissoras (micro-fonte branca)
	{
		for (st = 0; st < NUM_STATIONS; st++) c_Image[st] = g_Game.image[st];
		n = 0;
		for (st = 0; st < NUM_STATIONS; st++)
		{
			u8 k;
			for (k = 0; k < 3; k++) { char c = g_StationName[st][k]; b[n++] = (char)(c >= 'a' && c <= 'z' ? c - 32 : c); }
			n = PutNum(b, n, g_Game.image[st]);
			if (st < NUM_STATIONS - 1) b[n++] = ' ';
		}
		b[n] = 0;
		Ui_Begin(); CFill(6, CY + 14, 76, 7, 1);
		Chrome_MicroW(7, (u8)(CY + 15 + CB), b);
		CEnd(6, CY + 14, 76, 7);
	}
	if (c_Day != g_Game.day || c_Sym != g_Game.sym[0] || c_Speed != g_Speed)    // TV da direita, linha 2: dia, Betty, velocidade
	{
		c_Day = g_Game.day; c_Sym = g_Game.sym[0]; c_Speed = g_Speed;
		n = 0;
		b[n++] = 'D'; b[n++] = 'A'; b[n++] = 'Y'; b[n++] = ' '; n = PutNum(b, n, (u8)g_Game.day);
		b[n++] = ' '; b[n++] = ' '; b[n++] = 'B'; b[n++] = 'E'; b[n++] = 'T'; b[n++] = 'T'; b[n++] = 'Y'; b[n++] = ' '; n = PutNum(b, n, g_Game.sym[0]);
		b[n++] = ' '; b[n++] = ' ';
		if (g_Speed == SPEED_PAUSE) { b[n++] = 'P'; b[n++] = 'A'; b[n++] = 'U'; b[n++] = 'S'; b[n++] = 'E'; }
		else { b[n++] = 'X'; b[n++] = (char)('0' + g_Speed); }
		b[n] = 0;
		Ui_Begin(); CFill(158, CY + 14, 92, 7, 1);
		Chrome_MicroW(158, (u8)(CY + 15 + CB), b);
		CEnd(158, CY + 14, 92, 7);
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
