// O predio (hub navegavel) e o escritorio do jogador. Compilado no segmento 9 (banco 2).
// Graficos: o predio e redesenhado proceduralmente (retangulos VDP) e as figuras sao sprites de hardware (arte propria 16x16,
// uma cor por linha - sprite mode 2). Os rivais circulam como decoracao (ainda sem efeito de jogo).
#include "app.h"
#include "data/art_hub.h"                      // ART_HUB: fundo do predio com arte local (segmentos 16-18); 0 = procedural

// ---------------------------------------------------------------- tabelas
#define NFLOORS 5
#define NROOMS  14
#define FY(k)   ((u8)(182 - 31 * (k)))          // linha dos pes (topo da laje) do andar k
#define SHAFT_X 128                              // centro do elevador
#define CODE_STUB   0                            // sala ainda nao implementada
#define CODE_LOCKED 1                            // escritorio de um rival

typedef struct { u8 floor; u8 x; u8 col; u8 scr; const char* lbl; const char* name; } Room;
static const Room k_Rooms[NROOMS] = {
	{ 0,  40, COLOR_DARK_GREEN,  SCR_PORTER, "PORT", "Porter - daily summary and tips" },
	{ 0,  84, COLOR_DARK_YELLOW, SCR_ARCHIVE, "ARCH", "Archive - sell movies" },
	{ 0, 172, COLOR_LIGHT_BLUE,  SCR_SHOP,   "SHOP", "Supermarket - buy gifts" },
	{ 1,  40, COLOR_MEDIUM_RED,  SCR_AGENCY, "FILM", "Film agency - buy movies" },
	{ 1,  84, COLOR_LIGHT_GREEN, SCR_ADS,    "ADS",  "Ad agency - sign contracts" },
	{ 1, 172, COLOR_CYAN,        SCR_NEWS,   "NEWS", "News room" },
	{ 2,  40, COLOR_DARK_YELLOW, SCR_SCRIPTS, "SCRP", "Script agency - buy scripts" },
	{ 2,  84, COLOR_MAGENTA,     SCR_STUDIO,  "STUD", "Studios - produce your own films" },
	{ 2, 172, COLOR_LIGHT_RED,  SCR_REALTOR, "REAL", "Realtor - towers, cable, satellite" },
	{ 3,  84, COLOR_LIGHT_YELLOW, SCR_OFFICE, "OFFC", "Your office - grid, ratings, save" },
	{ 3, 172, COLOR_MEDIUM_GREEN, SCR_FUN,   "FUN",  "FunTV office - spy on their schedule" },
	{ 3, 216, COLOR_CYAN,      SCR_SUN,    "SUN",  "SunTV office - spy on their schedule" },
	{ 4,  40, COLOR_DARK_RED,    SCR_BOSS,   "BOSS", "Mr. Raffer - credit" },
	{ 4, 172, COLOR_MAGENTA,     SCR_BETTY,  "BETY", "Betty's office - gifts, marriage" },
};
#define ROOM_OFFICE 9

// sprites (16x16, 2 bytes... formato MSX: 16 bytes coluna esquerda + 16 bytes coluna direita)
#include "data/sprites_art.h"                  // SPR_ART 1 = bonecos a partir da folha do usuario (local, ignorado pelo git); 0 = os desenhados a mao
#if !SPR_ART
#include "sprites_fig.h"                       // k_SprPat / k_FigShape / k_FigCol (tools/make_sprites.py)
#endif
static const u8 k_SprCar[32] = {
	0xFF,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xFF, 0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xFF };

#define NSPR 13                                   // planos 2-13 = 3 bonecos x (preench. cima, preench. baixo, contorno cima, contorno baixo), 14 = elevador; 0-1 = cursor do mouse (ui.c)
#define HIDE_Y SPR_HIDE_Y

