#include "ui.h"
#include "font/font_mgl_sample6.h"

#define FONT_SRC_Y  212          // fonte branca desempacotada em page 0, linhas invisiveis (212..251)
#define FONT_Y0     256          // variantes coloridas na pagina 1 da VRAM
#define FONT_H      40           // 5 linhas de 8 px (192 chars / 42 por linha)
#define GLYPH_W     6
#define GLYPH_H     8
#define GLYPHS_ROW  42           // 256 / GLYPH_W
static const u8 s_Col[UI_NUM_COLORS] = {
	COLOR_WHITE, COLOR_LIGHT_YELLOW, COLOR_LIGHT_RED, COLOR_GRAY, COLOR_LIGHT_GREEN, COLOR_CYAN
};
#define BUF_Y 512                // pagina 2 = buffer de composicao fora da tela
static u16 s_YOff;               // 0 = desenha na tela; BUF_Y = desenha no buffer
static u8  s_Depth;
static u16 s_FontY;
static u8  s_PrevIn;
static u8  s_TypeHeld[5];        // bitmap de teclas ja tratadas (borda). NAO ha zeragem de BSS no crt0: tudo e inicializado em Ui_Init

// formato (MSXimg): 4 bytes de cabecalho (tamanho dos dados, tamanho da fonte, 1o e ultimo caractere) + 8 bytes por glifo, bit 7 = pixel mais a esquerda
static void FontUnpack(void)
{
	const u8* d = g_Font_MGL_Sample6 + 4;
	u8 g, r, row[3];
	for (g = 0; g < 192; g++)
	{
		u8 gx = (u8)((g % GLYPHS_ROW) * GLYPH_W);
		u16 y0 = FONT_SRC_Y + (u16)(g / GLYPHS_ROW) * GLYPH_H;
		for (r = 0; r < GLYPH_H; r++, d++)
		{
			u8 b = *d;
			row[0] = (u8)(((b & 0x80) ? 0xF0 : 0) | ((b & 0x40) ? 0x0F : 0));
			row[1] = (u8)(((b & 0x20) ? 0xF0 : 0) | ((b & 0x10) ? 0x0F : 0));
			row[2] = (u8)(((b & 0x08) ? 0xF0 : 0) | ((b & 0x04) ? 0x0F : 0));
			VDP_WriteVRAM_128K(row, (u16)((y0 + r) * 128 + gx / 2), 0, 3);
		}
	}
}

void Ui_Init(void)
{
	u8 i;
	s_YOff = 0; s_Depth = 0; s_PrevIn = 0; s_FontY = 0;
	{ u8 k; for (k = 0; k < 5; k++) s_TypeHeld[k] = 0; }
	VDP_SetMode(VDP_MODE_SCREEN5);
	VDP_SetColor(UI_BG);
	VDP_EnableVBlank(TRUE);
	VDP_EnableSprite(FALSE);   // tabelas de sprite (0x7400-0x7A00) coincidem com a area da fonte em VRAM
	VDP_ClearVRAM();
	// 1) desempacota a fonte (1 bit/pixel, glifos 6x8) em branco nas linhas 212.. (so aqui, uma vez; sem o modulo Print do MSXgl)
	FontUnpack();
	// 2) variantes de cor: preenche destino com a cor e aplica AND com a fonte branca (15 AND c = c; 0 AND c = 0)
	for (i = 0; i < UI_NUM_COLORS; i++)
	{
		u16 y = FONT_Y0 + (u16)i * FONT_H;
		VDP_CommandHMMV(0, y, 252, FONT_H, COLOR_MERGE2(s_Col[i]));
		VDP_CommandLMMM(0, FONT_SRC_Y, 0, y, 252, FONT_H, VDP_OP_AND);
	}
	Ui_Fill(0, 0, 255, 212, UI_BG); // remove restos da carga da fonte na area visivel
	Ui_Color(UI_WHITE);
}

void Ui_Color(u8 c)
{
	s_FontY = FONT_Y0 + (u16)c * FONT_H;
}

void Ui_Text(u8 x, u8 y, const char* s)
{
	u8 c, row;
	for (; (c = (u8)*s) != 0; s++, x += GLYPH_W)
	{
		if (c == ' ') continue;
		row = 0;
		while (c >= GLYPHS_ROW) { c -= GLYPHS_ROW; row++; }
		VDP_CommandLMMM((u16)c * GLYPH_W, s_FontY + (u16)row * GLYPH_H, x, y + s_YOff, GLYPH_W, GLYPH_H, VDP_OP_TIMP);
	}
}

void Ui_TextN(u8 x, u8 y, const char* s, u8 maxc)
{
	char buf[44];
	u8 i = 0;
	while (s[i] && i < maxc && i < sizeof(buf) - 1) { buf[i] = s[i]; i++; }
	buf[i] = 0;
	Ui_Text(x, y, buf);
}

