// VBlank hook (H.TIMI): input sampling + audio sequencer + frame counter.
// The ISR never touches the VDP; it only reads keyboard/joystick ports and drives the PSG.
#pragma once
#include "msxgl.h"

extern volatile u8 g_Frames; // incremented every VBlank (wraps at 256)

void Isr_Init(void);
