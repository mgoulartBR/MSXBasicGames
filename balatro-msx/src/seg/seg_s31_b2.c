// mapper code segment 31 (bank 2): Z80 self-test (only in the -selftest ROM): the same cases as the host tests, compiled by SDCC
#ifdef SELFTEST
#include "../../tests/test_cases.c"
volatile u8 g_selftest[24];
volatile u16 g_selgot[160];    // got value of every case. g_selftest: [0]=done [1]=total [2]=failures [3..]=bitmask of failing case numbers (case n -> bit n)
void selftest_clear(void) BANKED { for (u8 i = 0; i < 24; i++) g_selftest[i] = 0; }
void test_report(const char* name, u32 got, u32 want)
{
	(void)name;
	u8 n = g_selftest[1]++;
	g_selgot[n] = (u16)got;
	if (got != want) { g_selftest[2]++; g_selftest[3 + (n >> 3)] |= (u8)(1 << (n & 7)); }
}
#endif

typedef char seg31_unused_t;   /* keeps the translation unit non-empty in release builds */
