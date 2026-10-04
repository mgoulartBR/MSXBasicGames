// PSG audio: SFX on channel C (with noise), music on channels A and B.
// SFX and music are sequenced from the main loop (Audio_Update once per frame): no ISR needed.
#include "msx_audio.h"
#include "psg.h"
#include "../data/music_data.h"

// PSG period for MIDI notes 24..95 (clock 1.789773 MHz)
static const u16 s_Period[72] = {
	3420, 3229, 3047, 2876, 2715, 2562, 2419, 2283, 2155, 2034, 1920, 1812,
	1710, 1614, 1524, 1438, 1357, 1281, 1209, 1141, 1077, 1017, 960, 906,
	855, 807, 762, 719, 679, 641, 605, 571, 539, 508, 480, 453,
	428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
	214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
	107, 101, 95, 90, 85, 80, 76, 71, 67, 64, 60, 57,
};

typedef struct
{
	u8 frames;   // duration in ticks of 1/50 s
	u16 tone;    // tone period (0 = silent)
	u8 vol;      // 0..15
	u8 noise;    // noise period (0 = none)
} SfxStep;

static const SfxStep s_SfxClick[]   = { {1, 200, 12, 0}, {1, 160, 10, 0}, {0, 0, 0, 0} };
static const SfxStep s_SfxDrag[]    = { {1, 300, 11, 0}, {1, 240, 11, 0}, {1, 190, 10, 0}, {0, 0, 0, 0} };
static const SfxStep s_SfxDrop[]    = { {1, 500, 14, 12}, {2, 420, 12, 16}, {2, 600, 8, 22}, {0, 0, 0, 0} };
static const SfxStep s_SfxFeed[]    = { {3, 142, 12, 0}, {1, 0, 0, 0}, {4, 106, 12, 0}, {0, 0, 0, 0} };
static const SfxStep s_SfxAlarm[]   = { {5, 213, 13, 0}, {3, 0, 0, 0}, {5, 213, 13, 0}, {3, 0, 0, 0}, {5, 213, 13, 0}, {0, 0, 0, 0} };
static const SfxStep s_SfxDayOver[] = { {8, 142, 13, 0}, {8, 169, 13, 0}, {8, 190, 13, 0}, {16, 284, 13, 0}, {0, 0, 0, 0} };
static const SfxStep s_SfxError[]   = { {4, 900, 12, 0}, {4, 1100, 12, 0}, {0, 0, 0, 0} };

static const SfxStep* const s_Sfx[] = { 0, s_SfxClick, s_SfxDrag, s_SfxDrop, s_SfxFeed, s_SfxAlarm, s_SfxDayOver, s_SfxError };

static const SfxStep* s_SfxPtr;
static u8 s_SfxWait;
static u8 s_Mute;

typedef struct
{
	const u8* start;
	const u8* p;
	u8 wait;
	u8 vol;
	u8 note;
} Voice;

static Voice s_Voice[2];
static u8 s_MusicAcc;
static u8 s_Music;
static bool s_Is60;

static void reg(u8 r, u8 v) { PSG_SetRegister(r, v); }

// R7 mixer (bit = 1 disables): tone A,B on (music), noise A,B off; channel C depends on the SFX step
#define MIX_IDLE   0b00111100
#define MIX_BASE   0b00011000
static void mixer_c(bool tone, bool noise)
{
	reg(7, MIX_BASE | (tone ? 0 : 0b000100) | (noise ? 0 : 0b100000));
}

void Audio_Init(void)
{
	s_Is60 = Sys_Is60Hz();
	s_Mute = 0;
	s_SfxPtr = 0;
	s_Music = MUSIC_NONE;
	for (u8 r = 0; r < 14; ++r)
		reg(r, 0);
	reg(7, MIX_IDLE);
}

// Main-thread API: only sets request bytes; the VBlank hook (the only PSG user) applies them.
static volatile u8 s_ReqSfx;
static volatile u8 s_ReqMusic = 0xFF; // 0xFF = none

