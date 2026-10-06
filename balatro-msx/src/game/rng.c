// Small, cheap RNG for the Z80: 16-bit xorshift (period 65535). Deterministic for a given seed.
#include "bgame.h"

static u16 s_state = 0xACE1;

void rng_seed(u16 s) { s_state = s ? s : 0xACE1; }

u16 rnd16(void)
{
	s_state ^= (u16)(s_state << 7);
	s_state ^= (u16)(s_state >> 9);
	s_state ^= (u16)(s_state << 8);
	return s_state;
}

u8 rnd8(void) { return (u8)(rnd16() >> 8); }

// uniform-ish 0..n-1 (n <= 255): multiply-shift, no division
u8 rndn(u8 n) { return (u8)(((u16)rnd8() * n) >> 8); }

// Oops! All 6s doubles every listed probability (once per copy)
bool rnd_odds(u8 n)
{
	u8 k = joker_count(JK_OOPS), m = 1;
	while (k-- && m < 128) m <<= 1;
	return rndn(n) < m;
}
