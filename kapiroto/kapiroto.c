/*
 * KAPIROTO - MSX1
 * Inspired by Level Devil by Unept.
 * Copyright (c) 2026 8bit/lab. SPDX-License-Identifier: CC-BY-SA-4.0
 * Build: MSXgl, Machine=1, Target=ROM_32K.
 * Reference source: validate with the pinned SDK and test on MSX1.
 *
 * The game rules live in ld_logic.h (shared with tools/solver.c); this file only
 * handles input, drawing (SCREEN 1 tiles + hardware sprites), text and sound.
 */
#include "msxgl.h"
#include "input.h"
#include "psg.h"
#include "font/font_mgl_sample6.h"
#include "ld_logic.h"
#include "build_config.h"
#include "screens.h"

// Level order: the full game plays rooms 1-20; the demo plays a hand-picked selection of 5
#ifdef KAPIROTO_DEMO
#define NUM_LEVELS  5
const u8 g_Order[NUM_LEVELS] = { 0, 1, 5, 7, 8 };
#define ROOM_OF(l)  g_Order[l]
#else
#define NUM_LEVELS  NUM_ROOMS
#define ROOM_OF(l)  (l)
#endif

// Tiles: 0 empty, 1 floor, 2 cracked floor, 3 floor spike
const u8 g_Tiles[40] = {
    0,0,0,0,0,0,0,0,
    255,255,239,255,255,255,127,255,
    255,239,239,247,255,255,127,255,
    16,16,56,56,124,124,254,254,
    255,195,165,153,153,165,195,255
};
// Sprite patterns (16x16, 4 each): 0 player, 4 door top, 8 door bottom, 12 block, 16 saw, 20 ceiling spike
const u8 g_SpriteData[192] = {
    0,32,48,63,63,39,39,63,
    63,63,63,56,0,0,0,0,
    0,16,48,240,240,48,48,240,
    240,240,240,112,0,0,0,0,
    7,15,31,28,24,27,27,27,
    27,27,27,27,27,27,27,27,
    224,240,248,56,24,216,216,216,
    216,216,216,216,216,216,216,216,
    27,27,27,27,27,27,31,31,
    0,0,0,0,0,0,0,0,
    216,216,216,216,216,216,248,248,
    0,0,0,0,0,0,0,0,
    255,255,224,208,200,196,194,193,
    193,194,196,200,208,224,255,255,
    255,255,7,11,19,35,67,131,
    131,67,35,19,11,7,255,255,
    1,1,55,63,31,63,60,252,
    252,60,63,31,63,55,1,1,
    128,128,236,252,248,252,60,63,
    63,60,252,248,252,236,128,128,
    63,63,63,63,31,31,31,15,
    15,7,7,3,3,3,1,1,
    252,252,252,248,248,240,240,240,
    224,224,192,192,128,128,128,128
};

// Sprite planes
#define SP_PLAYER   0
#define SP_DOOR     1   // 1,2
#define SP_BLOCK    3
#define SP_PUSH     4
#define SP_SAW      5   // 5,6
#define SP_DROP     7   // 7,8,9
#define SP_END      10
#define SPR_HIDE    200

// Game states
#define ST_INFO     0   // controls screen (room 1 behind)
#define ST_PLAY     1
#define ST_DEAD     2
#define ST_CLEAR    3
#define ST_PAUSE    4
#define ST_TITLE    5   // SCREEN 2 title picture
#define ST_END      6   // SCREEN 2 ending picture

volatile u8 g_Tick = 0;
u8 g_Level=0, g_Deaths=0, g_State=ST_TITLE, g_Timer=0;
u8 g_PauseWas=0, g_ResetWas=0, g_MuteWas=0, g_StartWas=0;
u8 g_Sound=1, g_SoundTimer=0, g_PlatDrawn=255;

