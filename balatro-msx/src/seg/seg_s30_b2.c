// mapper code segment 30 (bank 2): title, blind select, info, game over + joker hooks
#include "ui/ui_menu.c"
#include "game/jokers2.c"      // joker hooks outside scoring (Blind select, discards, sales, packs)
#ifdef SELFTEST
#define JK_PART 1
#include "../../tests/test_cases2.c"      /* Z80 self-test: first half of the joker cases */
#endif