// ---------------------------------------------------------------- estado (RAM fixa, inicializado em Building_Reset)
typedef struct { u8 x, y, floor, tfloor, tx, phase, frame, wait, vis; } Mover;   // phase: 0 parado, 1 ate o elevador, 2 subindo/descendo, 3 ate a porta
static Mover s_M[3];                             // 0 = jogador, 1 e 2 = rivais
static u8 s_CarY;                                // pes do elevador (linha)
u8 g_BldSel;                                     // sala sob o cursor (global p/ testes)
static u8 s_Pending;                             // jogador a caminho de uma sala (entra ao chegar)
static u8 s_Spr[NSPR * 4];
static u8 s_PrevX[3], s_Dir[3];                       // atributos dos sprites (Y, X, padrao, 0)

// ---------------------------------------------------------------- desenho estatico
#if ART_HUB
// copia um retangulo do fundo do predio (ROM, segmentos ART_HUB_SEG.., 64 linhas de 128 bytes por segmento) para a tela/buffer
static void ArtRect(u8 sx, u8 sy, u8 w, u8 h, u8 dx, u8 dy)
{
	u8 r, row;
	u8 prev = GET_BANK_SEGMENT(3);
	for (r = 0; r < h; r++)
	{
		row = (u8)(sy + r);
		SET_BANK_SEGMENT(3, (u8)(ART_HUB_SEG + (row >> 6)));
		Ui_BlitRow((const u8*)(0xA000 + ((u16)(row & 63) << 7) + (sx >> 1)), dx, (u8)(dy + r), (u8)(w >> 1));
	}
	SET_BANK_SEGMENT(3, prev);
}

static void DoorBody(u8 i)
{
	const Room* r = &k_Rooms[i];
	u8 fy = FY(r->floor), x = r->x;
	ArtRect((u8)(x - 14), (u8)(fy - 56), 28, 31, (u8)(x - 14), (u8)(fy - 30));       // fundo (porta, parede) desta celula
	if (i == g_BldSel)
	{
		Ui_Fill((u8)(x - 12), (u8)(fy - 29), 24, 8, COLOR_LIGHT_YELLOW);               // rotulo em destaque
		Ui_FillPx((u8)(x - 11), (u8)(fy - 23), 22, 1, COLOR_LIGHT_YELLOW);             // moldura em volta da porta
		Ui_FillPx((u8)(x - 11), (u8)(fy - 23), 1, 23, COLOR_LIGHT_YELLOW);
		Ui_FillPx((u8)(x + 10), (u8)(fy - 23), 1, 23, COLOR_LIGHT_YELLOW);
	}
	Ui_Color(UI_BLACK);
	Ui_Text((u8)(x - 12), (u8)(fy - 29), r->lbl);
}
#else
static void DoorBody(u8 i)
{
	const Room* r = &k_Rooms[i];
	u8 fy = FY(r->floor), x = r->x;
	Ui_Fill((u8)(x - 14), (u8)(fy - 30), 28, 31, COLOR_DARK_BLUE);
	Ui_Fill((u8)(x - 12), (u8)(fy - 22), 24, 22, r->col);
	Ui_Fill((u8)(x - 2), (u8)(fy - 14), 4, 14, COLOR_BLACK);                // vao da porta
	Ui_Color(i == g_BldSel ? UI_YELLOW : UI_WHITE);
	Ui_Text((u8)(x - 12), (u8)(fy - 30), r->lbl);
	if (i == g_BldSel)
	{
		Ui_Fill((u8)(x - 14), (u8)(fy - 24), 28, 1, COLOR_LIGHT_YELLOW);
		Ui_Fill((u8)(x - 14), (u8)(fy - 24), 2, 25, COLOR_LIGHT_YELLOW);
		Ui_Fill((u8)(x + 12), (u8)(fy - 24), 2, 25, COLOR_LIGHT_YELLOW);
	}
}
#endif

static void DoorRow(u8 i)
{
	const Room* r;
	if (i >= NROOMS) return;
	r = &k_Rooms[i];
	Ui_Begin();
	DoorBody(i);
	Ui_End((u8)(r->x - 14), (u8)(FY(r->floor) - 30), 28, 31);
}

