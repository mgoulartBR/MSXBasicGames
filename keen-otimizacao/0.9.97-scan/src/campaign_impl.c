#include "boobus.h"
#include "swamp.h"
#include "grove.h"
#include "apple.h"
#include "city.h"
#include "rapids.h"
// SPDX-License-Identifier: GPL-2.0-or-later
#include "campaign.h"
#include "game.h"
#include "world.h"
typedef struct {u16 x,y;u8 kind;} Bonus;
#include "bonus_data.h"
#include "scanwin.inc"
u16 boobus_bombs;
u32 game_score,next_extra,bonus_taken;
i8 game_lives;
u8 restart_pending;
static u8 bonus_near[8],bonus_near_count,bonus_tx,bonus_ty;
void BonusReset(void) __banked {bonus_taken=0;bonus_high=0;bonus_tx=bonus_ty=255;bonus_near_count=0;}
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
extern const u8 g_Font_MGL_Sample6[];
#ifndef KEEN_INITIAL_BOMBS
#define KEEN_INITIAL_BOMBS 0
#endif
void CampaignNew(void) __banked
{
 boobus_bombs=KEEN_INITIAL_BOMBS;game_score=bonus_taken=0;next_extra=20000;game_lives=3;restart_pending=0;flower_ammo=0;
}
void GivePoints(u16 points) __banked
{
 SoundPlay(SFX_PICKUP);game_score+=points;
 if(game_score>=next_extra){if(game_lives<99)++game_lives;SoundPlay(SFX_LIFE);next_extra*=2;}
 combat_dirty=1;
}
void BonusTick(void) __banked
{
 u8 i,k,tx=player_x>>8,ty=player_y>>8;const Bonus* b;
 static const u8 widths[8]={16,16,16,16,16,16,8,24};
 static const u8 heights[8]={16,15,16,16,23,16,15,16};
 static const u16 values[6]={100,200,500,1000,2000,5000};
 if(tx!=bonus_tx || ty!=bonus_ty){
  bonus_tx=tx;bonus_ty=ty;
  // Width <=24: a bonus two tile columns left cannot touch player_x+4. Z80 window scan (ascending indices) of the
  // bonuses with -1 <= (x>>4)-tx <= 2 and -2 <= (y>>4)-ty <= 3.
  {u8 tmp[16];ScanArgs sa;sa.items=(const u8*)bonus_data[LEVEL_INDEX];sa.out=tmp;sa.ox=(u16)((tx-1)*16);sa.oy=(u16)((ty-2)*16);
   sa.w=64;sa.h=96;sa.stride=sizeof(Bonus);sa.n=bonus_count[LEVEL_INDEX];sa.count=0;
   i=ScanWin(&sa);if(i>8)i=8;bonus_near_count=i;for(k=0;k<i;++k)bonus_near[k]=tmp[k];}
 }
 for(k=0;k<bonus_near_count;++k){
  i=bonus_near[k];b=bonus_data[LEVEL_INDEX]+i;
  if(i<32?(((const u8*)&bonus_taken)[i>>3]&(1<<(i&7))):(bonus_high&(1<<(i-32))))continue;   // byte tests: no 32-bit shift
  // Reject far objects before converting coordinates or doing vertical tests.
  if((i16)((player_x>>4)+4-b->x)>=widths[b->kind] || (i16)(b->x-(player_x>>4)-4)>=16)continue;
  if((i16)((player_y>>4)+3-b->y)>=heights[b->kind] || (i16)(b->y-(player_y>>4)-3)>=28)continue;
  if(i<32)((u8*)&bonus_taken)[i>>3]|=(u8)(1<<(i&7));else bonus_high|=(u8)(1<<(i-32));combat_dirty=1;
  if(b->kind<6)GivePoints(values[b->kind]);
  else if(b->kind==6){if(game_lives<99)++game_lives;}
  else {game_lives=game_lives>96?99:game_lives+3;flower_ammo=flower_ammo>65527U?65535U:flower_ammo+8;GivePoints(10000);}
 }
}
u8 BonusCollect(DrawObject* dest,u8 capacity) __banked
{
 // Z80 window scan finds the on-screen candidates; the taken bitmap is
 // consulted only for those. -32 <= x-cam < 256 and -32 <= y-cam < 192.
 u8 idx[36],i,j,k,n=0,count=bonus_count[LEVEL_INDEX];
 ScanArgs a;const Bonus* b;
 a.items=(const u8*)bonus_data[LEVEL_INDEX];a.out=idx;a.ox=camera_px-32;a.oy=camera_py-32;
 a.w=288;a.h=224;a.stride=sizeof(Bonus);a.n=count;a.count=0;
 k=ScanWin(&a);
 for(j=0;j<k && n<capacity;++j){
  i=idx[j];b=bonus_data[LEVEL_INDEX]+i;
  if(((i<32?((const u8*)&bonus_taken)[i>>3]:bonus_high)>>(i&7))&1)continue;
  dest->x=b->x;dest->y=b->y;dest->frame=94+b->kind;dest->visible=1;++dest;++n;
 }
 return n;
}
void DeathFinish(void) __banked
{
 if(active_level==4)RapidLoseBombs();
 if(active_level==6)CityLoseBombs();
 if(active_level==7)AppleLoseBombs();
 if(active_level==8)GroveLoseBombs();
 if(active_level==10)SwampLoseBombs();
 if(active_level==11)BoobusRestoreBombs();
 --game_lives;restart_pending=game_lives<0?2:1;
}
void GameOver(void) __banked
{
 VDP_CommandWait();VDP_SetVerticalOffset(0);
 if(hardware_scroll){VDP_RegWrite(26,0);VDP_RegWrite(27,0);VDP_RegWrite(25,0);}
 VDP_SetPage(0);VDP_CommandHMMV(0,0,256,512,0);VDP_CommandWait();
 Print_SetBitmapFont(g_Font_MGL_Sample6);Print_SetColor(15,0);
 Print_SetPosition(96,64);Print_DrawText("GAME OVER");
 Print_SetPosition(48,104);Print_DrawText("SPACE: NEW GAME");VDP_CommandWait();
 while(Keyboard_IsKeyPressed(KEY_SPACE))Halt();
 while(!Keyboard_IsKeyPressed(KEY_SPACE))Halt();
 while(Keyboard_IsKeyPressed(KEY_SPACE))Halt();
 if(hardware_scroll)VDP_RegWrite(25,2);
}

