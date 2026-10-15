#include <stdio.h>
#include <stdlib.h>
#include "../src/sp_logic.h"

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); exit(1); } } while (0)

static int run(SP_Table *t, int max) { int f = 0; while (SP_Step(t) && f < max) f++; return f; }

int main(void)
{
    SP_Table t; int i, mom0 = 0, mom1 = 0, f;

    // 1. A shot comes to rest and nothing leaves the table bounds
    SP_Init(&t);
    SP_Shoot(&t, 32, 200);                  // angle 32 = towards -x (rack)
    f = run(&t, 5000);
    CHECK(f < 5000);                        // settles
    CHECK(!SP_IsMoving(&t));
    for (i = 0; i < SP_NUM_BALLS; i++)
        if (t.ball[i].active) {
            CHECK(t.ball[i].x >= SP_LEFT && t.ball[i].x <= SP_RIGHT);
            CHECK(t.ball[i].y >= SP_TOP  && t.ball[i].y <= SP_BOTTOM);
        }
    printf("shot settled in %d frames, pocketed lo=%02x scratch=%d\n", f, t.pocketed_mask_lo, t.scratch);

    // 2. Head-on collision transfers momentum (equal masses)
    SP_Init(&t);
    for (i = 2; i < SP_NUM_BALLS; i++) t.ball[i].active = 0;
    t.ball[0].x = 100 << 4; t.ball[0].y = 80 << 4;
    t.ball[1].x = 120 << 4; t.ball[1].y = 80 << 4;
    t.ball[0].vx = 6 << 8;
    run(&t, 5);
    CHECK(t.ball[1].vx > (5 << 8) || t.ball[1].x > (121 << 4));
    CHECK(abs(t.ball[0].vx) < (1 << 8));

    // 3. A ball driven at a corner pocket is pocketed
    SP_Init(&t);
    for (i = 2; i < SP_NUM_BALLS; i++) t.ball[i].active = 0;
    t.ball[0].x = 40 << 4; t.ball[0].y = 50 << 4;
    t.ball[0].vx = -(3 << 8); t.ball[0].vy = -(3 << 8);
    t.ball[1].x = 200 << 4; t.ball[1].y = 100 << 4;
    run(&t, 500);
    CHECK(t.scratch == 1 && !t.ball[0].active);

    // 4. Rack is non-overlapping
    SP_Init(&t);
    for (i = 0; i < SP_NUM_BALLS; i++) for (f = i + 1; f < SP_NUM_BALLS; f++) {
        long dx = t.ball[i].x - t.ball[f].x, dy = t.ball[i].y - t.ball[f].y;
        CHECK(dx*dx + dy*dy >= (long)(2*SP_RADIUS)*(2*SP_RADIUS) - 64);
    }
    (void)mom0; (void)mom1;
    puts("all tests passed");
    return 0;
}
