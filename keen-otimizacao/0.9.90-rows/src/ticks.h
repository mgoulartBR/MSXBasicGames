#ifndef KEEN_TICKS_H
#define KEEN_TICKS_H
#include "msxgl.h"
// Division-free tick phases (the Z80 has no divide: `/20` and `%20` call __divuint, ~1000 T each).
u8 Mod40(u16 t);
extern volatile u16 simulation_ticks;
static u8 Tick20Zero(void){u8 m=Mod40(simulation_ticks);return m==0||m==20;}   // simulation_ticks % 20 == 0
static u8 TickTenthOdd(void){u8 m=Mod40(simulation_ticks);return (m>=10&&m<20)||m>=30;}   // (simulation_ticks / 10) & 1
#endif
