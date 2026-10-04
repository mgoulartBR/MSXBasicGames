#pragma once
#include "msxgl.h"
#include "../platform/msx_gfx.h"
#include "../platform/msx_input.h"
#include "../platform/msx_audio.h"

#define CREDIT_PORT  "MSX port by BigFive Studios"
#define CREDIT_ORIG  "by Lucas Pope @dukope"
#define GAME_VERSION "0.1"

// One frame: wait for VBlank, read input, run audio sequencer.
void Ui_Frame(void);
void Ui_WaitRelease(void);
// Paged, word-wrapped message. Returns when the last page is acknowledged with A.
void Ui_ShowPages(const char* msg, u8 col, u8 row0, u8 rows, u8 wtiles, u8 color,
                  const char* finalLabel, u8 promptRow, u8 promptColor);
void Ui_Credits(u8 row, u8 color);

// Screens: each runs its own loop and returns the next state
enum { ST_TITLE, ST_MORNING, ST_PLAY, ST_NIGHT };
u8 Scr_Title(void);
u8 Scr_Morning(void);
u8 Scr_Play(void);
u8 Scr_Night(void);
