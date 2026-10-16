// Real-hardware diagnostic build (-DRT_DIAG): measures the cartridge / VDP / RAM / interrupts
// instead of running the game, and prints a report to photograph.  See scripts/build_diag.sh.
typedef int rt_diag_placeholder; // keeps the translation unit non-empty in release builds
#ifdef RT_DIAG
#include "msxgl.h"
#include "platform/msx_gfx.h"
#include "platform/msx_seg.h"
#include "platform/msx_isr.h"
#include "game_data.h"

#ifndef EXP4
#define EXP4 0
#define EXP5 0
#define EXP6 0
#define EXPF 0
#endif

static char s_Buf[40];

static char* hex16(char* p, u16 v)
{
	static const char h[] = "0123456789ABCDEF";
	*p++ = h[(v >> 12) & 15]; *p++ = h[(v >> 8) & 15]; *p++ = h[(v >> 4) & 15]; *p++ = h[v & 15];
	return p;
}

static char* str(char* p, const char* s)
{
	while (*s) *p++ = *s++;
	return p;
}

static char* dec(char* p, u16 v)
{
	char t[6]; u8 n = 0;
	do { t[n++] = '0' + v % 10; v /= 10; } while (v);
	while (n) *p++ = t[--n];
	return p;
}

static void out(u8 row, const char* s, u8 color)
{
	Gfx_Text(0, row, 32, s, color);
}

static u16 win_sum(u8 seg)
{
	const volatile u8* p = (const volatile u8*)0xA000;
	u16 s = 0;
	SET_BANK_SEGMENT(SEG_WINDOW_BANK, seg);
	for (u16 i = 0; i < 0x2000; ++i)
		s += p[i];
	return s;
}

// One result line: "NAME  got exp  OK|FAIL"
static bool seg_line(u8 row, u8 seg, u16 exp, const char* tag)
{
	u16 a = win_sum(seg), b = win_sum(seg), c = win_sum(seg);
	bool ok = (a == exp) && (b == exp) && (c == exp);
	char* p = s_Buf;
	p = str(p, tag); p = str(p, " seg"); *p++ = '0' + seg; *p++ = ' ';
	p = hex16(p, a); *p++ = ' '; p = hex16(p, b); *p++ = ' '; p = hex16(p, c);
	p = str(p, " exp "); p = hex16(p, exp); *p++ = ' '; p = str(p, ok ? "OK" : "FAIL"); *p = 0;
	out(row, s_Buf, ok ? CT_INK : CT_INV);
	return ok;
}

static u16 vram_test(u8 display)
{
	static u8 wbuf[128];
	static u8 rbuf[128];
	u16 bad = 0;
	u16 lfsr = 0xACE1;
	Gfx_Display(display);
	for (u8 it = 0; it < 24; ++it)
	{
		for (u8 i = 0; i < 128; ++i)
		{
			lfsr ^= lfsr << 7; lfsr ^= lfsr >> 9; lfsr ^= lfsr << 8;
			wbuf[i] = (u8)lfsr;
		}
		VDP_WriteVRAM_16K(wbuf, 0x3C00, 128);
		VDP_ReadVRAM_16K(0x3C00, rbuf, 128);
		for (u8 i = 0; i < 128; ++i)
			if (wbuf[i] != rbuf[i])
				++bad;
	}
	Gfx_Display(TRUE);
	return bad;
}

static u16 ram_test(void)
{
	volatile u8* p = (volatile u8*)0xE000; // well above the program data + heap (ends ~DA70), below the stack (~F300)
	u16 bad = 0;
	for (u16 i = 0; i < 0x1000; ++i) { p[i] = 0x55; }
	for (u16 i = 0; i < 0x1000; ++i) { if (p[i] != 0x55) ++bad; p[i] = 0xAA; }
	for (u16 i = 0; i < 0x1000; ++i) { if (p[i] != 0xAA) ++bad; p[i] = (u8)(i ^ (i >> 8)); }
	for (u16 i = 0; i < 0x1000; ++i) { if (p[i] != (u8)(i ^ (i >> 8))) ++bad; }
	return bad;
}

