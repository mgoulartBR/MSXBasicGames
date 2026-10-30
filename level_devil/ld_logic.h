/*
 * LEVEL DEVIL - MSX1 Edition: game logic (no hardware access).
 * Shared by the MSX build (level_devil.c) and the native room solver (tools/solver.c).
 * Expects u8/i8/u16/i16 to be defined by the includer.
 * Copyright (c) 2026 8bit/lab. SPDX-License-Identifier: CC-BY-SA-4.0
 */
#pragma once

#define NUM_ROOMS     20
#define FLOOR_Y       152          // top of the floor (pixels)
#define BLOCK_Y       136          // floor-level 16x16 objects (pusher, saw rolling on the floor)
#define PW            10           // player width
#define PH            12           // player height

// Dirty flags: the renderer redraws the tiles when these are set
#define DIRTY_FLOOR   1
#define DIRTY_PLAT    2

// Events: the renderer turns these into sound
#define EV_JUMP       1
#define EV_CRACK      2
#define EV_SPIKE      4
#define EV_DOOR       8
#define EV_WIN        16
#define EV_DIE        32

// PlayStep() results
#define PS_RUN        0
#define PS_DIE        1
#define PS_WIN        2

typedef struct { u8 ax, ay, bx, by, spd; } Saw;      // moves from (ax,ay) to (bx,by) and back; ax<=bx, ay<=by; spd==0: unused
typedef struct { u8 x, trig; } Drop;                 // ceiling spike at x, falls once PX >= trig; x==0: unused
typedef struct { u8 x, trig; i8 dir; u8 spd; } Push; // 16x16 block sliding on the floor once PX >= trig; spd==0: unused

struct Room {
    u8 gapX, gapW, gap2X, gap2W, gap3X, gap3W;   // permanent holes
    u8 crackX, crackW;             // floor that collapses when stepped on
    u8 spikeX, spikeW;             // floor spikes that pop up when approached
    u8 blockX, blockW;             // block that drops from the ceiling when approached
    u8 platX, platY;               // 32 px platform (platY = top, platX multiple of 8)
    i8 platDX;                     // platform shifts by this many px (multiple of 8) at every jump
    u8 doorX, doorMove;            // exit door; doorMove = px it flees when approached
    Saw saw[2];
    Drop drop[3];
    Push push;
};

#define R_END 220

const struct Room g_Rooms[NUM_ROOMS] = {
    /*  1 */ { .gapX=100,.gapW=40, .doorX=R_END,.doorMove=24 },
    /*  2 */ { .crackX=92,.crackW=44, .spikeX=170,.spikeW=24, .doorX=R_END },
    /*  3 */ { .spikeX=100,.spikeW=40, .gapX=164,.gapW=32, .doorX=R_END },
    /*  4 */ { .blockX=112,.blockW=16, .gapX=150,.gapW=44, .doorX=R_END },
    /*  5 */ { .crackX=84,.crackW=32, .gapX=140,.gapW=48, .spikeX=196,.spikeW=16, .doorX=R_END,.doorMove=36 },
    /*  6 */ { .saw={{88,136,168,136,2}}, .spikeX=184,.spikeW=24, .doorX=R_END },
    /*  7 */ { .gapX=96,.gapW=44, .drop={{150,92},{172,120},{196,150}}, .doorX=R_END },
    /*  8 */ { .gapX=80,.gapW=96, .platX=112,.platY=136,.platDX=24, .spikeX=184,.spikeW=24, .doorX=R_END },
    /*  9 */ { .gapX=120,.gapW=40, .gap2X=176,.gap2W=28, .push={0,0,1,3}, .drop={{204,160}}, .doorX=R_END },
    /* 10 */ { .crackX=76,.crackW=36, .saw={{128,112,184,112,2}}, .gapX=160,.gapW=40, .doorX=R_END },
    /* 11 */ { .gapX=64,.gapW=36, .blockX=124,.blockW=16, .spikeX=168,.spikeW=32, .doorX=R_END },
    /* 12 */ { .gapX=60,.gapW=44, .crackX=116,.crackW=28, .spikeX=162,.spikeW=28, .blockX=208,.blockW=16, .drop={{140,84}}, .doorX=224,.doorMove=32 },
    /* 13 */ { .gapX=88,.gapW=88, .platX=120,.platY=136,.platDX=-24, .saw={{104,100,168,100,2}}, .spikeX=186,.spikeW=20, .doorX=R_END },
    /* 14 */ { .gapX=150,.gapW=44, .push={240,40,-1,3}, .drop={{100,70},{124,96}}, .spikeX=200,.spikeW=16, .doorX=R_END },
    /* 15 */ { .crackX=68,.crackW=40, .saw={{112,136,152,136,2}}, .gapX=156,.gapW=44, .drop={{132,76}}, .doorX=R_END },
    /* 16 */ { .gapX=68,.gapW=40, .gap2X=128,.gap2W=40, .drop={{110,80},{150,110},{176,136}}, .doorX=R_END,.doorMove=40 },
    /* 17 */ { .spikeX=84,.spikeW=32, .push={0,0,1,3}, .gapX=150,.gapW=44, .saw={{110,112,148,112,2}}, .drop={{184,120}}, .doorX=R_END,.doorMove=28 },
    /* 18 */ { .gapX=76,.gapW=72, .platX=96,.platY=128,.platDX=24, .saw={{152,48,152,136,2},{176,112,216,112,2}}, .spikeX=190,.spikeW=16, .doorX=R_END },
    /* 19 */ { .crackX=60,.crackW=36, .gapX=116,.gapW=48, .blockX=104,.blockW=16, .saw={{172,136,212,136,2}}, .drop={{140,100}}, .doorX=R_END,.doorMove=44 },
    /* 20 */ { .gapX=52,.gapW=44, .crackX=108,.crackW=32, .spikeX=156,.spikeW=28, .push={240,150,-1,2}, .saw={{128,64,128,136,3},{190,136,214,136,2}}, .drop={{204,176}}, .doorX=224,.doorMove=32 }
};