static void BodyAll(void)
{
	u8 k, i;
#if ART_HUB
	(void)k;
	{
		u8 r, row, prev = GET_BANK_SEGMENT(3);
		for (r = 0; r < ART_HUB_ROWS; r++)                                    // fundo completo: 160 linhas x 128 bytes
		{
			row = r;
			SET_BANK_SEGMENT(3, (u8)(ART_HUB_SEG + (row >> 6)));
			Ui_BlitRow((const u8*)(0xA000 + ((u16)(row & 63) << 7)), 0, (u8)(CONTENT_Y - 2 + r), 128);
		}
		SET_BANK_SEGMENT(3, prev);
	}
	for (i = 0; i < NROOMS; i++) { const Room* rm = &k_Rooms[i]; Ui_Color(UI_BLACK); Ui_Text((u8)(rm->x - 12), (u8)(FY(rm->floor) - 29), rm->lbl); }
	DoorBody(g_BldSel);
#else
	ClearContent();
	Ui_Fill(6, 26, 244, 3, COLOR_GRAY);                                       // telhado
	Ui_Fill(6, 29, 244, 157, COLOR_DARK_BLUE);                                // fachada
	Ui_Fill(118, 29, 20, 157, COLOR_BLACK);                                   // poco do elevador
	Ui_Fill(118, 29, 2, 157, COLOR_GRAY); Ui_Fill(136, 29, 2, 157, COLOR_GRAY);   // largura >= 2: HMMV com 1 pixel vira 0 bytes = linha inteira!
	for (k = 0; k < NFLOORS; k++)
	{
		Ui_Fill(6, (u8)(FY(k) + 1), 112, 3, COLOR_GRAY);                      // lajes (interrompidas pelo poco)
		Ui_Fill(138, (u8)(FY(k) + 1), 112, 3, COLOR_GRAY);
	}
	for (i = 0; i < NROOMS; i++) DoorBody(i);
#endif
}

// ---------------------------------------------------------------- sprites
static void SprWrite(void)
{
	u8 i;
	// jogador/rivais: Y = pes-16 (atributo = linha visivel - 1); invisivel -> fora da tela
	for (i = 0; i < 3; i++)
	{
		u8 y = s_M[i].vis ? (u8)(s_M[i].y - 23 - YSHIFT) : HIDE_Y, x = (u8)(s_M[i].x - 8), k, pn, *a = &s_Spr[i * 16];
		if (x > s_PrevX[i]) s_Dir[i] = 0; else if (x < s_PrevX[i]) s_Dir[i] = 1;          // direcao da ultima caminhada (0 = direita)
		s_PrevX[i] = x;
		pn = (u8)(8 + (((k_FigShape[i] * 2 + s_Dir[i]) * 2 + (s_M[i].frame ? 1 : 0)) * 4));  // 1o padrao (16x16: numero = 4 x posicao) do boneco
		for (k = 0; k < 4; k++)                                                          // 0 preench. cima, 1 preench. baixo, 2 contorno cima, 3 contorno baixo
		{
			a[k * 4]     = (y == HIDE_Y) ? HIDE_Y : (u8)(y + ((k & 1) ? 16 : 0));
			a[k * 4 + 1] = x; a[k * 4 + 2] = (u8)((pn + k) * 4); a[k * 4 + 3] = 0;
		}
	}
#if ART_HUB
	s_Spr[48] = HIDE_Y;                                           // o fundo ja tem as portas do elevador
#else
	s_Spr[48] = (u8)(s_CarY - 16 - YSHIFT);
#endif
	s_Spr[49] = (u8)(SHAFT_X - 8); s_Spr[50] = 8; s_Spr[51] = 0;
	VDP_WriteVRAM_128K(s_Spr, SPR_ATT_LO + 8, 1, NSPR * 4);
}