void Diag_Main(void)
{
	u16 n;
	bool ok;
	char* p;
	Gfx_Clear(CT_INK);
	Gfx_TextCenter(0, 0, 32, "REPUBLIA DIAG v0.3", CT_INV);
#if (ROM_MAPPER == ROM_KONAMI)
	out(1, "build: KONAMI mapper", CT_INK);
#else
	out(1, "build: ASCII8 mapper", CT_INK);
#endif

	// 1. window checksums (3 reads each, interrupts still off)
	out(3, "1 ROM window (sum, 3 reads, exp)", CT_INK);
	ok = seg_line(4, 4, EXP4, "");
	ok &= seg_line(5, 5, EXP5, "");
	ok &= seg_line(6, 6, EXP6, "");
	// 2. fixed ROM data
	{
		u16 s = 0;
		const u8* f = g_Font_Silk;
		for (u16 i = 0; i < 95 * 9; ++i) s += f[i];
		p = s_Buf; p = str(p, "2 fixed font sum "); p = hex16(p, s); p = str(p, " exp "); p = hex16(p, EXPF);
		p = str(p, s == EXPF ? " OK" : " FAIL"); *p = 0;
		out(8, s_Buf, s == EXPF ? CT_INK : CT_INV);
	}
	// 3. bank switch latency: read the marker immediately after the switch
	{
		const volatile u8* m4 = (const volatile u8*)g_SegMagic4;
		const volatile u8* m5 = (const volatile u8*)g_SegMagic5;
		u16 bad = 0;
		for (u16 i = 0; i < 400; ++i)
		{
			SET_BANK_SEGMENT(SEG_WINDOW_BANK, 4);
			if (m4[1] != 0x34) ++bad;
			SET_BANK_SEGMENT(SEG_WINDOW_BANK, 5);
			if (m5[1] != 0x35) ++bad;
		}
		p = s_Buf; p = str(p, "3 switch+read x800 bad "); p = dec(p, bad); p = str(p, bad ? " FAIL" : " OK"); *p = 0;
		out(9, s_Buf, bad ? CT_INV : CT_INK);
	}
	// 4. RAM
	n = ram_test();
	p = s_Buf; p = str(p, "4 RAM E000-EFFF bad "); p = dec(p, n); p = str(p, n ? " FAIL" : " OK"); *p = 0;
	out(10, s_Buf, n ? CT_INV : CT_INK);
	// 5. VRAM read-back, display off / on
	n = vram_test(FALSE);
	p = s_Buf; p = str(p, "5 VRAM r/w display OFF bad "); p = dec(p, n); p = str(p, n ? " FAIL" : " OK"); *p = 0;
	out(11, s_Buf, n ? CT_INV : CT_INK);
	n = vram_test(TRUE);
	p = s_Buf; p = str(p, "  VRAM r/w display ON  bad "); p = dec(p, n); p = str(p, n ? " FAIL" : " OK"); *p = 0;
	out(12, s_Buf, n ? CT_INV : CT_INK);
	// 6. interrupts + hook (input/audio ISR) and window checksum while it runs
	Isr_Init();
	{
		u8 f0 = g_Frames;
		for (u8 i = 0; i < 50; ++i) Halt();
		p = s_Buf; p = str(p, "6 ISR frames in 50 halts: "); p = dec(p, (u8)(g_Frames - f0));
		p = str(p, Sys_Is60Hz() ? "  60Hz" : "  50Hz"); *p = 0;
		out(14, s_Buf, CT_INK);
	}
	out(15, "7 window with ISR running", CT_INK);
	ok &= seg_line(16, 5, EXP5, "");
	ok &= seg_line(17, 4, EXP4, "");
	// 8. images: draw the logo like the game does (look for noise)
	Gfx_TextCenter(0, 19, 32, ok ? "ALL ROM CHECKS OK" : "ROM CHECK FAILED", ok ? CT_INK : CT_INV);
	out(20, "Photo this screen. Logo below:", CT_INK);
	{
		extern const unsigned char g_Gfx_logo_small[];
		SET_BANK_SEGMENT(SEG_WINDOW_BANK, SEG_GFX);
		Gfx_Blit(0, 21, g_Gfx_logo_small, 16, 3);
	}
	for (;;)
		Halt();
}
#endif