struct GS {
    u8  PX;
    i16 PY, VY;
    u8  Grounded, Coyote, JumpBuffer, JumpWas;
    u8  Crack, Spike;
    u8  BlockState, BlockVy;
    u16 BlockPY;
    u8  DoorX, DoorMoved;
    u8  PlatX, PlatTarget, PlatTick;
    u8  SawPos[2];
    i8  SawDir[2];
    u8  DropState[3];      // 0 hanging, 1 falling, 2 stuck in the floor
    u16 DropY[3];          // 1/16 px
    u8  DropVy[3];
    u8  PushOn;
    i16 PushX;
    u8  Dirty, Ev;
};

struct GS g;
const struct Room* g_R;

u8 InRange(u8 x, u8 start, u8 width) {
    return width && x >= start && x < (u16)start + width;
}
u8 Solid(u8 x) {
    if(InRange(x, g_R->gapX, g_R->gapW)) return 0;
    if(InRange(x, g_R->gap2X, g_R->gap2W)) return 0;
    if(InRange(x, g_R->gap3X, g_R->gap3W)) return 0;
    if(g.Crack >= 10 && InRange(x, g_R->crackX, g_R->crackW)) return 0;
    return 1;
}

// Current position of a saw (top-left pixel)
u8 SawX(u8 i) { return g_R->saw[i].ax + (g_R->saw[i].bx > g_R->saw[i].ax ? g.SawPos[i] : 0); }
u8 SawY(u8 i) { return g_R->saw[i].ay + (g_R->saw[i].by > g_R->saw[i].ay ? g.SawPos[i] : 0); }

void ResetRoomState(u8 level) {
    u8 i;
    g_R = &g_Rooms[level];
    g.PX = 22; g.PY = 140 * 16; g.VY = 0;
    g.Grounded = 1; g.Coyote = 6; g.JumpBuffer = 0; g.JumpWas = 1;
    g.Crack = 0; g.Spike = 0; g.BlockState = 0; g.BlockVy = 0;
    g.BlockPY = 32 * 16; g.DoorX = g_R->doorX; g.DoorMoved = 0;
    g.PlatX = g_R->platX; g.PlatTarget = g_R->platX; g.PlatTick = 0;
    for(i = 0; i < 2; i++) { g.SawPos[i] = 0; g.SawDir[i] = 1; }
    for(i = 0; i < 3; i++) { g.DropState[i] = 0; g.DropY[i] = 24 * 16; g.DropVy[i] = 0; }
    g.PushOn = (g_R->push.spd && g_R->push.trig == 0);
    g.PushX = g_R->push.x;
    g.Dirty = DIRTY_FLOOR | DIRTY_PLAT; g.Ev = 0;
}

