// Side Pocket-style pool logic for MSX2 (MSXGL). Pure C, no platform deps,
// so it builds on the host for testing and with SDCC for the MSX.
#ifndef SP_LOGIC_H
#define SP_LOGIC_H

#include <stdint.h>

// Fixed point: positions are 12.4 (1/16 px), velocities are 8.8 px/frame.
#define SP_POS_SHIFT   4
#define SP_NUM_BALLS   10          // cue + 9 object balls
#define SP_RADIUS      (4 << SP_POS_SHIFT)   // ball radius, 4 px

// Playfield (inner cushion edges), in 1/16 px.
#define SP_LEFT        (16  << SP_POS_SHIFT)
#define SP_RIGHT       (240 << SP_POS_SHIFT)
#define SP_TOP         (24  << SP_POS_SHIFT)
#define SP_BOTTOM      (136 << SP_POS_SHIFT)
#define SP_POCKET_R    (7 << SP_POS_SHIFT)

#define SP_MAX_SPEED   (12 << 8)   // 12 px/frame
#define SP_STOP_SPEED  6           // below this (8.8) a ball stops

typedef struct {
    int16_t x, y;      // 12.4
    int16_t vx, vy;    // 8.8
    uint8_t active;    // on table
} SP_Ball;

typedef struct {
    SP_Ball ball[SP_NUM_BALLS];   // ball[0] = cue ball
    uint8_t pocketed_mask_lo;     // bit n = ball n pocketed this shot (balls 0-7)
    uint8_t pocketed_mask_hi;     // balls 8-9 in bits 0-1
    uint8_t scratch;              // cue ball pocketed this shot
} SP_Table;

void    SP_Init(SP_Table *t);
void    SP_Shoot(SP_Table *t, uint8_t angle, uint8_t power); // angle 0-63 (64 = full turn), power 0-255
uint8_t SP_Step(SP_Table *t);       // advance 1 frame; returns 1 while any ball moves
uint8_t SP_IsMoving(const SP_Table *t);
void    SP_PocketPosition(uint8_t i, int16_t *x, int16_t *y); // i in 0..5

#endif
