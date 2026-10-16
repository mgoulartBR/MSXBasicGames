#include "blob_io.h"
#include "text.h"
#include "gen/blob.h"
#include "../src/sp_logic.h"

#define WINDOW  0xA000
#define RING_N  4096
#define RING_F  18

static u8 s_Seg;
static const u8* s_Ptr;
static u8 s_Row[128];

void Blob_Open(u8 seg, u16 off)
{
	s_Seg = BLOB_FIRST_SEG + seg;
	SET_BANK_SEGMENT(3, s_Seg);
	s_Ptr = (const u8*)(WINDOW + off);
}

u8 Blob_Byte(void)
{
	u8 b = *s_Ptr++;
	if (s_Ptr == (const u8*)0xC000)
	{
		SET_BANK_SEGMENT(3, ++s_Seg);
		s_Ptr = (const u8*)WINDOW;
	}
	return b;
}

static void SetPaletteFromBlob(void)
{
	u8 i, lo, hi, dark = 0, best = 255, v;
	for (i = 0; i < 16; i++)
	{
		lo = Blob_Byte(); hi = Blob_Byte();
		VDP_SetPaletteEntry(i, (u16)lo | ((u16)hi << 8));
		v = (lo >> 4) + (lo & 7) + hi;                 // R + B + G
		if (v < best) { best = v; dark = i; }
	}
	VDP_SetColor(dark);                                // border = the darkest colour of the picture
}

void Blob_SetPalette(u8 seg, u16 off)
{
	Blob_Open(seg, off);
	SetPaletteFromBlob();
}

// ---------------------------------------------------------------------------------------------------------
// LZSS decoder core (Z80).  Ring buffer = 4 KB at 0xD000-0xDFFF (nothing else may live there).
// State (kept in memory between rows): lz_ptr = source pointer in the 0xA000 window, lz_w = ring write
// pointer, lz_m = ring read pointer of a match in progress, lz_ml = bytes left in that match, lz_fl = flag
// byte with a sentinel bit.  Registers: HL src, DE dst, B dst count, C flags | HL-alt ring write, DE-alt match read,
// B-alt match left, C-alt scratch.  ~150 cycles per output byte against ~1500 for the C version.
// ---------------------------------------------------------------------------------------------------------
#define RING_BASE 0xD000
u16 lz_ptr, lz_w, lz_m;
u8  lz_ml, lz_fl;

void Blob_NextSeg(void)
{
	SET_BANK_SEGMENT(3, ++s_Seg);
}

static void Lz_Rows(u8 count) __z88dk_fastcall __naked
{
	count; // L
	__asm
		ld	b, l
		ld	de, #_s_Row
		ld	hl, (_lz_ptr)
		ld	a, (_lz_fl)
		ld	c, a
		exx
		ld	hl, (_lz_w)
		ld	de, (_lz_m)
		ld	a, (_lz_ml)
		ld	b, a
		exx
	LZ100$:
		exx
		ld	a, b			; bytes left in a match?
		or	a, a
		jp	z, LZ110$
		dec	b
		ld	a, (de)			; copy one byte ring[M] -> ring[W] and out
		ld	c, a
		inc	de
		ld	a, d
		cp	a, #0xE0
		jr	nz, LZ101$
		ld	d, #0xD0
	LZ101$:
		ld	(hl), c
		inc	hl
		ld	a, h
		cp	a, #0xE0
		jr	nz, LZ102$
		ld	h, #0xD0
	LZ102$:
		ld	a, c
		exx
		ld	(de), a
		inc	de
		dec	b
		jp	nz, LZ100$
		jp	LZ199$
	LZ110$:
		exx				; primary set: next flag bit
		srl	c
		jr	nz, LZ111$
		ld	a, (hl)			; refill the flag byte
		inc	hl
		bit	6, h
		call	nz, LZ900$
		ld	c, a
		scf
		rr	c
	LZ111$:
		jp	nc, LZ120$
		ld	a, (hl)			; literal
		inc	hl
		bit	6, h
		call	nz, LZ900$
		exx
		ld	c, a
		ld	(hl), a
		inc	hl
		ld	a, h
		cp	a, #0xE0
		jr	nz, LZ112$
		ld	h, #0xD0
	LZ112$:
		ld	a, c
		exx
		ld	(de), a
		inc	de
		dec	b
		jp	nz, LZ100$
		jp	LZ199$
	LZ120$:					; match: two bytes -> ring read pointer and length
		ld	a, (hl)
		inc	hl
		bit	6, h
		call	nz, LZ900$
		exx
		ld	e, a			; E-alt = low 8 bits of the ring position
		exx
		ld	a, (hl)
		inc	hl
		bit	6, h
		call	nz, LZ900$
		exx
		ld	c, a
		rrca
		rrca
		rrca
		rrca
		and	a, #0x0F
		or	a, #0xD0
		ld	d, a			; D-alt = 0xD0 | high 4 bits of the position
		ld	a, c
		and	a, #0x0F
		add	a, #3
		ld	b, a			; B-alt = length
		exx
		jp	LZ100$
	LZ199$:
		ld	(_lz_ptr), hl
		ld	a, c
		ld	(_lz_fl), a
		exx
		ld	(_lz_w), hl
		ld	(_lz_m), de
		ld	a, b
		ld	(_lz_ml), a
		exx
		ret
	LZ900$:					; crossed the end of the 8 KB window: next segment (keeps every register)
		push	af
		push	bc
		push	de
		exx
		push	bc
		push	de
		push	hl
		exx
		call	_Blob_NextSeg
		exx
		pop	hl
		pop	de
		pop	bc
		exx
		pop	de
		pop	bc
		pop	af
		ld	hl, #0xA000
		ret
	__endasm;
}

void Blob_LoadImage(u8 seg, u16 off, u16 yoff, u8 mirror)
{
	u8 w, h, flags, xb, y0, row;
	u32 addr;

	Blob_Open(seg, off);
	w = Blob_Byte(); h = Blob_Byte(); flags = Blob_Byte(); xb = Blob_Byte(); y0 = Blob_Byte();
	if (flags & 1) SetPaletteFromBlob();
	addr = (u32)(y0 + yoff) * 128 + xb;

	if (flags & 2)                                   // raw rows: straight from the ROM window when possible
	{
		for (row = 0; row < h; row++)
		{
			if ((u16)s_Ptr + w <= 0xC000)
			{
				Vram_Write(s_Ptr, addr, w);
				s_Ptr += w;
				if (s_Ptr == (const u8*)0xC000) { SET_BANK_SEGMENT(3, ++s_Seg); s_Ptr = (const u8*)WINDOW; }
			}
			else
			{
				u8 i;
				for (i = 0; i < w; i++) s_Row[i] = Blob_Byte();
				Vram_Write(s_Row, addr, w);
			}
			addr += 128;
		}
		return;
	}

	lz_ptr = (u16)s_Ptr;
	lz_w = RING_BASE + RING_N - RING_F;
	lz_m = RING_BASE; lz_ml = 0; lz_fl = 1;
	for (row = 0; row < h; row++)
	{
		Lz_Rows(w);
		Vram_Write(s_Row, addr, w);
		if (mirror) Vram_Write(s_Row, addr + 0x8000, w);
		addr += 128;
	}
}