static u8 Overlap(i16 ax, i16 ay, i16 aw, i16 ah, i16 bx, i16 by, i16 bw, i16 bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

// One frame of gameplay. Returns PS_RUN / PS_DIE / PS_WIN.
u8 PlayStep(u8 left, u8 right, u8 jump) {
    u8 nx, landed, i, moved;
    i16 oldY, newY, landY, py, feet;
    const Saw* s;

    g.Ev = 0;
    if(jump && !g.JumpWas) g.JumpBuffer = 6;
    g.JumpWas = jump;

    if(g.Coyote) g.Coyote--;
    if(g.Grounded) g.Coyote = 6;
    if(g.JumpBuffer && g.Coyote) {
        g.VY = -76; g.Grounded = 0; g.Coyote = 0; g.JumpBuffer = 0; g.Ev |= EV_JUMP;
        if(g_R->platDX) g.PlatTarget = (g.PlatTarget == g_R->platX) ? (u8)(g_R->platX + g_R->platDX) : g_R->platX;
    }
    if(g.JumpBuffer) g.JumpBuffer--;

    // ---- horizontal movement
    nx = g.PX;
    if(left && nx > 2) nx -= 2;
    if(right && nx < 244) nx += 2;
    feet = (g.PY + PH * 16) >> 4;
    if(g_R->platY && g.PY + 192 > (i16)g_R->platY * 16 && g.PY < (i16)(g_R->platY + 8) * 16) {
        if(g.PX + PW <= g.PlatX && nx + PW > g.PlatX) nx = g.PlatX - PW;
        if(g.PX >= g.PlatX + 32 && nx < g.PlatX + 32) nx = g.PlatX + 32;
    }
    if(g_R->push.spd && feet > BLOCK_Y + 1 && Overlap(nx, 0, PW, 1, g.PushX, 0, 16, 1)) nx = g.PX;
    g.PX = nx;

    // ---- floor traps
    if(g_R->crackW && (u16)g.PX + 10 > g_R->crackX && g.PX < (u16)g_R->crackX + g_R->crackW && g.Crack < 250) {
        g.Crack++;
        if(g.Crack == 10) { g.Dirty |= DIRTY_FLOOR; g.Ev |= EV_CRACK; }
    }
    if(g_R->spikeW && (u16)g.PX + 30 > g_R->spikeX && g.PX < (u16)g_R->spikeX + g_R->spikeW + 10 && g.Spike < 250) {
        g.Spike++;
        if(g.Spike == 6) { g.Dirty |= DIRTY_FLOOR; g.Ev |= EV_SPIKE; }
    }
    // ---- falling block
    if(g_R->blockW && (u16)g.PX + 48 > g_R->blockX) g.BlockState = 1;
    if(g.BlockState && g.BlockPY < 136 * 16) {
        g.BlockVy += 3;
        g.BlockPY += g.BlockVy;
        if(g.BlockPY >= 136 * 16) { g.BlockPY = 136 * 16; g.BlockVy = 0; }
    }
    // ---- fleeing door
    if(g_R->doorMove && !g.DoorMoved && (u16)g.PX + 44 > g_R->doorX) {
        g.DoorX -= g_R->doorMove; g.DoorMoved = 1; g.Ev |= EV_DOOR;
    }
    // ---- saws
    for(i = 0; i < 2; i++) {
        s = &g_R->saw[i];
        if(!s->spd) continue;
        {
            u8 len = (s->bx - s->ax) + (s->by - s->ay);
            if(g.SawDir[i] > 0) {
                g.SawPos[i] += s->spd;
                if(g.SawPos[i] >= len) { g.SawPos[i] = len; g.SawDir[i] = -1; }
            } else {
                if(g.SawPos[i] <= s->spd) { g.SawPos[i] = 0; g.SawDir[i] = 1; }
                else g.SawPos[i] -= s->spd;
            }
        }
    }
    // ---- ceiling spikes
    for(i = 0; i < 3; i++) {
        if(!g_R->drop[i].x) continue;
        if(g.DropState[i] == 0 && g.PX >= g_R->drop[i].trig) g.DropState[i] = 1;
        if(g.DropState[i] == 1) {
            g.DropVy[i] += 4;
            g.DropY[i] += g.DropVy[i];
            if(g.DropY[i] >= BLOCK_Y * 16) { g.DropY[i] = BLOCK_Y * 16; g.DropState[i] = 2; }
        }
    }
    // ---- sliding platform (moves one tile every 3 frames)
    moved = 0;
    if(g.PlatX != g.PlatTarget) {
        if(++g.PlatTick >= 3) {
            g.PlatTick = 0;
            if(g.PlatX < g.PlatTarget) g.PlatX += 8; else g.PlatX -= 8;
            g.Dirty |= DIRTY_PLAT; moved = 1;
        }
    }
    if(moved && g_R->platY && Overlap(g.PX, g.PY >> 4, PW, PH, g.PlatX, g_R->platY, 32, 8)) {
        // the platform shoves the player sideways
        if(g.PX + PW / 2 < g.PlatX + 16) g.PX = (g.PlatX >= PW + 2) ? g.PlatX - PW : 2;
        else g.PX = (g.PlatX + 32 <= 244) ? g.PlatX + 32 : 244;
    }
    // ---- sliding pusher block
    if(g_R->push.spd) {
        if(!g.PushOn && g.PX >= g_R->push.trig) g.PushOn = 1;
        if(g.PushOn) {
            g.PushX += g_R->push.dir * g_R->push.spd;
            if(g.PushX < 0) { g.PushX = 0; g.PushOn = 0; }
            if(g.PushX > 240) { g.PushX = 240; g.PushOn = 0; }
            feet = (g.PY + PH * 16) >> 4;
            if(feet > BLOCK_Y + 1 && Overlap(g.PX, 0, PW, 1, g.PushX, 0, 16, 1)) {
                i16 np;
                if(g_R->push.dir > 0) np = g.PushX + 16; else np = g.PushX - PW;
                if(np < 2 || np > 244) { g.Ev |= EV_DIE; return PS_DIE; }   // crushed against the wall
                g.PX = (u8)np;
            }
        }
    }

    // ---- vertical movement
    g.VY += 4;
    if(g.VY > 96) g.VY = 96;
    oldY = g.PY; newY = oldY + g.VY;
    g.Grounded = 0; landed = 0; landY = FLOOR_Y * 16;
    if(g.VY >= 0) {
        if(oldY + 192 <= FLOOR_Y * 16 && newY + 192 >= FLOOR_Y * 16 && (Solid(g.PX + 2) || Solid(g.PX + 8))) landed = 1;
        if(g_R->platY && g.PX + 10 > g.PlatX && g.PX < g.PlatX + 32 && oldY + 192 <= (i16)g_R->platY * 16 && newY + 192 >= (i16)g_R->platY * 16) {
            landed = 1; landY = (i16)g_R->platY * 16;
        }
        if(g_R->push.spd && g.PX + 10 > g.PushX && g.PX < g.PushX + 16 && oldY + 192 <= BLOCK_Y * 16 && newY + 192 >= BLOCK_Y * 16) {
            landed = 1; landY = BLOCK_Y * 16;
        }
        if(landed) { newY = landY - 192; g.VY = 0; g.Grounded = 1; }
    } else if(g_R->platY && g.PX + 10 > g.PlatX && g.PX < g.PlatX + 32 && oldY >= (i16)(g_R->platY + 8) * 16 && newY < (i16)(g_R->platY + 8) * 16) {
        newY = (i16)(g_R->platY + 8) * 16; g.VY = 0;
    }
    if(newY < 22 * 16) { newY = 22 * 16; g.VY = 0; }
    g.PY = newY;
    if(g.PY > 192 * 16) { g.Ev |= EV_DIE; return PS_DIE; }

    // ---- hazards
    py = g.PY >> 4;
    if(g.Spike >= 6 && g.PX + 8 > g_R->spikeX + 2 && g.PX + 2 < (u16)g_R->spikeX + g_R->spikeW - 2 && g.PY + 192 > 144 * 16 && g.PY < 152 * 16) { g.Ev |= EV_DIE; return PS_DIE; }
    if(g.BlockState && g.PX + 9 > g_R->blockX + 1 && g.PX + 1 < (u16)g_R->blockX + 15 && g.PY + 192 > (g.BlockPY >> 4) * 16 + 16 && g.PY < ((g.BlockPY >> 4) + 15) * 16) { g.Ev |= EV_DIE; return PS_DIE; }
    for(i = 0; i < 3; i++) {
        if(i < 2 && g_R->saw[i].spd && Overlap(g.PX + 1, py + 1, PW - 2, PH - 2, SawX(i) + 3, SawY(i) + 3, 10, 10)) { g.Ev |= EV_DIE; return PS_DIE; }
        if(g_R->drop[i].x && g.DropState[i] && Overlap(g.PX + 1, py + 1, PW - 2, PH - 2, g_R->drop[i].x + 4, (g.DropY[i] >> 4) + 2, 8, 13)) { g.Ev |= EV_DIE; return PS_DIE; }
    }
    // ---- exit
    if(g.PX + 10 > g.DoorX + 3 && g.PX < g.DoorX + 13 && g.PY + 192 > 132 * 16 && g.PY < 152 * 16) { g.Ev |= EV_WIN; return PS_WIN; }
    return PS_RUN;
}