void Audio_Mute(bool mute)
{
	s_Mute = mute;
}

bool Audio_IsMuted(void)
{
	return s_Mute;
}

void Sfx_Play(u8 id)
{
	if (id != SFX_NONE && id <= SFX_ERROR)
		s_ReqSfx = id;
}

void Music_Play(u8 id)
{
	s_ReqMusic = id;
}

static void music_start(u8 id)
{
	s_Music = id;
	s_MusicAcc = 0;
	reg(8, 0);
	reg(9, 0);
	if (id == MUSIC_NONE)
		return;
	const MusicTrack* t = &g_MusicTracks[id - 1];
	for (u8 v = 0; v < 2; ++v)
	{
		s_Voice[v].start = s_Voice[v].p = t->voice[v];
		s_Voice[v].wait = 1;
		s_Voice[v].vol = 0;
	}
}

static void voice_step(u8 v)
{
	Voice* vc = &s_Voice[v];
	if (--vc->wait)
	{
		// simple release on the last tick of a note
		if (vc->wait == 1 && vc->vol > 3)
			reg(8 + v, vc->vol - 3);
		return;
	}
	for (;;)
	{
		u8 n = *vc->p++;
		if (n == 0xFF) // end -> loop
		{
			vc->p = vc->start;
			continue;
		}
		u8 d = *vc->p++;
		vc->wait = d ? d : 1;
		if (n == 0)
		{
			reg(8 + v, 0);
			vc->vol = 0;
		}
		else
		{
			u16 per = s_Period[n < 24 ? 0 : (n > 95 ? 71 : n - 24)];
			reg(v * 2, (u8)per);
			reg(v * 2 + 1, per >> 8);
			vc->vol = v == 0 ? 11 : 8;
			reg(8 + v, s_Mute ? 0 : vc->vol);
		}
		break;
	}
}

static u8 s_MuteApplied;

void Audio_Update(void)
{
	if (s_Mute != s_MuteApplied)
	{
		s_MuteApplied = s_Mute;
		if (s_Mute)
		{
			reg(8, 0);
			reg(9, 0);
			reg(10, 0);
			reg(7, MIX_IDLE);
		}
	}
	// --- requests from the main thread
	if (s_ReqMusic != 0xFF)
	{
		u8 m = s_ReqMusic;
		s_ReqMusic = 0xFF;
		music_start(m);
	}
	if (s_ReqSfx)
	{
		s_SfxPtr = s_Sfx[s_ReqSfx];
		s_SfxWait = 0;
		s_ReqSfx = 0;
	}
	// --- music: sequencer ticks at 30 Hz regardless of 50/60 Hz refresh
	if (s_Music != MUSIC_NONE && !s_Mute)
	{
		s_MusicAcc += s_Is60 ? 10 : 12;
		if (s_MusicAcc >= 20)
		{
			s_MusicAcc -= 20;
			voice_step(0);
			voice_step(1);
		}
	}

	// --- sfx on channel C
	if (s_SfxPtr)
	{
		if (s_SfxWait == 0)
		{
			const SfxStep* st = s_SfxPtr;
			if (st->frames == 0)
			{
				s_SfxPtr = 0;
				reg(10, 0);
				reg(7, MIX_IDLE);
				return;
			}
			s_SfxWait = st->frames;
			s_SfxPtr++;
			if (s_Mute || st->vol == 0 || (!st->tone && !st->noise))
			{
				reg(10, 0);
				reg(7, MIX_IDLE);
			}
			else
			{
				reg(4, (u8)st->tone);
				reg(5, st->tone >> 8);
				reg(6, st->noise);
				reg(10, st->vol);
				mixer_c(st->tone != 0, st->noise != 0);
			}
		}
		if (s_SfxWait)
			--s_SfxWait;
	}
}