void Ui_Dec1(u8 x, u8 y, u8 v)
{
	char b[5];
	u8 i = 0;
	if (v >= 100) b[i++] = '0' + v / 100 % 10;
	b[i++] = '0' + (v / 10) % 10; b[i++] = '.'; b[i++] = '0' + v % 10; b[i] = 0;
	Ui_Text(x, y, b);
}

void Ui_TextR(u8 x, u8 y, const char* s)
{
	u8 n = 0;
	while (s[n]) n++;
	Ui_Text((u8)(x - n * GLYPH_W), y, s);
}

u8 Ui_Int(u8 x, u8 y, i16 v)
{
	char buf[7];
	u8 i = 6;
	u16 u = (v < 0) ? (u16)(-v) : (u16)v;
	buf[6] = 0;
	do { buf[--i] = '0' + (u % 10); u /= 10; } while (u);
	if (v < 0) buf[--i] = '-';
	Ui_Text(x, y, &buf[i]);
	return (u8)(x + (6 - i) * GLYPH_W);
}

void Ui_Fill(u8 x, u8 y, u8 w, u8 h, u8 col)
{
	// HMMV em Screen 5 conta bytes (2 pixels): w < 2 viraria 0 bytes = a linha inteira. Sempre use w >= 2.
	if (w < 2) w = 2;
	VDP_CommandHMMV(x, y + s_YOff, w, h, COLOR_MERGE2(col));
}

void Ui_Begin(void)
{
	s_Depth++;
	s_YOff = BUF_Y;
}

void Ui_End(u8 x, u8 y, u8 w, u8 h)
{
	if (s_Depth) s_Depth--;
	if (s_Depth) return;
	s_YOff = 0;
	VDP_CommandHMMM(x, y + BUF_Y, x, y, ((u16)w + 1) & 0xFFFE, h);   // HMMM trabalha em bytes (2 pixels): largura par
}

u16 Ui_DirectBegin(void)
{
	u16 s = s_YOff;
	s_YOff = 0;
	return s;
}

void Ui_DirectEnd(u16 saved)
{
	s_YOff = saved;
}

void Ui_Clear(void)
{
	Ui_Fill(0, 0, 255, 212, UI_BG);
}

// barra horizontal de largura w (par), preenchida pct% - fundo azul escuro
void Ui_Bar(u8 x, u8 y, u8 w, u8 h, u8 pct, u8 col)
{
	u8 f = (u8)(((u16)w * pct / 100) & 0xFE);
	Ui_Fill(x, y, w, h, UI_PANEL);
	if (f) Ui_Fill(x, y, f, h, col);
}

u8 Input_Poll(void)
{
	u8 in = 0, joy = Joystick_Read(JOY_PORT_1), pushed;
	if (Keyboard_IsKeyPressed(KEY_UP)    || !(joy & JOY_INPUT_DIR_UP))    in |= IN_UP;
	if (Keyboard_IsKeyPressed(KEY_DOWN)  || !(joy & JOY_INPUT_DIR_DOWN))  in |= IN_DOWN;
	if (Keyboard_IsKeyPressed(KEY_LEFT)  || !(joy & JOY_INPUT_DIR_LEFT))  in |= IN_LEFT;
	if (Keyboard_IsKeyPressed(KEY_RIGHT) || !(joy & JOY_INPUT_DIR_RIGHT)) in |= IN_RIGHT;
	if (Keyboard_IsKeyPressed(KEY_RET) || Keyboard_IsKeyPressed(KEY_SPACE) || !(joy & JOY_INPUT_TRIGGER_A)) in |= IN_OK;
	if (Keyboard_IsKeyPressed(KEY_ESC) || !(joy & JOY_INPUT_TRIGGER_B)) in |= IN_BACK;
	if (Keyboard_IsKeyPressed(KEY_TAB)) in |= IN_SPEED;
	if (Keyboard_IsKeyPressed(KEY_P))   in |= IN_PAUSE;
	pushed = in & ~s_PrevIn;
	s_PrevIn = in;
	return pushed;
}

static const u8 k_TypeKeys[37] = {
	KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
	KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
	KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_BS
};

u8 Input_TypedChar(void)
{
	u8 i, ret = 0;
	for (i = 0; i < 37; i++)
	{
		u8 mask = (u8)(1 << (i & 7)), *h = &s_TypeHeld[i >> 3];
		if (Keyboard_IsKeyPressed(k_TypeKeys[i])) { if (!(*h & mask)) { *h |= mask; if (!ret) ret = (i < 10) ? (u8)('0' + i) : (i < 36) ? (u8)('A' + i - 10) : 8; } }
		else *h &= (u8)~mask;
	}
	return ret;
}
