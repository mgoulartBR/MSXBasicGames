// Side Pocket-style pool logic for MSX2 (MSXGL). Pure C, no platform deps,
// so it builds on the host for testing and with SDCC for the MSX.
#ifndef SP_LOGIC_H
#define SP_LOGIC_H

#if defined(__SDCC)
// The SDCC bundled with MSXGL ships no <stdint.h>.
typedef signed char    int8_t;
typedef unsigned char  uint8_t;
typedef int            int16_t;
typedef unsigned int   uint16_t;
typedef long           int32_t;
typedef unsigned long  uint32_t;
#else
#include <stdint.h>
#endif

// Fixed point 8.8 everywhere: positions (unsigned, the high byte is the pixel) and velocities
// (signed, px/frame).  A position update is a plain 16-bit add.
#define SP_POS_SHIFT   8
#define SP_NUM_BALLS   10                         // cue + 9 object balls
#define SP_RADIUS      (4u << SP_POS_SHIFT)       // ball radius, 4 px

// Playfield in table-image pixels (the 256x166 table crop, see tools/extract_assets.py): the edges of the
// flat cloth, where a ball touches the cushion (measured: cloth x 16..239, y 26..141).
// A ball centre stays SP_RADIUS inside these.  The pockets sit in the cushions, so their capture radius
// is larger than the pocket graphic: a ball held back by the cushion still has to reach the pocket.
#define SP_LEFT        (15u  << SP_POS_SHIFT)
#define SP_RIGHT       (240u << SP_POS_SHIFT)
#define SP_TOP         (26u  << SP_POS_SHIFT)
#define SP_BOTTOM      (142u << SP_POS_SHIFT)
#define SP_POCKET_R    (17u << SP_POS_SHIFT)
#define SP_CUE_X       188
#define SP_CUE_Y       84

#define SP_MAX_SPEED   (12 << 8)   // 12 px/frame
#define SP_STOP_SPEED  6           // below this (8.8) a ball stops

typedef struct {                  // 8 bytes: indexing is a shift, not a multiply
    uint16_t x, y;                // 8.8 (high byte = pixel)
    int16_t  vx, vy;              // 8.8
} SP_Ball;

typedef struct {
    SP_Ball  ball[SP_NUM_BALLS];  // ball[0] = cue ball
    uint8_t  px8[SP_NUM_BALLS];   // pixel coordinates (high bytes of x/y), 0xFF when off the table:
    uint8_t  py8[SP_NUM_BALLS];   // lets the collision scan reject a ball with one 8-bit subtract
    uint16_t active;              // bit n = ball n is on the table
    uint16_t moving;              // bit n = ball n has a non-zero velocity
    uint16_t moved;               // bit n = ball n moved or was pocketed since the renderer last cleared this
    uint8_t  pocketed_mask_lo;    // bit n = ball n pocketed this shot (balls 0-7)
    uint8_t  pocketed_mask_hi;    // balls 8-9 in bits 0-1
    uint8_t  scratch;             // cue ball pocketed this shot
} SP_Table;

#define SP_PX(v)  ((uint8_t)((v) >> 8))   // pixel coordinate of a 8.8 position
#define SP_ACTIVE(t, i)  (((t)->active >> (i)) & 1)

void    SP_Init(SP_Table *t);
void    SP_Shoot(SP_Table *t, uint8_t angle, uint8_t power); // angle 0-255 (256 = full turn), power 0-255
uint8_t SP_Step(SP_Table *t);       // advance 1 frame; returns 1 while any ball moves
uint8_t SP_IsMoving(const SP_Table *t);
void    SP_PocketPosition(uint8_t i, uint16_t *x, uint16_t *y); // i in 0..5 (0-2 top row, 3-5 bottom row)
int16_t SP_Sin(uint8_t angle);   // -255..255, 256 angles per turn
int16_t SP_Cos(uint8_t angle);
// Window scan over x[10] / y[10] (y = x + 10) byte arrays; see sp_logic.c.  Inputs go in the globals.
extern uint8_t sp_scan_xl, sp_scan_wx, sp_scan_yl, sp_scan_wy;
extern const uint8_t *sp_scan_px;
#if defined(__SDCC)
uint8_t SP_ScanWindow(uint8_t start) __z88dk_fastcall;   // first j >= start inside the window, or 0xFF
#else
uint8_t SP_ScanWindow(uint8_t start);
#endif
void    SP_Sync(SP_Table *t);       // rebuild px8/py8 after editing positions or the active mask by hand
void    SP_PlaceCue(SP_Table *t);   // put the cue ball back on the head spot

#endif