static void SprInit(void)
{
	u8 col[16], i, r;
	VDP_WriteVRAM_128K(k_SprCar, SPR_PAT_LO + 64, 1, 32);
	VDP_WriteVRAM_128K((const u8*)k_SprPat, SPR_PAT_LO + 256, 1, (u16)sizeof(k_SprPat));         // padroes dos bonecos a partir do slot 8 (offset 256)
	for (i = 0; i < 3; i++)
	{
		const u8* c = k_FigCol[i];
		VDP_WriteVRAM_128K(c, SPR_COL_LO + (2 + i * 4) * 16, 1, 16);                              // preench. cima: linhas 0-15
		for (r = 0; r < 16; r++) col[r] = (r < 8) ? c[16 + r] : 0;
		VDP_WriteVRAM_128K(col, SPR_COL_LO + (3 + i * 4) * 16, 1, 16);                            // preench. baixo: linhas 16-23
		for (r = 0; r < 16; r++) col[r] = COLOR_BLACK;
		VDP_WriteVRAM_128K(col, SPR_COL_LO + (4 + i * 4) * 16, 1, 16);
		VDP_WriteVRAM_128K(col, SPR_COL_LO + (5 + i * 4) * 16, 1, 16);                            // contornos pretos
		s_PrevX[i] = 0; s_Dir[i] = 0;
	}
	for (r = 0; r < 16; r++) col[r] = COLOR_GRAY;
	VDP_WriteVRAM_128K(col, SPR_COL_LO + 14 * 16, 1, 16);                                         // elevador (plano 14)
}

// ---------------------------------------------------------------- movimento
static void Travel(Mover* m, u8 floor, u8 x)
{
	m->tfloor = floor; m->tx = x;
	m->phase = (m->floor == floor) ? 3 : 1;
}

static u8 Step(u8* v, u8 target, u8 speed)       // anda `speed` px em direcao a target; 1 = chegou
{
	if (*v < target) { *v = (u8)((target - *v <= speed) ? target : *v + speed); }
	else if (*v > target) { *v = (u8)((*v - target <= speed) ? target : *v - speed); }
	return *v == target;
}

static u8 MoveOne(Mover* m, u8 is_player)        // retorna 1 quando chega ao destino
{
	u8 arrived = 0;
	switch (m->phase)
	{
	case 1: if (Step(&m->x, SHAFT_X, 2)) { m->phase = 2; s_CarY = m->y; } break;
	case 2:
		if (Step(&m->y, FY(m->tfloor), 2)) { m->floor = m->tfloor; m->phase = 3; }
		if (is_player) s_CarY = m->y;
		break;
	case 3: if (Step(&m->x, m->tx, 2)) { m->phase = 0; arrived = 1; } break;
	}
	if (m->phase) { if ((++m->wait & 7) == 0) m->frame ^= 1; } else m->frame = 0;
	return arrived;
}

static void RivalThink(Mover* m)                 // decoracao: anda ate uma porta do andar, "entra" (some) e reaparece em outra sala
{
	if (m->phase) return;
	if (!m->vis)                                  // dentro de uma sala
	{
		if (m->wait) { m->wait--; return; }
		{
			u8 r = Sim_Rnd(NROOMS);
			m->floor = k_Rooms[r].floor; m->x = k_Rooms[r].x; m->y = FY(m->floor); m->vis = 1; m->wait = 0;
		}
		return;
	}
	if (m->wait) { m->wait--; return; }
	{
		u8 tries, r = NROOMS;
		for (tries = 0; tries < 8; tries++) { u8 c = Sim_Rnd(NROOMS); if (k_Rooms[c].floor == m->floor && k_Rooms[c].x != m->x) { r = c; break; } }
		if (r == NROOMS) { m->vis = 0; m->wait = 60; return; }
		Travel(m, m->floor, k_Rooms[r].x);
	}
}

// ---------------------------------------------------------------- pontos de entrada
void Building_Reset(void) __banked
{
	u8 i;
	for (i = 0; i < 3; i++)
	{
		s_M[i].x = (i == 0) ? SHAFT_X : k_Rooms[i == 1 ? 3 : 9].x;
		s_M[i].floor = (i == 0) ? 0 : k_Rooms[i == 1 ? 3 : 9].floor;
		s_M[i].y = FY(s_M[i].floor); s_M[i].tfloor = s_M[i].floor; s_M[i].tx = s_M[i].x;
		s_M[i].phase = 0; s_M[i].frame = 0; s_M[i].wait = 20 + i * 70; s_M[i].vis = 1;
	}
	s_CarY = FY(0); g_BldSel = ROOM_OFFICE; s_Pending = 0;
	SprInit();
}

