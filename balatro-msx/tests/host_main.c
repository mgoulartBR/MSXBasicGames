// Host runner for tests/test_cases.c (+ the random-run simulation lives in host_sim.c)
#include <stdio.h>
#include "bgame.h"
extern void run_all_cases(void);
static int fails, total;
void test_report(const char* name, u32 got, u32 want)
{
	total++;
	if (got != want) { fails++; printf("FAIL %-52s got %lu want %lu\n", name, (unsigned long)got, (unsigned long)want); }
	else printf("ok   %-52s %lu\n", name, (unsigned long)got);
}
int main(void)
{
	run_all_cases();
	printf(fails ? "\n%d of %d FAILED\n" : "\nall %d cases passed\n", fails ? fails : total, total);
	return fails != 0;
}