void TickHook(void) { g_Tick=1; }
void WaitFrame(void) {
    while(!g_Tick) Halt();
    g_Tick=0;
}
void Sound(u16 period) {
    if(!g_Sound) return;
    PSG_SetTone(0, period);
    PSG_SetVolume(0, 12);
    g_SoundTimer=14;
}
void Tile(u8 x, u8 y, u8 t) {
    VDP_Poke_16K(t, g_ScreenLayoutLow+(u16)y*32+x);
}
void DrawFloor(void) {
    u8 x,y,t;
    for(x=0;x<32;x++) {
        t=Solid(x*8+4) ? 1 : 0;
        for(y=19;y<23;y++) Tile(x,y,t);
        if(g_R->crackW && g.Crack<10 && Solid(x*8+4)) Tile(x,19,2);
        t=(g.Spike>=6 && InRange(x*8+4,g_R->spikeX,g_R->spikeW)) ? 3 : 0;
        Tile(x,18,t);
    }
}
void DrawPlat(void) {
    u8 i,row;
    if(!g_R->platY) return;
    row=g_R->platY>>3;
    if(g_PlatDrawn!=255) for(i=0;i<4;i++) Tile((g_PlatDrawn>>3)+i,row,0);
    for(i=0;i<4;i++) Tile((g.PlatX>>3)+i,row,1);
    g_PlatDrawn=g.PlatX;
}
void ClearMessage(void) {
    VDP_FillVRAM_16K(0,g_ScreenLayoutLow+4*32,12*32);
}
void DrawHeader(void) {
    Print_SetPosition(1,1);
    Print_DrawText("ROOM ");
    Print_DrawInt(g_Level+1);
    Print_DrawText("/");
    Print_DrawInt(NUM_LEVELS);
    Print_SetPosition(18,1);
    Print_DrawText("DEATHS ");
    Print_DrawInt(g_Deaths);
}
void DrawRoom(void) {
    VDP_EnableDisplay(FALSE);
    VDP_FillVRAM_16K(0,g_ScreenLayoutLow,768);
    g_PlatDrawn=255;
    DrawFloor();
    DrawPlat();
    DrawHeader();
    VDP_EnableDisplay(TRUE);
}
void ResetRoom(void) {
    ResetRoomState(ROOM_OF(g_Level));
    DrawRoom();
}
void DrawSprites(void) {
    u8 i;
    VDP_SetSpriteSM1(SP_PLAYER,g.PX,(u8)(g.PY>>4)-1,0,COLOR_BLACK);
    VDP_SetSpriteSM1(SP_DOOR,g.DoorX,127,4,COLOR_DARK_GREEN);
    VDP_SetSpriteSM1(SP_DOOR+1,g.DoorX,143,8,COLOR_DARK_GREEN);
    if(g.BlockState) VDP_SetSpriteSM1(SP_BLOCK,g_R->blockX,(u8)(g.BlockPY>>4)-1,12,COLOR_BLACK);
    else VDP_SetSpriteSM1(SP_BLOCK,0,SPR_HIDE,12,COLOR_BLACK);
    if(g_R->push.spd) VDP_SetSpriteSM1(SP_PUSH,(u8)g.PushX,BLOCK_Y-1,12,COLOR_BLACK);
    else VDP_SetSpriteSM1(SP_PUSH,0,SPR_HIDE,12,COLOR_BLACK);
    for(i=0;i<2;i++) {
        if(g_R->saw[i].spd) VDP_SetSpriteSM1(SP_SAW+i,SawX(i),SawY(i)-1,16,COLOR_MEDIUM_RED);
        else VDP_SetSpriteSM1(SP_SAW+i,0,SPR_HIDE,16,COLOR_MEDIUM_RED);
    }
    for(i=0;i<3;i++) {
        if(g_R->drop[i].x) VDP_SetSpriteSM1(SP_DROP+i,g_R->drop[i].x,(u8)(g.DropY[i]>>4)-1,20,COLOR_DARK_RED);
        else VDP_SetSpriteSM1(SP_DROP+i,0,SPR_HIDE,20,COLOR_DARK_RED);
    }
}
void Die(void) {
    if(g_Deaths<255) g_Deaths++;
    g_State=ST_DEAD; g_Timer=34;
    Sound(900);
    DrawHeader();
}
void StartRoom(void) {
    ClearMessage();
    VDP_FillVRAM_16K(0,g_ScreenLayoutLow+23*32,32);
    DrawHeader();
    g_State=ST_PLAY;
}
// RLE: 0x00-0x7F literal run of n+1 bytes, 0x80-0xFE repeat next byte (n-0x7D) times, 0xFF end
void UnpackVRAM(const u8* src, u16 dst) {
    u8 c;
    while((c=*src++)!=0xFF) {
        if(c<0x80) { c++; VDP_WriteVRAM_16K(src,dst,c); src+=c; dst+=c; }
        else { c-=0x7D; VDP_FillVRAM_16K(*src++,dst,c); dst+=c; }
    }
}
void ShowImage(const u8* pat, const u8* col, u8 backdrop) {
    u16 i;
    VDP_EnableDisplay(FALSE);
    VDP_SetMode(VDP_MODE_SCREEN2);
    VDP_SetColor(backdrop);
    VDP_FillVRAM_16K(0,0,0x4000);
    UnpackVRAM(pat,g_ScreenPatternLow);
    UnpackVRAM(col,g_ScreenColorLow);
    for(i=0;i<768;i++) VDP_Poke_16K((u8)i,g_ScreenLayoutLow+i);
    VDP_DisableSpritesFrom(0);
    VDP_EnableDisplay(TRUE);
}
void ShowTitle(void) {
#ifdef KAPIROTO_DEMO
    ShowImage(g_TitleDemo_Pat,g_TitleDemo_Col,COLOR_WHITE);
#else
    ShowImage(g_Title_Pat,g_Title_Col,COLOR_WHITE);
#endif
    g_State=ST_TITLE;
}
void InitGameVideo(void) {
    VDP_EnableDisplay(FALSE);
    VDP_SetMode(VDP_MODE_SCREEN1);
    VDP_SetColor(COLOR_WHITE);
    VDP_FillVRAM_16K(0,0,0x4000);
    VDP_WriteVRAM_16K(g_Tiles,g_ScreenPatternLow,40);
    Print_SetTextFont(g_Font_MGL_Sample6,32);
    Print_SetColor(COLOR_BLACK,COLOR_WHITE);
    VDP_FillVRAM_16K(0x1F,g_ScreenColorLow,32);
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);
    VDP_LoadSpritePattern(g_SpriteData,0,24);
    VDP_DisableSpritesFrom(SP_END);
}
void ShowInfo(void) {
    InitGameVideo();
    g_Level=0; g_Deaths=0;
    ResetRoom();
    Print_SetPosition(10,5);  Print_DrawText("ARROWS: MOVE");
    Print_SetPosition(10,7);  Print_DrawText("SPACE: JUMP");
    Print_SetPosition(10,9);  Print_DrawText("R: RESTART");
    Print_SetPosition(7,11);  Print_DrawText("P: PAUSE  M: SOUND");
    Print_SetPosition(8,23);  Print_DrawText("BigFive Studios");
    g_State=ST_INFO;
}
void ShowEnd(void) {
#ifdef KAPIROTO_DEMO
    InitGameVideo();
    Print_SetPosition(9,5);   Print_DrawText("DEMO COMPLETE!");
    Print_SetPosition(10,7);  Print_DrawText("DEATHS: "); Print_DrawInt(g_Deaths);
    Print_SetPosition(5,11);  Print_DrawText("THE FULL GAME HAS 20 ROOMS");
    Print_SetPosition(5,13);  Print_DrawText("AND MANY MORE WAYS TO DIE");
    Print_SetPosition(5,17);  Print_DrawText("bigfivestudios.itch.io");
    Print_SetPosition(9,18);  Print_DrawText("/kapiroto-msx");
    Print_SetPosition(8,23);  Print_DrawText("BigFive Studios");
    VDP_EnableDisplay(TRUE);
#else
    ShowImage(g_End_Pat,g_End_Col,COLOR_DARK_BLUE);
#endif
    g_State=ST_END;
}
void Update(void) {
    u8 row8,dir,left,right,jump,pauseNow,resetNow,muteNow,startNow,res;
    row8=Keyboard_Read(8);
    dir=Joystick_GetDirection(JOY_PORT_1);
    left=IS_KEY_PRESSED(row8,KEY_LEFT)||Keyboard_IsKeyPressed(KEY_A)||dir==JOY_INPUT_DIR_LEFT;
    right=IS_KEY_PRESSED(row8,KEY_RIGHT)||Keyboard_IsKeyPressed(KEY_D)||dir==JOY_INPUT_DIR_RIGHT;
    jump=IS_KEY_PRESSED(row8,KEY_SPACE)||Joystick_IsButtonPressed(JOY_PORT_1,JOY_INPUT_TRIGGER_A)||Joystick_IsButtonPressed(JOY_PORT_1,JOY_INPUT_TRIGGER_B);
    pauseNow=Keyboard_IsKeyPressed(KEY_P)||Keyboard_IsKeyPressed(KEY_ESC);
    resetNow=Keyboard_IsKeyPressed(KEY_R);
    muteNow=Keyboard_IsKeyPressed(KEY_M);
    startNow=jump||Keyboard_IsKeyPressed(KEY_RETURN);
    if(muteNow && !g_MuteWas) {
        g_Sound=!g_Sound;
        if(!g_Sound) PSG_SetVolume(0,0);
    }
    g_MuteWas=muteNow;
    if(pauseNow && !g_PauseWas && (g_State==ST_PLAY || g_State==ST_PAUSE)) {
        if(g_State==ST_PLAY) {
            g_State=ST_PAUSE;
            Print_SetPosition(13,5); Print_DrawText("PAUSED");
        } else { ClearMessage(); g_State=ST_PLAY; }
    }
    g_PauseWas=pauseNow;
    if(resetNow && !g_ResetWas && g_State>=ST_PLAY && g_State<=ST_PAUSE) { ResetRoom(); g_State=ST_PLAY; }
    g_ResetWas=resetNow;
    if(g_State==ST_TITLE) {
        if(startNow && !g_StartWas) ShowInfo();
    } else if(g_State==ST_INFO) {
        if(startNow && !g_StartWas) StartRoom();
    } else if(g_State==ST_END) {
        if(startNow && !g_StartWas) ShowTitle();
    } else if(g_State==ST_DEAD) {
        if(--g_Timer==0) { ResetRoom(); g_State=ST_PLAY; }
    } else if(g_State==ST_CLEAR) {
        if(--g_Timer==0) {
            g_Level++;
            if(g_Level==NUM_LEVELS) ShowEnd();
            else { ResetRoom(); g_State=ST_PLAY; }
        }
    } else if(g_State==ST_PLAY) {
        res=PlayStep(left,right,jump);
        if(g.Dirty & DIRTY_FLOOR) DrawFloor();
        if(g.Dirty & DIRTY_PLAT) DrawPlat();
        g.Dirty=0;
        if(g.Ev & EV_JUMP) Sound(240);
        if(g.Ev & EV_CRACK) Sound(650);
        if(g.Ev & EV_SPIKE) Sound(450);
        if(g.Ev & EV_DOOR) Sound(330);
        if(res==PS_DIE) Die();
        else if(res==PS_WIN) { g_State=ST_CLEAR; g_Timer=40; Sound(120); }
    }
    g_StartWas=startNow;
    if(g_SoundTimer) {
        g_SoundTimer--;
        PSG_SetVolume(0,g_Sound ? (g_SoundTimer>7 ? 10 : g_SoundTimer/2) : 0);
    }
}
void main(void) {
    *(volatile u8*)0xF3DB = 0; // CLIKSW: keyclick off
    PSG_SetMixer(PSG_TONE_A_ON | 0x40); // bit 6: keep PSG port A as input (joystick)
    PSG_SetVolume(0,0);
    ShowTitle();
    BIOS_SetHookCallback(H_TIMI,TickHook);
    VDP_EnableVBlank(TRUE);
    while(1) {
        WaitFrame();
        Update();
        if(g_State<ST_TITLE) DrawSprites();
    }
}