void Building_Enter(void) __banked { SprWrite(); }
void Building_Leave(void) __banked        // esconde so os sprites do predio (o cursor do mouse continua)
{
	u8 hide[NSPR * 4], i;
	for (i = 0; i < NSPR * 4; i += 4) { hide[i] = HIDE_Y; hide[i + 1] = 0; hide[i + 2] = 0; hide[i + 3] = 0; }
	VDP_WriteVRAM_128K(hide, SPR_ATT_LO + 8, 1, NSPR * 4);
}

void Building_Draw(void) __banked
{
	Ui_Begin();
	BodyAll();
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Building_Enter();
	Building_Detail();                                           // dica com o nome da sala (desenhada direto na tela)
}

void Building_Row(u8 r) __banked { DoorRow(r); }

void Building_Detail(void) __banked
{
	char buf[44];
	u8 i = 0;
	const char* n = k_Rooms[g_BldSel].name;
	while (n[i] && i < 40) { buf[i] = n[i]; i++; }
	buf[i] = 0;
	Hint(buf);
}

void Building_Dyn(void) __banked { }

void Building_Input(u8 ev) __banked
{
	u8 old = g_BldSel, i, best = NROOMS, bd = 255, fl = k_Rooms[g_BldSel].floor, x = k_Rooms[g_BldSel].x;
	if ((ev & IN_LEFT) && g_BldSel > 0 && k_Rooms[g_BldSel - 1].floor == fl) g_BldSel--;
	if ((ev & IN_RIGHT) && g_BldSel + 1 < NROOMS && k_Rooms[g_BldSel + 1].floor == fl) g_BldSel++;
	if (ev & (IN_UP | IN_DOWN))
	{
		u8 tf = (ev & IN_UP) ? (u8)(fl + 1) : (u8)(fl - 1);
		if (tf < NFLOORS)
			for (i = 0; i < NROOMS; i++)
				if (k_Rooms[i].floor == tf)
				{
					u8 d = (k_Rooms[i].x > x) ? (u8)(k_Rooms[i].x - x) : (u8)(x - k_Rooms[i].x);
					if (d < bd) { bd = d; best = i; }
				}
		if (best < NROOMS) g_BldSel = best;
	}
	if (g_BldSel != old) { MarkRows(old, g_BldSel); g_Dirty |= D_DET; }
	if (ev & IN_OK)
	{
		Travel(&s_M[0], k_Rooms[g_BldSel].floor, k_Rooms[g_BldSel].x);
		s_Pending = 1;
		if (s_M[0].phase == 3 && s_M[0].x == s_M[0].tx) { s_M[0].phase = 0; }   // ja esta na porta
	}
}

// Chamada a cada frame com o predio na tela. Retorna a tela a abrir (SCR_*) ou 0xFF.
u8 Building_Frame(void) __banked
{
	u8 arrived, ret = 0xFF;
	arrived = MoveOne(&s_M[0], 1);
	if (s_Pending && (arrived || (s_M[0].phase == 0 && s_M[0].floor == k_Rooms[g_BldSel].floor && s_M[0].x == k_Rooms[g_BldSel].x)))
	{
		const Room* r = &k_Rooms[g_BldSel];
		s_Pending = 0;
		if (r->scr == CODE_STUB) { Sim_Msg("Closed - not part of this version yet."); g_Dirty |= D_MSG; }
		else if (r->scr == CODE_LOCKED) { Sim_Msg("The door is locked. Rivals keep out visitors."); g_Dirty |= D_MSG; }
		else ret = r->scr;
	}
	RivalThink(&s_M[1]); RivalThink(&s_M[2]);
	if (s_M[1].phase) MoveOne(&s_M[1], 0);
	if (s_M[2].phase) MoveOne(&s_M[2], 0);
	if (ret == 0xFF) SprWrite();
	return ret;
}

