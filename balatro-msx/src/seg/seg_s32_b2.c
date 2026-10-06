// mapper code segment 32 (bank 2): Z80 self-test cases (only in the 512 KB -selftest ROM: the release ROM never links segments above 31)
#ifdef SELFTEST
#include "../../tests/test_cases.c"
extern volatile u8 g_selftest[24];
#define JK_PART 2
#include "../../tests/test_cases2.c"   /* second half of the joker cases */
void selftest_clear(void) BANKED { for (u8 i = 0; i < 24; i++) g_selftest[i] = 0; }
#endif
typedef char seg32_unused_t;
