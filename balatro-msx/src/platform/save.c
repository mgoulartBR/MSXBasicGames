// Run save in cartridge SRAM (see save.h). Fixed-code module: it switches bank 3, so it must not live in a banked segment's
// data window; it runs with interrupts disabled while the SRAM is mapped.
#include "msxgl.h"
#include "save.h"
#include "bgame.h"

#define BANK3_REG   (*(volatile u8*)0x7800)
#define SRAM        ((volatile u8*)0xA000)
#define SEG_TEXT    20                 // the segment normally mapped in bank 3 (see pvideo.h)
#define SAVE_VERSION 1

typedef struct { u8 magic[4]; u8 version, screen; u16 len, sum; } Hdr;     // followed by the Game bytes

static u8 s_bit;                        // bank-3 register value that maps the SRAM (0 = none)

static u16 fletcher(const u8* p, u16 n)
{
	u8 a = 1, b = 0;
	while (n--) { a = (u8)(a + *p++); b = (u8)(b + a); }
	return (u16)((u16)b << 8 | a);
}

u16 Save_Sum(void) { return fletcher((const u8*)&g, sizeof(Game)); }

void Save_Init(void)
{
	static const u8 bits[4] = { 0x20, 0x40, 0x80, 0x10 };      // candidate "SRAM enable" bits for a 256 KB (32 segment) ROM
	s_bit = 0;
	DisableInterrupt();
	for (u8 k = 0; k < 4 && !s_bit; k++)
	{
		BANK3_REG = bits[k];
		u8 a = SRAM[0];
		SRAM[0] = (u8)~a;
		if (SRAM[0] == (u8)~a) { SRAM[0] = a; s_bit = bits[k]; }
	}
	SET_BANK_SEGMENT(3, SEG_TEXT);
	EnableInterrupt();
}

bool Save_Available(void) { return s_bit != 0; }

static void map_in(void)  { DisableInterrupt(); BANK3_REG = s_bit; }
static void map_out(void) { SET_BANK_SEGMENT(3, SEG_TEXT); EnableInterrupt(); }

static bool valid(const Hdr* h)
{
	return h->magic[0] == 'B' && h->magic[1] == 'A' && h->magic[2] == 'L' && h->magic[3] == 'X' && h->version == SAVE_VERSION && h->len == sizeof(Game);
}

bool Save_Peek(SaveInfo* info)
{
	if (!s_bit) return FALSE;
	Hdr h; Game* sg = (Game*)((u8*)SRAM + sizeof(Hdr));
	map_in();
	for (u8 i = 0; i < sizeof(Hdr); i++) ((u8*)&h)[i] = SRAM[i];
	bool ok = valid(&h) && fletcher((const u8*)sg, sizeof(Game)) == h.sum;
	if (ok && info) { info->screen = h.screen; info->ante = sg->ante; info->deck = sg->deckId; info->stake = sg->stake; info->money = sg->money; }
	map_out();
	return ok;
}

bool Save_Load(u8* screen)
{
	if (!Save_Peek(0)) return FALSE;
	map_in();
	const u8* src = (const u8*)SRAM + sizeof(Hdr);
	u8* dst = (u8*)&g;
	for (u16 i = 0; i < sizeof(Game); i++) dst[i] = src[i];
	*screen = SRAM[5];
	map_out();
	return TRUE;
}

void Save_Write(u8 screen)
{
	if (!s_bit) return;
	Hdr h;
	h.magic[0] = 'B'; h.magic[1] = 'A'; h.magic[2] = 'L'; h.magic[3] = 'X';
	h.version = SAVE_VERSION; h.screen = screen; h.len = sizeof(Game); h.sum = Save_Sum();
	map_in();
	u8* dst = (u8*)SRAM + sizeof(Hdr);
	const u8* src = (const u8*)&g;
	for (u16 i = 0; i < sizeof(Game); i++) dst[i] = src[i];
	for (u8 i = 0; i < sizeof(Hdr); i++) SRAM[i] = ((const u8*)&h)[i];       // header last: a torn write leaves an invalid save
	map_out();
}

void Save_Erase(void)
{
	if (!s_bit) return;
	map_in();
	SRAM[0] = 0;
	map_out();
}