// ---------------------------------------------------------------- ESCRITORIO (submenu: grade, audiencias, salvar/carregar)
static const char* const k_OfficeItems[] = { "Programme grid (TV computer)", "Ratings & image (graphs)", "Save / Load (wall picture)", "Options (sound, mouse speed)" };
static const u8 k_OfficeScr[] = { SCR_GRID, SCR_RATINGS, SCR_SLOTS, SCR_OPTIONS };
#define OFFICE_N 4

#if ART_SCENES
#define OFF_RY(i) ((u8)(142 + (i) * ROW_H))              // menu num painel cinza embaixo da cena do escritorio
static void OfficeRowBody(u8 i)
{
	u8 y = OFF_RY(i);
	Ui_Fill(4, y - 1, 248, ROW_H, 14);
	if (i == g_Sel) Chrome_Box(4, y - 1, 248, ROW_H, 5, 0);
	Ui_Color(UI_BLACK);
	Ui_Text(12, y, k_OfficeItems[i]);
}
#else
#define OFF_RY(i) ((u8)(CONTENT_Y + 24 + (i) * ROW_H))
static void OfficeRowBody(u8 i)
{
	u8 y = OFF_RY(i);
	Ui_Fill(0, y - 1, 255, ROW_H, UI_BG);
	Ui_Color(i == g_Sel ? UI_YELLOW : UI_WHITE);
	Ui_Text(4, y, i == g_Sel ? ">" : " ");
	Ui_Text(14, y, k_OfficeItems[i]);
}
#endif

void Office_Enter(void) __banked { }
void Office_Row(u8 r) __banked
{
	if (r >= OFFICE_N) return;
	Ui_Begin(); OfficeRowBody(r); Ui_End(0, (u8)(OFF_RY(r) - 1), 255, ROW_H);
}

void Office_Draw(void) __banked
{
	u8 i;
	Ui_Begin();
#if ART_SCENES
	Scene_Draw(SCN_OFFICE, 0, CONTENT_Y - 2);
	Chrome_Box(0, 138, 256, 48, 14, 1);
	for (i = 0; i < OFFICE_N; i++) OfficeRowBody(i);
	Ui_End(0, CONTENT_Y - 2, 0, 160);
	Hint("OK:use  BACK:corridor");
	return;
#endif
	ClearContent();
	Ui_Color(UI_YELLOW); Ui_Text(4, CONTENT_Y, "Your office");
	for (i = 0; i < OFFICE_N; i++) OfficeRowBody(i);
	Ui_Color(UI_GRAY);
	Ui_Text(4, (u8)(CONTENT_Y + 74), "Tower map & bank statement: not open yet.");
	Ui_End(0, CONTENT_Y - 2, 255, 160);
	Hint("OK:use  BACK:corridor");
}

void Office_Input(u8 ev) __banked
{
	u8 old = g_Sel;
	if ((ev & IN_DOWN) && g_Sel + 1 < OFFICE_N) g_Sel++;
	if ((ev & IN_UP) && g_Sel > 0) g_Sel--;
	if (g_Sel != old) MarkRows(old, g_Sel);
	if (ev & IN_BACK) { Goto(SCR_HUB); return; }
	if (ev & IN_OK) Goto(k_OfficeScr[g_Sel]);
}

// ---------------------------------------------------------------- mouse
void Building_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 i;
	for (i = 0; i < NROOMS; i++)
	{
		const Room* r = &k_Rooms[i];
		u8 fy = FY(r->floor), dx = (x > r->x) ? (u8)(x - r->x) : (u8)(r->x - x);
		if (dx <= 14 && y >= fy - 30 && y <= fy + 3)
		{
			if (i != g_BldSel) { u8 old = g_BldSel; g_BldSel = i; MarkRows(old, i); g_Dirty |= D_DET; }
			if (btn) Building_Input(IN_OK);
			return;
		}
	}
}

void Office_Mouse(u8 x, u8 y, u8 btn) __banked
{
	u8 r = HitRow(y, (u8)(OFF_RY(0) - 1), ROW_H, OFFICE_N);
	(void)x;
	if (r == 0xFF) return;
	if (r != g_Sel) { u8 old = g_Sel; g_Sel = r; MarkRows(old, r); }
	if (btn) Office_Input(IN_OK);
}
