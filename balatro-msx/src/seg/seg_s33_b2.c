// mapper code segment 33 (bank 2): more Z80 self-test cases (512 KB -selftest ROM only)
#ifdef SELFTEST
#define JK_PART 1
#include "../../tests/test_cases2.c"      /* first half of the joker cases */
#undef JK_PART
#define JK_PART 3
#include "../../tests/test_cases2.c"      /* deck / stake cases */
#undef JK_PART
#define JK_PART 4
#include "../../tests/test_cases2.c"      /* Negative edition, new decks, tags */
#endif
typedef char seg33_unused_t;
