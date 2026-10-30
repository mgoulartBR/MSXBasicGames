/*
 * LEVEL DEVIL - MSX1 Edition
 * Fan-made independent adaptation. Original game by Unept.
 * Copyright (c) 2026 8bit/lab. SPDX-License-Identifier: CC-BY-SA-4.0
 * Build: MSXgl, Machine=1, Target=ROM_32K.
 * Reference source: validate with the pinned SDK and test on MSX1.
 */
#include "msxgl.h"
#include "input.h"
#include "psg.h"
#include "font/font_mgl_sample6.h"

struct Room {
    u8 gapX, gapW, crackX, crackW, spikeX, spikeW;
    u8 blockX, blockW, platX, platY, doorX, doorMove;
};
const struct Room g_Rooms[12] = {
    {96,40,0,0,0,0,0,0,0,0,220,0},
    {0,0,88,40,0,0,0,0,0,0,220,0},
    {0,0,0,0,112,32,0,0,0,0,220,0},
    {0,0,0,0,0,0,120,16,0,0,220,0},
    {100,32,0,0,0,0,0,0,0,0,220,1},
    {64,40,0,0,160,24,0,0,0,0,220,0},
    {0,0,0,0,176,24,0,0,120,128,220,0},
    {150,40,72,32,0,0,0,0,112,120,220,0},
    {0,0,0,0,150,32,100,16,0,0,220,0},
    {0,0,80,40,0,0,0,0,0,0,220,1},
    {64,32,0,0,176,24,130,16,0,0,220,0},
    {60,40,116,28,168,24,208,16,0,0,224,1}
};
const u8 g_Tiles[40] = {
    0,0,0,0,0,0,0,0,
    255,255,239,255,255,255,127,255,
    255,239,239,247,255,255,127,255,
    16,16,56,56,124,124,254,254,
    255,195,165,153,153,165,195,255
};
const u8 g_SpriteData[128] = {
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
    131,67,35,19,11,7,255,255
};
volatile u8 g_Tick = 0;
const struct Room* g_R;
u8 g_Level=0, g_Deaths=0, g_State=0, g_Timer=0;
u8 g_PX=22, g_Grounded=1, g_Coyote=0, g_JumpBuffer=0;
i16 g_PY=2240, g_VY=0;
u8 g_Crack=0, g_Spike=0, g_BlockState=0, g_BlockVy=0;
u16 g_BlockPY=512;
u8 g_DoorX=220, g_DoorMoved=0, g_Frame=0;
u8 g_PauseWas=0, g_ResetWas=0, g_MuteWas=0, g_JumpWas=0;
u8 g_Sound=1, g_SoundTimer=0;

