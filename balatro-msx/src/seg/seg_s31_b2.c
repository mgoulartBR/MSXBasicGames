// mapper code segment 31 (bank 2): Z80 self-test (only in the -selftest ROM): the same cases as the host tests, compiled by SDCC
#ifdef SELFTEST
#include "../../tests/test_cases.c"
extern volatile u8 g_selftest[24];
#define JK_PART 2
#include "../../tests/test_cases2.c"   /* second half of the joker cases */
void selftest_clear(void) BANKED { for (u8 i = 0; i < 24; i++) g_selftest[i] = 0; }
#endif

typedef char seg31_unused_t;   /* keeps the translation unit non-empty in release builds */
