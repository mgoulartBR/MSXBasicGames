// Game modes: Pocket (1P / 2P), Nine Ball (2P), Trick shots.
#pragma once
#include "msxgl.h"

typedef struct { u8 up, down, left, right, fire, back; } Keys;
extern Keys g_Held, g_Press;                     // keys down now / newly pressed this frame
void Input_Poll(void);
void Frame(void);                                // Halt, run the queued VDP commands, read the keys
void WaitFrames(u8 n);

void Game_Pocket(u8 players);                    // 1 = Pocket Game, 2 = 2P match (best of 5 sets)
void Game_Nine(u8 raceTo);                       // 2P nine ball, first to raceTo racks
u8   Game_Trick(u8 index);                       // returns 1 if solved
extern u8 g_TrickDone[3];                        // bit per solved trick

// implemented by the UI (sidepocket.c)
void Screen_StageIntro(u8 stage, u16 target);
