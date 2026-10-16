// Trick-shot layouts (generated, see tools/trick_gen.c).  Success = every target ball pocketed in one shot,
// cue ball not pocketed; the other balls are obstacles.  The player has TRICK_TRIES attempts per trick.
#ifndef TRICKS_H
#define TRICKS_H
#define TRICK_COUNT 19
#define TRICK_TRIES 3
typedef struct {
    unsigned char cx, cy;          // cue ball
    unsigned char n;               // object balls (1..4), numbered 1..n
    unsigned char x[4], y[4];
    unsigned char targets;         // bit i = ball i+1 must be pocketed
} Trick;
extern const Trick g_Tricks[TRICK_COUNT];
#endif
