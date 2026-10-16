// Graphics for the table screen (Screen 5): VDP command queue, balls, aim guide, HUD.
#pragma once
#include "msxgl.h"
#include "../src/sp_logic.h"

#define TBL_Y     46                    // line where the table image starts (the HUD is above it)
#define PAGE1     256                   // first line of page 1 (clean copy of the screen + ball cells)

void Gfx_Init(void);                    // Screen 5, 212 lines, no sprites; the display stays off
void Gfx_Show(bool on);
void Gfx_Flush(void);                   // run the queued VDP commands (call right after Halt)
void Gfx_Wait(void);

void Gfx_LoadGame(void);                // HUD + table into page 0 and (clean copy) page 1, ball cells, palette
void Gfx_RestoreTable(void);            // paint the clean table over page 0 and forget the drawn balls
void Gfx_Balls(SP_Table* t);            // plan the redraw of the balls that moved
void Gfx_Aim(SP_Table* t, u8 angle, bool show);

void Hud_Score(u16 score);
void Hud_Shots(u8 n);                   // lit lamps (0..8)
void Hud_Next(u8 ball);                 // ball number shown in the NEXT box, 0 = "-"
void Hud_Power(u8 power);               // 0 clears
void Hud_Label(const c8* line1, const c8* line2);   // two short texts in the CENTER box

// immediate helpers (page 0 / any page)
void Gfx_Copy(u16 sx, u16 sy, u16 dx, u16 dy, u16 w, u16 h);
void Gfx_Fill(u16 x, u16 y, u16 w, u16 h, u8 col);
void Gfx_Outline(u16 x, u16 y, u16 w, u16 h, u8 col);
