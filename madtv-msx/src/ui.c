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
	Ui_SpriteSetup();
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
	u8 in = 0, joy = Joystick_Read(g_PtrMode == 1 ? JOY_PORT_2 : JOY_PORT_1), pushed;
	if (Keyboard_IsKeyPressed(KEY_UP)    || !(joy & JOY_INPUT_DIR_UP))    in |= IN_UP;
	if (Keyboard_IsKeyPressed(KEY_DOWN)  || !(joy & JOY_INPUT_DIR_DOWN))  in |= IN_DOWN;
	if (Keyboard_IsKeyPressed(KEY_LEFT)  || !(joy & JOY_INPUT_DIR_LEFT))  in |= IN_LEFT;
	if (Keyboard_IsKeyPressed(KEY_RIGHT) || !(joy & JOY_INPUT_DIR_RIGHT)) in |= IN_RIGHT;
	if (Keyboard_IsKeyPressed(KEY_RET) || Keyboard_IsKeyPressed(KEY_SPACE) || !(joy & JOY_INPUT_TRIGGER_A)) in |= IN_OK;
	if (Keyboard_IsKeyPressed(KEY_ESC) || !(joy & JOY_INPUT_TRIGGER_B)) in |= IN_BACK;
	if (Keyboard_IsKeyPressed(KEY_TAB)) in |= IN_SPEED;
	if (Keyboard_IsKeyPressed(KEY_P))   in |= IN_PAUSE;
	{ static u8 mHeld; u8 m = Keyboard_IsKeyPressed(KEY_M); g_InExtra = (m && !mHeld) ? 1 : 0; mHeld = m; }
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

// ---------------------------------------------------------------- ponteiro (mouse) e sprites comuns
u8 g_PtrX, g_PtrY, g_PtrMode, g_PtrInject, g_InExtra;
static Mouse_State s_Mouse;
static u8 s_PtrShown;

// seta 16x16 (branca) e contorno (preto): arte propria. Hotspot = ponta da seta em (2,2) do sprite.
static const u8 k_SprArrow[32] = { 0x00,0x00,0x20,0x30,0x38,0x3C,0x3E,0x3F,0x3F,0x3F,0x3E,0x36,0x23,0x03,0x01,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0x00,0x00,0x00,0x00,0x80,0x00 };
static const u8 k_SprOutline[32] = { 0x00,0x70,0x78,0x7C,0x7E,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x77,0x07,0x03, 0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xE0,0xE0,0x80,0x80,0xC0,0xC0,0xC0 };

static void PtrDraw(void)
{
	u8 a[8], hide = (g_PtrMode == 0);
	a[0] = hide ? SPR_HIDE_Y : (u8)(g_PtrY - 2); a[1] = (u8)(g_PtrX - 2); a[2] = 12; a[3] = 0;     // seta (padrao 12 = offset 96)
	a[4] = a[0]; a[5] = a[1]; a[6] = 16; a[7] = 0;                                                    // contorno (padrao 16 = offset 128)
	VDP_WriteVRAM_128K(a, SPR_ATT_LO, 1, 8);
}

void Ui_SpriteSetup(void)
{
	u8 col[16], i, hide[24];
	VDP_WriteVRAM_128K(k_SprArrow, SPR_PAT_LO + 96, 1, 32);
	VDP_WriteVRAM_128K(k_SprOutline, SPR_PAT_LO + 128, 1, 32);
	for (i = 0; i < 16; i++) col[i] = COLOR_WHITE;
	VDP_WriteVRAM_128K(col, SPR_COL_LO + 0, 1, 16);          // sprite 0 = seta branca
	for (i = 0; i < 16; i++) col[i] = COLOR_BLACK;
	VDP_WriteVRAM_128K(col, SPR_COL_LO + 16, 1, 16);         // sprite 1 = contorno preto
	for (i = 0; i < 24; i += 4) { hide[i] = SPR_HIDE_Y; hide[i + 1] = 0; hide[i + 2] = 0; hide[i + 3] = 0; }
	VDP_WriteVRAM_128K(hide, SPR_ATT_LO + 8, 1, 24);         // sprites do predio comecam escondidos
	VDP_RegWrite(5, 0xF7); VDP_RegWrite(11, 3); VDP_RegWrite(6, 0x3E);       // tabelas na pagina 3 da VRAM
	VDP_RegWriteBakMask(1, (u8)~(R01_ST | R01_MAG), R01_ST);                 // sprites 16x16
	g_PtrX = 128; g_PtrY = 106; g_PtrMode = 0; g_PtrInject = 0; g_InExtra = 0; s_PtrShown = 0;
	s_Mouse.Buttons = 0xFF; s_Mouse.PrevButtons = 0xFF; s_Mouse.dX = 0; s_Mouse.dY = 0;
	PtrDraw();
	VDP_EnableSprite(TRUE);
}

void Pointer_Cycle(void)
{
	g_PtrMode = (g_PtrMode + 1) % 3;
	s_Mouse.Buttons = 0xFF; s_Mouse.PrevButtons = 0xFF;
	PtrDraw();
}

u8 Pointer_Update(void)
{
	u8 ret = g_PtrInject;                 // gancho de teste: cliques injetados (g_PtrInject e zerado aqui)
	i16 nx, ny;
	g_PtrInject = 0;
	if (g_PtrMode == 0) return ret;
	Mouse_Read(g_PtrMode == 1 ? MOUSE_PORT_1 : MOUSE_PORT_2, &s_Mouse);
	nx = (i16)g_PtrX + Mouse_GetOffsetX(&s_Mouse);
	ny = (i16)g_PtrY + Mouse_GetOffsetY(&s_Mouse);
	if (nx < 2) nx = 2; if (nx > 253) nx = 253;
	if (ny < 2) ny = 2; if (ny > 209) ny = 209;
	if ((u8)nx != g_PtrX || (u8)ny != g_PtrY) { g_PtrX = (u8)nx; g_PtrY = (u8)ny; ret |= PTR_MOVED; }
	if (Mouse_IsButtonClick(&s_Mouse, MOUSE_BOUTON_LEFT)) ret |= PTR_LEFT;
	if (Mouse_IsButtonClick(&s_Mouse, MOUSE_BOUTON_RIGHT)) ret |= PTR_RIGHT;
	if (ret & PTR_MOVED) PtrDraw();
	return ret;
}
