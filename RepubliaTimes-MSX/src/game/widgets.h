// HUD widgets: analog clock and loyalty gauge (procedurally drawn, as in the original)
#pragma once
#include "msxgl.h"

void Clock_Init(void);
// time in 1/600 s units; draws hands into the 5x4 tile clock at (col,row)
void Clock_Draw(u8 col, u8 row, u16 time, bool alarmBlink);
// loyalty gauge 5x5 tiles at (col,row)
void Meter_Draw(u8 col, u8 row, i8 value);
// "Readers" label + count (2 text rows) in a 5-tile column; onWhite selects colours
void Readers_Draw(u8 col, u8 row, i16 count, i16 delta, bool showDelta, bool onWhite);
