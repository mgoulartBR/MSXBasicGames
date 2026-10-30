/* Native room solver for Level Devil: BFS over every reachable game state.
 * Usage: solver <K> [room 1..20]   (K = frames each key press is held; K=1 is exact)
 * Build: gcc -O2 -o solver tools/solver.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef int8_t i8; typedef uint16_t u16; typedef int16_t i16;
#include "../ld_logic.h"

#define MAXN 12000000u
static struct GS *nodes; static int32_t *parent; static u8 *act; static u8 *frames_used;
static uint64_t *tab; static const uint64_t TABSZ = 1ull << 26;

static uint64_t hashgs(const struct GS *s) {
    struct GS t = *s; t.Dirty = 0; t.Ev = 0;
    const u8 *p = (const u8*)&t; uint64_t h = 1469598103934665603ull;
    for(size_t i = 0; i < sizeof t; i++) { h ^= p[i]; h *= 1099511628211ull; }
    return h ? h : 1;
}
static int seen(uint64_t h) {            /* returns 1 if already present, else inserts */
    uint64_t i = h & (TABSZ - 1);
    while(tab[i]) { if(tab[i] == h) return 1; i = (i + 1) & (TABSZ - 1); }
    tab[i] = h; return 0;
}
static const char *ACT[6] = {"..", "L.", "R.", ".J", "LJ", "RJ"};

#define MAXF 6000
static int32_t bhead[MAXF + 1]; static int32_t *nxt; static uint16_t *gcost;

static int hcost(const struct GS *s) {       /* admissible: frames needed to walk to the door */
    int d = (int)s->DoorX - 3 - (int)s->PX;  /* door may flee: use the final position instead */
    int fin = (int)g_R->doorX - (g_R->doorMove ? g_R->doorMove : 0) - 3 - (int)s->PX;
    if(fin < d) d = fin;
    return d > 0 ? d / 2 : 0;
}

static int solve(int room, int K, int maxframes, int verbose, long *explored) {
    memset(&g, 0, sizeof g);
    ResetRoomState(room - 1);
    memset(tab, 0, TABSZ * 8);
    for(int i = 0; i <= MAXF; i++) bhead[i] = -1;
    size_t n = 1; int cur_f = 0;
    nodes[0] = g; parent[0] = -1; act[0] = 0; gcost[0] = 0; nxt[0] = -1;
    seen(hashgs(&g));
    bhead[hcost(&g)] = 0;
    while(1) {
        while(cur_f <= MAXF && bhead[cur_f] < 0) cur_f++;
        if(cur_f > MAXF) break;
        int cur_i = bhead[cur_f]; bhead[cur_f] = nxt[cur_i];
        struct GS cur = nodes[cur_i];
        int gc = gcost[cur_i];
        if(gc * K >= maxframes) continue;
        for(int a = 0; a < 6; a++) {
            u8 l = (a == 1 || a == 4), r = (a == 2 || a == 5), j = (a >= 3);
            g = cur; u8 res = PS_RUN; int f;
            for(f = 0; f < K; f++) { res = PlayStep(l, r, j); if(res != PS_RUN) break; }
            if(res == PS_DIE) continue;
            if(res == PS_WIN) {
                *explored = (long)n;
                if(verbose) {
                    static char buf[60000]; int pos = 0; int idx = cur_i; int steps = 0; static int path[20000];
                    path[steps++] = a;
                    while(idx > 0) { path[steps++] = act[idx]; idx = parent[idx]; }
                    for(int i = steps - 1; i >= 0; i--) pos += sprintf(buf + pos, "%s ", ACT[path[i]]);
                    printf("  path (%d frames per symbol): %s\n", K, buf);
                }
                return (gc + 1) * K;
            }
            if(seen(hashgs(&g))) continue;
            if(n >= MAXN) { *explored = (long)n; return -2; }
            nodes[n] = g; parent[n] = cur_i; act[n] = (u8)a; gcost[n] = gc + 1;
            int fc = gc + 1 + hcost(&g); if(fc > MAXF) fc = MAXF;
            nxt[n] = bhead[fc]; bhead[fc] = (int32_t)n;
            if(fc < cur_f) cur_f = fc;
            n++;
        }
    }
    *explored = (long)n;
    return -1;
}


/* Monte-Carlo difficulty probe: a "blind" player holds right and jumps at random moments. */
static uint32_t rs = 12345;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static double mc_rate(int room, int trials, int pj) {
    int wins = 0;
    for(int t = 0; t < trials; t++) {
        memset(&g, 0, sizeof g); ResetRoomState(room - 1);
        for(int f = 0; f < 700; f++) {
            u8 r = (rnd() % 100) < 92, l = (!r) && (rnd() % 100) < 30;
            u8 j = (rnd() % 1000) < (uint32_t)pj;
            u8 res = PlayStep(l, r, j);
            if(res == PS_WIN) { wins++; break; }
            if(res == PS_DIE) break;
        }
    }
    return 100.0 * wins / trials;
}

int main(int argc, char **argv) {
    int K = argc > 1 ? atoi(argv[1]) : 1;
    int only = argc > 2 ? atoi(argv[2]) : 0;
    int verbose = argc > 3;
    nodes = malloc(sizeof(struct GS) * MAXN); parent = malloc(4 * MAXN); act = malloc(MAXN); frames_used = malloc(MAXN); nxt = malloc(4 * MAXN); gcost = malloc(2 * MAXN);
    tab = malloc(TABSZ * 8);
    for(int r = 1; r <= NUM_ROOMS; r++) {
        if(only && r != only) continue;
        long ex = 0; int f = solve(r, K, 3000, verbose, &ex);
        int walk = (g_Rooms[r-1].doorX - g_Rooms[r-1].doorMove - 3 - 22) / 2;
        printf("room %2d: %-11s frames=%4d (walk %3d, +%3d)  blind-win%%: %5.2f %5.2f  states=%ld\n", r, f > 0 ? "SOLVABLE" : (f == -2 ? "OVERFLOW" : "NO SOLUTION"), f, walk, f - walk,
               mc_rate(r, 20000, 40), mc_rate(r, 20000, 15), ex);
        fflush(stdout);
    }
    return 0;
}