#include "finale_text.inc"
extern u8 ReadInput(void) __banked;
// Require a fresh SPACE/joystick trigger edge for every page. A held jump
// from combat cannot skip the finale, and input is released before restart.
void FinaleAck(void)
{
 u8 delay=60;
 while(delay--)Halt();
 while(ReadInput()&80)Halt();
 while(!(ReadInput()&80))Halt();
 while(ReadInput()&80)Halt();
}
static void FinaleScore(void)
{
 char digits[11];u8 i=10;u32 value=game_score;
 digits[i]=0;
 do {digits[--i]='0'+(u8)(value%10);value/=10;}while(value);
 Print_DrawText(digits+i);
}
static void FinaleText(const char* text)
{
 u8 y=28;Print_SetPosition(12,y);
 while(*text){
  if(*text=='\n'){y+=8;Print_SetPosition(12,y);}
  else Print_DrawChar(*text);
  ++text;
 }
}
static void FinalePage(const char* text,u8 page)
{
 VDP_CommandHMMV(0,0,256,212,0);VDP_CommandWait();
 Print_SetPosition(16,8);Print_DrawText("KEEN DREAMS - THE END");
 FinaleText(text);
 if(page==2){Print_SetPosition(12,160);Print_DrawText("SCORE: ");FinaleScore();}
 Print_SetPosition(12,180);Print_DrawText(page==2?"SPACE / FIRE: NEW GAME":"SPACE / FIRE: CONTINUE");
 VDP_CommandWait();FinaleAck();
}
void GameFinale(void) __banked
{
 VDP_CommandWait();VDP_EnableSprite(0);VDP_SetVerticalOffset(0);
 if(hardware_scroll){VDP_RegWrite(26,0);VDP_RegWrite(27,0);VDP_RegWrite(25,0);}
 VDP_SetPage(0);VDP_CommandHMMV(0,0,256,512,0);VDP_CommandWait();
 Print_SetBitmapFont(g_Font_MGL_Sample6);Print_SetColor(15,0);
 FinalePage(finale_text_0,0);FinalePage(finale_text_1,1);FinalePage(finale_text_2,2);
 if(hardware_scroll)VDP_RegWrite(25,2);
}
