#include "msxgl.h"
#include "psg.h"
#include "audio.h"

extern const u16 g_NotePeriod[];
extern const u8 g_MusicLen, g_MusicLead[], g_MusicBass[];

typedef struct { u16 p0, p1; u8 len, v0, v1, noise; } Seg;      // linear sweep of period / volume; noise = noise period + 1 (0 = tone)
static const Seg k_select[]  = { { 214, 160, 3, 12, 6, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_discard[] = { { 0, 0, 10, 12, 0, 6 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_use[]     = { { 285, 285, 3, 12, 10, 0 }, { 214, 214, 3, 12, 10, 0 }, { 170, 170, 3, 12, 10, 0 }, { 142, 142, 6, 12, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_sell[]    = { { 142, 142, 3, 12, 10, 0 }, { 214, 214, 8, 12, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_coin[]    = { { 106, 106, 2, 13, 11, 0 }, { 80, 80, 8, 13, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_cash[]    = { { 170, 170, 3, 12, 10, 0 }, { 142, 142, 3, 12, 10, 0 }, { 113, 113, 3, 12, 10, 0 }, { 85, 85, 10, 13, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_buy[]     = { { 128, 128, 3, 13, 10, 0 }, { 96, 96, 9, 13, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_reroll[]  = { { 0, 0, 3, 11, 8, 4 }, { 300, 150, 8, 11, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_pick[]    = { { 170, 170, 3, 13, 10, 0 }, { 113, 113, 3, 13, 10, 0 }, { 85, 85, 8, 13, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_chips[]   = { { 0, 0, 5, 13, 5, 0 }, { 0, 0, 0, 0, 0, 0 } };     // period patched by pitch step
static const Seg k_mult[]    = { { 0, 0, 6, 13, 4, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_xmult[]   = { { 0, 0, 3, 14, 12, 3 }, { 0, 0, 9, 14, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_win[]     = { { 170, 170, 4, 13, 12, 0 }, { 142, 142, 4, 13, 12, 0 }, { 113, 113, 4, 13, 12, 0 }, { 85, 85, 4, 13, 12, 0 }, { 113, 113, 3, 13, 12, 0 }, { 85, 85, 18, 14, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_lose[]    = { { 200, 200, 8, 13, 10, 0 }, { 250, 250, 8, 13, 10, 0 }, { 320, 320, 8, 13, 10, 0 }, { 400, 700, 30, 13, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg k_error[]   = { { 600, 600, 5, 12, 12, 0 }, { 0, 0, 2, 0, 0, 0 }, { 600, 600, 6, 12, 0, 0 }, { 0, 0, 0, 0, 0, 0 } };
static const Seg* const k_sfx[SFX_COUNT] = { 0, k_select, k_discard, k_use, k_sell, k_coin, k_cash, k_buy, k_reroll, k_pick, k_chips, k_mult, k_xmult, k_win, k_lose, k_error };

static const Seg* s_seg; static u8 s_frame; static u16 s_patch; static const Seg* s_cur;
static bool s_music = TRUE; static u8 s_step, s_tick, s_stepFrames;

static void reg(u8 r, u8 v) { PSG_SetRegister(r, v); }

void Snd_Init(u8 hz)
{
	s_stepFrames = (hz == 50) ? 6 : 7;
	for (u8 r = 0; r < 14; r++) reg(r, 0);
	reg(7, 0xB8);                     // tones+noise off, I/O port directions as the MSX BIOS sets them
}
void Snd_Music(bool on) { s_music = on; if (!on) { reg(8, 0); reg(9, 0); } }
bool Snd_MusicOn(void) { return s_music; }

void Snd_PlayPitch(u8 sfx, u8 step)
{
	if (sfx == SFX_NONE || sfx >= SFX_COUNT) return;
	s_seg = s_cur = k_sfx[sfx]; s_frame = 0;
	if (sfx == SFX_CHIPS || sfx == SFX_MULT || sfx == SFX_XMULT)
	{
		u8 n = step > 24 ? 24 : step;
		u8 base = (sfx == SFX_CHIPS) ? 49 : (sfx == SFX_MULT ? 37 : 31);     // C5 / C4 / F#3
		s_patch = g_NotePeriod[base + n];
	}
	else s_patch = 0;
}
void Snd_Play(u8 sfx) { Snd_PlayPitch(sfx, 0); }

static i16 lerp(i16 a, i16 b, u8 i, u8 n) { return n ? (i16)(a + ((i32)(b - a) * i) / n) : a; }

void Snd_Update(void)
{
	// ---- sound effect (channel C + noise) ----
	u8 mix = 0x3F ^ 0x00;                                  // all tone/noise disabled by default (bit set = off)
	u8 tone = 0, nz = 0;
	if (s_seg && s_seg->len)
	{
		u16 p0 = s_seg->p0, p1 = s_seg->p1;
		if (s_patch) { p0 = p1 = s_patch; }
		u8 vol = (u8)lerp(s_seg->v0, s_seg->v1, s_frame, s_seg->len);
		if (s_seg->noise) { nz = 1; reg(6, (u8)((s_seg->noise - 1) * 4 + 2)); }
		if (p0 || p1) { u16 p = (u16)lerp((i16)p0, (i16)p1, s_frame, s_seg->len); reg(4, (u8)(p & 0xFF)); reg(5, (u8)(p >> 8)); tone = 1; }
		reg(10, vol);
		if (++s_frame >= s_seg->len) { s_frame = 0; s_seg++; if (!s_seg->len) s_seg = 0; }
	}
	else reg(10, 0);
	// ---- music (channels A and B) ----
	bool ma = FALSE;
	if (s_music)
	{
		if (s_tick == 0)
		{
			u8 l = g_MusicLead[s_step], b = g_MusicBass[s_step];
			if (l != 255) { u16 p = g_NotePeriod[l]; if (p) { reg(0, (u8)(p & 0xFF)); reg(1, (u8)(p >> 8)); } }
			if (b != 255) { u16 p = g_NotePeriod[b]; if (p) { reg(2, (u8)(p & 0xFF)); reg(3, (u8)(p >> 8)); } }
		}
		u8 l = g_MusicLead[s_step], b = g_MusicBass[s_step];
		// short decay on every new note, silence on rests
		u8 hold = (l == 255);
		u8 va = (l == 0) ? 0 : (u8)(hold ? 4 : (s_tick < 4 ? 8 - s_tick : 4));
		u8 vb = (b == 0) ? 0 : (u8)(b == 255 ? 5 : (s_tick < 5 ? 9 - s_tick : 5));
		reg(8, va); reg(9, vb);
		ma = TRUE;
		if (++s_tick >= s_stepFrames) { s_tick = 0; if (++s_step >= g_MusicLen) s_step = 0; }
	}
	mix = 0x38;                                            // noise off on all channels, tones on A,B (music)
	if (!ma) mix |= 0x03;                                  // tone A/B off when music is off
	if (!tone) mix |= 0x04;                                // tone C off when no sfx tone
	if (nz) mix &= (u8)~0x20;                              // noise on channel C
	reg(7, (u8)(0x80 | mix));
}