void TickHook(void) { g_Tick=1; }
void WaitFrame(void) {
    while(!g_Tick) Halt();
    g_Tick=0;
    g_Frame++;
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
u8 InRange(u8 x, u8 start, u8 width) {
    return width && x>=start && x<(u16)start+width;
}
u8 Solid(u8 x) {
    if(InRange(x,g_R->gapX,g_R->gapW)) return FALSE;
    if(g_Crack>=10 && InRange(x,g_R->crackX,g_R->crackW)) return FALSE;
    return TRUE;
}
void DrawFloor(void) {
    u8 x,y,t;
    for(x=0;x<32;x++) {
        t=Solid(x*8+4) ? 1 : 0;
        for(y=19;y<23;y++) Tile(x,y,t);
        if(g_R->crackW && g_Crack<10 && Solid(x*8+4)) Tile(x,19,2);
        t=(g_Spike>=6 && InRange(x*8+4,g_R->spikeX,g_R->spikeW)) ? 3 : 0;
        Tile(x,18,t);
    }
}
void ClearMessage(void) {
    VDP_FillVRAM_16K(0,g_ScreenLayoutLow+4*32,9*32);
}
void DrawRoom(void) {
    u8 i;
    VDP_EnableDisplay(FALSE);
    VDP_FillVRAM_16K(0,g_ScreenLayoutLow,768);
    DrawFloor();
    if(g_R->platY) {
        for(i=0;i<4;i++) Tile((g_R->platX>>3)+i,g_R->platY>>3,1);
    }
    Print_SetPosition(1,1);
    Print_DrawText("LEVEL DEVIL     SALA ");
    Print_DrawInt(g_Level+1);
    Print_SetPosition(1,23);
    Print_DrawText("R:REINICIAR P:PAUSA M:SOM");
    VDP_EnableDisplay(TRUE);
}
void ResetRoom(void) {
    g_R=&g_Rooms[g_Level];
    g_PX=22; g_PY=140*16; g_VY=0;
    g_Grounded=1; g_Coyote=6; g_JumpBuffer=0;
    g_Crack=0; g_Spike=0; g_BlockState=0; g_BlockVy=0;
    g_BlockPY=32*16; g_DoorX=g_R->doorX; g_DoorMoved=0;
    DrawRoom();
}
void DrawSprites(void) {
    VDP_SetSpriteSM1(0,g_PX,(u8)(g_PY>>4)-1,0,COLOR_BLACK);
    VDP_SetSpriteSM1(1,g_DoorX,127,4,COLOR_DARK_GREEN);
    VDP_SetSpriteSM1(2,g_DoorX,143,8,COLOR_DARK_GREEN);
    if(g_BlockState) {
        VDP_SetSpriteSM1(3,g_R->blockX,(u8)(g_BlockPY>>4)-1,12,COLOR_BLACK);
    } else VDP_DisableSpritesFrom(3);
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
    g_State=1;
}
void Update(void) {
    u8 row8,dir,left,right,jump,pauseNow,resetNow,muteNow;
    u8 nx,landed;
    i16 oldY,newY,landY;
    row8=Keyboard_Read(8);
    dir=Joystick_GetDirection(JOY_PORT_1);
    left=IS_KEY_PRESSED(row8,KEY_LEFT)||IS_KEY_PRESSED(row8,KEY_A)||dir==JOY_INPUT_DIR_LEFT;
    right=IS_KEY_PRESSED(row8,KEY_RIGHT)||IS_KEY_PRESSED(row8,KEY_D)||dir==JOY_INPUT_DIR_RIGHT;
    jump=IS_KEY_PRESSED(row8,KEY_SPACE)||IS_KEY_PRESSED(row8,KEY_UP)||IS_KEY_PRESSED(row8,KEY_W)||dir==JOY_INPUT_DIR_UP||Joystick_IsButtonPressed(JOY_PORT_1,JOY_INPUT_TRIGGER_A)||Joystick_IsButtonPressed(JOY_PORT_1,JOY_INPUT_TRIGGER_B);
    if(jump && !g_JumpWas) g_JumpBuffer=6;
    g_JumpWas=jump;
    pauseNow=Keyboard_IsKeyPressed(KEY_P)||Keyboard_IsKeyPressed(KEY_ESC);
    resetNow=Keyboard_IsKeyPressed(KEY_R);
    muteNow=Keyboard_IsKeyPressed(KEY_M);
    if(muteNow && !g_MuteWas) {
        g_Sound=!g_Sound;
        if(!g_Sound) PSG_SetVolume(0,0);
    }
    g_MuteWas=muteNow;
    if(pauseNow && !g_PauseWas && (g_State==1 || g_State==4)) {
        if(g_State==1) {
            g_State=4;
            Print_SetPosition(5,5); Print_DrawText("PAUSA - P PARA VOLTAR");
        } else { ClearMessage(); g_State=1; }
    }
    g_PauseWas=pauseNow;
    if(resetNow && !g_ResetWas && g_State!=0) { ResetRoom(); g_State=1; }
    g_ResetWas=resetNow;
    if(g_State==0) {
        if(g_JumpBuffer || Keyboard_IsKeyPressed(KEY_RETURN)) StartRoom();
    } else if(g_State==2) {
        if(--g_Timer==0) { ResetRoom(); g_State=1; }
    } else if(g_State==3) {
        if(g_JumpBuffer || Keyboard_IsKeyPressed(KEY_RETURN)) {
            g_Level++;
            if(g_Level==12) { g_Level=0; g_Deaths=0; }
            ResetRoom(); g_State=1;
        }
    } else if(g_State==1) {
        if(g_Coyote) g_Coyote--;
        if(g_Grounded) g_Coyote=6;
        if(g_JumpBuffer && g_Coyote) {
            g_VY=-76; g_Grounded=0; g_Coyote=0; g_JumpBuffer=0; Sound(240);
        }
        if(g_JumpBuffer) g_JumpBuffer--;
        nx=g_PX;
        if(left && nx>2) nx-=2;
        if(right && nx<244) nx+=2;
        if(g_R->platY && g_PY+192>(i16)g_R->platY*16 && g_PY<(i16)(g_R->platY+8)*16) {
            if(g_PX+10<=g_R->platX && nx+10>g_R->platX) nx=g_R->platX-10;
            if(g_PX>=g_R->platX+32 && nx<g_R->platX+32) nx=g_R->platX+32;
        }
        g_PX=nx;
        if(g_R->crackW && (u16)g_PX+10>g_R->crackX && g_PX<(u16)g_R->crackX+g_R->crackW && g_Crack<250) {
            g_Crack++;
            if(g_Crack==10) { DrawFloor(); Sound(650); }
        }
        if(g_R->spikeW && (u16)g_PX+30>g_R->spikeX && g_PX<(u16)g_R->spikeX+g_R->spikeW+10 && g_Spike<250) {
            g_Spike++;
            if(g_Spike==6) { DrawFloor(); Sound(450); }
        }
        if(g_R->blockW && (u16)g_PX+32>g_R->blockX) g_BlockState=1;
        if(g_BlockState && g_BlockPY<136*16) {
            g_BlockVy+=3;
            g_BlockPY+=g_BlockVy;
            if(g_BlockPY>=136*16) { g_BlockPY=136*16; g_BlockVy=0; }
        }
        if(g_R->doorMove && !g_DoorMoved && (u16)g_PX+44>g_R->doorX) {
            g_DoorX-=28; g_DoorMoved=1; Sound(330);
        }
        g_VY+=4;
        if(g_VY>96) g_VY=96;
        oldY=g_PY; newY=oldY+g_VY;
        g_Grounded=0; landed=0; landY=152*16;
        if(g_VY>=0) {
            if(oldY+192<=152*16 && newY+192>=152*16 && (Solid(g_PX+2)||Solid(g_PX+8))) landed=1;
            if(g_R->platY && g_PX+10>g_R->platX && g_PX<g_R->platX+32 && oldY+192<=(i16)g_R->platY*16 && newY+192>=(i16)g_R->platY*16) {
                landed=1; landY=(i16)g_R->platY*16;
            }
            if(landed) { newY=landY-192; g_VY=0; g_Grounded=1; }
        } else if(g_R->platY && g_PX+10>g_R->platX && g_PX<g_R->platX+32 && oldY>=(i16)(g_R->platY+8)*16 && newY<(i16)(g_R->platY+8)*16) {
            newY=(i16)(g_R->platY+8)*16; g_VY=0;
        }
        if(newY<22*16) { newY=22*16; g_VY=0; }
        g_PY=newY;
        if(g_PY>192*16) Die();
        if(g_Spike>=6 && g_PX+8>g_R->spikeX+2 && g_PX+2<(u16)g_R->spikeX+g_R->spikeW-2 && g_PY+192>144*16 && g_PY<152*16) Die();
        if(g_BlockState && g_PX+9>g_R->blockX+1 && g_PX+1<(u16)g_R->blockX+15 && g_PY+192>(g_BlockPY>>4)*16+16 && g_PY<((g_BlockPY>>4)+15)*16) Die();
        if(g_State==1 && g_PX+10>g_DoorX+3 && g_PX<g_DoorX+13 && g_PY+192>132*16 && g_PY<152*16) {
            g_State=3; Sound(120);
            Print_SetPosition(7,5); Print_DrawText("SALA CONCLUIDA!");
            Print_SetPosition(5,7); Print_DrawText("ENTER: PROXIMA SALA");
            if(g_Level==11) {
                Print_SetPosition(5,9); Print_DrawText("12 SALAS. VOCE VENCEU!");
            }
        }
    }
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
    VDP_LoadSpritePattern(g_SpriteData,0,16);
    VDP_DisableSpritesFrom(4);
    *(volatile u8*)0xF3DB = 0; // CLIKSW: desliga o keyclick
    PSG_SetMixer(PSG_TONE_A_ON);
    PSG_SetVolume(0,0);
    ResetRoom();
    Print_SetPosition(6,5); Print_DrawText("LEVEL DEVIL / MSX1");
    Print_SetPosition(5,7); Print_DrawText("PARECE FACIL. CONFIA.");
    Print_SetPosition(4,10); Print_DrawText("ENTER OU ESPACO: JOGAR");
    Print_SetPosition(4,12); Print_DrawText("SETAS / A D + ESPACO");
    BIOS_SetHookCallback(H_TIMI,TickHook);
    VDP_EnableVBlank(TRUE);
    while(1) {
        WaitFrame();
        Update();
        DrawSprites();
    }
}