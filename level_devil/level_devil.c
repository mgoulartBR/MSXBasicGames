/*
 * LEVEL DEVIL - MSX1 Edition
 * Fan-made independent adaptation. Original game by Unept.
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

volatile u8 g_Tick = 0;
u8 g_Level=0, g_Deaths=0, g_State=0, g_Timer=0;
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
    Print_DrawText("SALA ");
    Print_DrawInt(g_Level+1);
    Print_DrawText("/20");
    Print_SetPosition(18,1);
    Print_DrawText("MORTES ");
    Print_DrawInt(g_Deaths);
    Print_SetPosition(1,23);
    Print_DrawText("R:REINICIAR P:PAUSA M:SOM");
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
    ResetRoomState(g_Level);
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
    g_State=2; g_Timer=34;
    Sound(900);
    Print_SetPosition(6,5);
    Print_DrawText("MAIS UMA TENTATIVA!");
    Print_SetPosition(8,7);
    Print_DrawText("MORTES: "); Print_DrawInt(g_Deaths);
}
void StartRoom(void) {
    ClearMessage();
    DrawHeader();
    g_State=1;
}
void Update(void) {
    u8 row8,dir,left,right,jump,pauseNow,resetNow,muteNow,startNow,res;
    row8=Keyboard_Read(8);
    dir=Joystick_GetDirection(JOY_PORT_1);
    left=IS_KEY_PRESSED(row8,KEY_LEFT)||IS_KEY_PRESSED(row8,KEY_A)||dir==JOY_INPUT_DIR_LEFT;
    right=IS_KEY_PRESSED(row8,KEY_RIGHT)||IS_KEY_PRESSED(row8,KEY_D)||dir==JOY_INPUT_DIR_RIGHT;
    jump=IS_KEY_PRESSED(row8,KEY_SPACE)||IS_KEY_PRESSED(row8,KEY_UP)||IS_KEY_PRESSED(row8,KEY_W)||dir==JOY_INPUT_DIR_UP||Joystick_IsButtonPressed(JOY_PORT_1,JOY_INPUT_TRIGGER_A)||Joystick_IsButtonPressed(JOY_PORT_1,JOY_INPUT_TRIGGER_B);
    pauseNow=Keyboard_IsKeyPressed(KEY_P)||Keyboard_IsKeyPressed(KEY_ESC);
    resetNow=Keyboard_IsKeyPressed(KEY_R);
    muteNow=Keyboard_IsKeyPressed(KEY_M);
    startNow=jump||Keyboard_IsKeyPressed(KEY_RETURN);
    if(muteNow && !g_MuteWas) {
        g_Sound=!g_Sound;
        if(!g_Sound) PSG_SetVolume(0,0);
    }
    g_MuteWas=muteNow;
    if(pauseNow && !g_PauseWas && (g_State==1 || g_State==4)) {
        if(g_State==1) {
            g_State=4;
            Print_SetPosition(5,5); Print_DrawText("PAUSA - P PARA VOLTAR");
        } else { ClearMessage(); DrawHeader(); g_State=1; }
    }
    g_PauseWas=pauseNow;
    if(resetNow && !g_ResetWas && g_State!=0) { ResetRoom(); g_State=1; }
    g_ResetWas=resetNow;
    if(g_State==0) {
        if(startNow && !g_StartWas) StartRoom();
    } else if(g_State==2) {
        if(--g_Timer==0) { ResetRoom(); g_State=1; }
    } else if(g_State==3) {
        if(startNow && !g_StartWas) {
            g_Level++;
            if(g_Level==NUM_ROOMS) { g_Level=0; g_Deaths=0; }
            ResetRoom(); g_State=1;
        }
    } else if(g_State==1) {
        res=PlayStep(left,right,jump);
        if(g.Dirty & DIRTY_FLOOR) DrawFloor();
        if(g.Dirty & DIRTY_PLAT) DrawPlat();
        g.Dirty=0;
        if(g.Ev & EV_JUMP) Sound(240);
        if(g.Ev & EV_CRACK) Sound(650);
        if(g.Ev & EV_SPIKE) Sound(450);
        if(g.Ev & EV_DOOR) Sound(330);
        if(res==PS_DIE) Die();
        else if(res==PS_WIN) {
            g_State=3; Sound(120);
            Print_SetPosition(7,5); Print_DrawText("SALA CONCLUIDA!");
            Print_SetPosition(5,7); Print_DrawText("ENTER: PROXIMA SALA");
            if(g_Level==NUM_ROOMS-1) {
                Print_SetPosition(4,9); Print_DrawText("20 SALAS. VOCE VENCEU!");
            }
        }
    }
    g_StartWas=startNow;
    if(g_SoundTimer) {
        g_SoundTimer--;
        PSG_SetVolume(0,g_Sound ? (g_SoundTimer>7 ? 10 : g_SoundTimer/2) : 0);
    }
}
void main(void) {
    VDP_SetMode(VDP_MODE_SCREEN1);
    VDP_SetColor(COLOR_WHITE);
    VDP_EnableDisplay(FALSE);
    VDP_FillVRAM_16K(0,0,0x4000);
    VDP_WriteVRAM_16K(g_Tiles,g_ScreenPatternLow,40);
    Print_SetTextFont(g_Font_MGL_Sample6,32);
    Print_SetColor(COLOR_BLACK,COLOR_WHITE);
    VDP_FillVRAM_16K(0x1F,g_ScreenColorLow,32);
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);
    VDP_LoadSpritePattern(g_SpriteData,0,24);
    VDP_DisableSpritesFrom(SP_END);
    *(volatile u8*)0xF3DB = 0; // CLIKSW: desliga o keyclick
    PSG_SetMixer(PSG_TONE_A_ON | 0x40); // bit 6: manter a porta A do PSG como entrada (joystick)
    PSG_SetVolume(0,0);
    ResetRoom();
    Print_SetPosition(6,5); Print_DrawText("LEVEL DEVIL / MSX1");
    Print_SetPosition(5,7); Print_DrawText("PARECE FACIL. CONFIA.");
    Print_SetPosition(4,10); Print_DrawText("ENTER OU ESPACO: JOGAR");
    Print_SetPosition(4,12); Print_DrawText("SETAS / A D + ESPACO");
    Print_SetPosition(4,14); Print_DrawText("20 SALAS. BOA SORTE.");
    BIOS_SetHookCallback(H_TIMI,TickHook);
    VDP_EnableVBlank(TRUE);
    while(1) {
        WaitFrame();
        Update();
        DrawSprites();
    }
}
