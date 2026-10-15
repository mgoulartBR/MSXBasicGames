#include "sp_logic.h"

// Quarter-wave sine, 255*sin(pi/2*i/64), i = 0..64. The Mega Drive game uses the same
// formula (8-bit table, 256 angles per turn, sign handled by quadrant), see docs/spec.md.
static const uint8_t sin_q[65] = {
    0, 6, 13, 19, 25, 31, 37, 44, 50, 56, 62, 68, 74, 80, 86, 92,
    98, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176,
    180, 185, 189, 193, 197, 201, 205, 208, 212, 215, 219, 222, 225, 228, 231, 233,
    236, 238, 240, 242, 244, 246, 247, 249, 250, 251, 252, 253, 254, 254, 255, 255,
    255
};

// Angle: 0..255 per full turn. Returns -255..255.
static int16_t sp_sin(uint8_t a)
{
    uint8_t i = a & 0x3F;
    int16_t v = (a & 0x40) ? sin_q[64 - i] : sin_q[i];
    return (a & 0x80) ? -v : v;
}
static int16_t sp_cos(uint8_t a) { return sp_sin((uint8_t)(a + 64)); }

static int16_t sp_abs(int16_t v) { return v < 0 ? -v : v; }

static uint16_t isqrt32(uint32_t n)
{
    uint32_t r = 0, b = 1UL << 30;
    while (b > n) b >>= 2;
    while (b) {
        if (n >= r + b) { n -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return (uint16_t)r;
}

void SP_PocketPosition(uint8_t i, int16_t *x, int16_t *y)
{
    int16_t mid = (SP_LEFT + SP_RIGHT) / 2;
    *x = (i % 3 == 0) ? SP_LEFT : (i % 3 == 1) ? mid : SP_RIGHT;
    *y = (i < 3) ? SP_TOP : SP_BOTTOM;
}

void SP_Init(SP_Table *t)
{
    uint8_t i;
    int16_t cx = 80 << SP_POS_SHIFT, cy = 80 << SP_POS_SHIFT;
    int16_t dx = (SP_RADIUS * 2) + 2;   // rack spacing
    uint8_t n = 1, col, row;

    for (i = 0; i < SP_NUM_BALLS; i++) {
        t->ball[i].vx = t->ball[i].vy = 0;
        t->ball[i].active = 1;
    }
    t->ball[0].x = 190 << SP_POS_SHIFT;
    t->ball[0].y = cy;

    // Diamond rack: 1-2-3-2-1 = 9 balls, apex towards the cue ball.
    for (col = 0; col < 5 && n < SP_NUM_BALLS; col++) {
        uint8_t rows = (col < 3) ? col + 1 : 5 - col;
        for (row = 0; row < rows && n < SP_NUM_BALLS; row++) {
            t->ball[n].x = (int16_t)(cx - (dx * col * 7) / 8);
            t->ball[n].y = (int16_t)(cy + (row * 2 - (rows - 1)) * (dx / 2));
            n++;
        }
    }
    t->pocketed_mask_lo = t->pocketed_mask_hi = t->scratch = 0;
}

void SP_Shoot(SP_Table *t, uint8_t angle, uint8_t power)
{
    int32_t speed = ((int32_t)SP_MAX_SPEED * power) >> 8;
    t->ball[0].vx = (int16_t)((speed * sp_cos(angle)) >> 8);
    t->ball[0].vy = (int16_t)((speed * sp_sin(angle)) >> 8);
    t->pocketed_mask_lo = t->pocketed_mask_hi = t->scratch = 0;
}

uint8_t SP_IsMoving(const SP_Table *t)
{
    uint8_t i;
    for (i = 0; i < SP_NUM_BALLS; i++)
        if (t->ball[i].active && (t->ball[i].vx || t->ball[i].vy)) return 1;
    return 0;
}

static void collide(SP_Ball *a, SP_Ball *b)
{
    int16_t dx = b->x - a->x, dy = b->y - a->y;
    int32_t d2 = (int32_t)dx * dx + (int32_t)dy * dy;
    int32_t min = SP_RADIUS * 2;
    int16_t d, nx, ny, dvx, dvy, dot, push;

    if (d2 >= min * min || d2 == 0) return;
    d = (int16_t)isqrt32((uint32_t)d2);
    nx = (int16_t)(((int32_t)dx << 8) / d);   // unit normal, 8.8
    ny = (int16_t)(((int32_t)dy << 8) / d);

    // Separate overlapping balls
    push = (min - d + 1) / 2;
    a->x -= (int16_t)(((int32_t)nx * push) >> 8);
    a->y -= (int16_t)(((int32_t)ny * push) >> 8);
    b->x += (int16_t)(((int32_t)nx * push) >> 8);
    b->y += (int16_t)(((int32_t)ny * push) >> 8);

    // Equal-mass elastic collision: exchange velocity along the normal
    dvx = a->vx - b->vx; dvy = a->vy - b->vy;
    dot = (int16_t)(((int32_t)dvx * nx + (int32_t)dvy * ny) >> 8);
    if (dot <= 0) return;                      // already separating
    dot = (dot * 245) >> 8;                    // slight energy loss
    a->vx -= (int16_t)(((int32_t)dot * nx) >> 8);
    a->vy -= (int16_t)(((int32_t)dot * ny) >> 8);
    b->vx += (int16_t)(((int32_t)dot * nx) >> 8);
    b->vy += (int16_t)(((int32_t)dot * ny) >> 8);
}

static void pocket(SP_Table *t, uint8_t i)
{
    t->ball[i].active = 0;
    t->ball[i].vx = t->ball[i].vy = 0;
    if (i == 0) t->scratch = 1;
    if (i < 8) t->pocketed_mask_lo |= (uint8_t)(1 << i);
    else       t->pocketed_mask_hi |= (uint8_t)(1 << (i - 8));
}

uint8_t SP_Step(SP_Table *t)
{
    uint8_t i, j;

    for (i = 0; i < SP_NUM_BALLS; i++) {
        SP_Ball *b = &t->ball[i];
        if (!b->active) continue;

        b->x += b->vx >> 4;
        b->y += b->vy >> 4;

        // Pockets (checked before cushions so corner balls drop in)
        for (j = 0; j < 6; j++) {
            int16_t px, py, dx, dy;
            SP_PocketPosition(j, &px, &py);
            dx = b->x - px; dy = b->y - py;
            if ((int32_t)dx * dx + (int32_t)dy * dy < (int32_t)SP_POCKET_R * SP_POCKET_R) {
                pocket(t, i);
                break;
            }
        }
        if (!b->active) continue;

        // Cushions: reflect with 25% loss
        if (b->x < SP_LEFT + SP_RADIUS)   { b->x = SP_LEFT + SP_RADIUS;   b->vx = -((b->vx * 192) >> 8); }
        if (b->x > SP_RIGHT - SP_RADIUS)  { b->x = SP_RIGHT - SP_RADIUS;  b->vx = -((b->vx * 192) >> 8); }
        if (b->y < SP_TOP + SP_RADIUS)    { b->y = SP_TOP + SP_RADIUS;    b->vy = -((b->vy * 192) >> 8); }
        if (b->y > SP_BOTTOM - SP_RADIUS) { b->y = SP_BOTTOM - SP_RADIUS; b->vy = -((b->vy * 192) >> 8); }

        // Rolling friction
        // (v>>6 alone never decays small positive values, so add a 1-unit floor)
        b->vx -= (b->vx >> 6) + (b->vx > 0 ? 1 : 0);
        b->vy -= (b->vy >> 6) + (b->vy > 0 ? 1 : 0);
        if (sp_abs(b->vx) < SP_STOP_SPEED && sp_abs(b->vy) < SP_STOP_SPEED)
            b->vx = b->vy = 0;
    }

    for (i = 0; i < SP_NUM_BALLS; i++) {
        if (!t->ball[i].active) continue;
        for (j = i + 1; j < SP_NUM_BALLS; j++)
            if (t->ball[j].active) collide(&t->ball[i], &t->ball[j]);
    }
    return SP_IsMoving(t);
}
