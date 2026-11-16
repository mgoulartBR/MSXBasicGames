#include "render_fronts.h"
#include "scroll_experiments.h"
// SPDX-License-Identifier: GPL-2.0-or-later
// Hybrid renderer: Keen in hardware, enemies in bitmap. Bitmap fallback
// preserves foreground occlusion and clipping at the software viewport edge.
#include "game.h"
#include "combat.h"
#include "combat_assets.h"
#include "hw_tomato.h"
#include "hw_hero_resident.h"
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);
extern volatile u8 hardware_scroll;
extern volatile u8 flip_flag,flip_r5,flip_r6,flip_r8,flip_r11;
extern volatile u16 camera_px,camera_py;
void HeroData0(u8 f,u16 base) __banked;
void HeroData1(u8 f,u16 base) __banked;
void HeroData2(u8 f,u16 base) __banked;
void HeroData3(u8 f,u16 base) __banked;
void HeroData4(u8 f,u16 base) __banked;
u8* hw_attributes;
u8 fixed_mines_tags[2];
void BarOff(void) __banked;
void HWHide(void) __banked
{
 while(flip_flag)Halt();
 BarOff();
 fixed_mines_tags[0]=fixed_mines_tags[1]=255;
 hw_tomato[0]=hw_tomato[1]=hw_tomato_cached[0]=hw_tomato_cached[1]=255;
 hw_tomato_second[0]=hw_tomato_second[1]=hw_tomato_second_cached[0]=hw_tomato_second_cached[1]=255;
 VDP_EnableSprite(FALSE);hw->visible[0]=hw->visible[1]=0;
 hw->cached[0]=hw->cached[1]=255;hw->bitmap_cached[0]=hw->bitmap_cached[1]=255;hw->check_valid=0;hw->bitmap_second[0]=hw->bitmap_second[1]=255;hw->bitmap_tall=hw->bitmap_recent=0;hw->scene_map_dirty=3;
}
void HWControl(void) __banked {}
void HWPrepare(u8 page) __banked
{
 i16 sx,sy;u16 y,px,py;u8 f,left,right,top,bottom,tx,ty;
 const SpriteDef* sp;const SpriteDef* fg;u8 id;u16 hx,hy,fx,fy;
#if !EXP_TEXTURE_SELECT
 // Invalidate at every page viewport change, including views containing
 // no streamed enemy. Such views may paint over the previous hidden texture.
 f=hardware_scroll?(camera_py+192)&255:212;
 if(hw->bitmap_y[page]!=f){
  if(!CacheScroll(page,f))hw->bitmap_cached[page]=hw->bitmap_second[page]=255;
  hw->bitmap_y[page]=f;hw->bitmap_tall&=~(1<<page);
 }
#endif
 hw->visible[page]=0;
 if(!hero_visible)return;
 f=hero_frame;
 if(f<16){}else if(f==30 || f==31)f-=14;else if(f>=36 && f<46)f-=18;else return;
 px=player_x>>4;py=player_y>>4;
 sx=px-camera_px;sy=py-camera_py;
 if(sx<0 || sx>224 || sy<0 || sy>(hardware_scroll?160:144))return;
 // Cache static foreground queries at an unchanged position/frame. A
 // transparent margin of a foreground tile does not require bitmap drawing.
 if(!hw->check_valid || hw->check_x!=px || hw->check_y!=py || hw->check_frame!=hero_frame){
  hw->check_x=px;hw->check_y=py;hw->check_frame=hero_frame;hw->check_valid=1;hw->check_result=0;
  sp=&sprites[LEVEL_INDEX][hero_frame];hx=px+SPR_OX(sp);hy=py+SPR_OY(sp);
  left=hx>>4;right=(hx+sp->w-1)>>4;top=hy>>4;bottom=(hy+sp->h-1)>>4;
  for(ty=top;ty<=bottom && ty<MAP_HEIGHT;++ty)
   for(tx=left;tx<=right && tx<MAP_WIDTH;++tx){
    id=world_collision[524+MapTile((u16)ty*MAP_WIDTH+tx)];
    if(!id)continue;
    fg=&sprites[LEVEL_INDEX][id];fx=(u16)tx*16+SPR_OX(fg);fy=(u16)ty*16+SPR_OY(fg);
    if(hx<fx+fg->w && fx<hx+sp->w && hy<fy+fg->h && fy<hy+sp->h)hw->check_result=1;
   }
 }
 if(hw->check_result)return;
 y=hardware_scroll?py:(sy+16);
 hw->frame=f*2+(y&1);hw->x=sx;hw->y=(u8)(y&~1U);
 hw->visible[page]=1;
 // Hardware hero patterns/attributes live in atlas VRAM, not the offscreen
 // bitmap texture. Keep the enemy cache until its frame or texture Y changes.
}
u8 vdp_fast,bar_ok;
// Counts a 35 T loop over one BIOS jiffy (measured n*hz: 3.58 MHz ~81k, 5.37 MHz ~135k, 7.16 MHz ~181k;
// ISR time included). Above ~5.6 MHz the VDP data port
// needs paced writes (see vram_paced.inc).
static u16 ClockLoop(void) __sdcccall(1) __naked
{
 __asm
  ld hl,#0
  ld a,(0xFC9E)
  ld b,a
cl_sync:
  ld a,(0xFC9E)
  cp b
  jr z,cl_sync
  ld b,a
cl_count:
  inc hl
  ld a,(0xFC9E)
  cp b
  jr z,cl_count
  ex de,hl
  ret
 __endasm;
}
void DetectClock(u8 hz) __banked
{
 u16 n=ClockLoop();
 vdp_fast=((u32)n*hz>160000UL)?1:0;
 bar_ok=((u32)n*hz>110000UL)?1:0;   // fixed HUD bar needs >= ~5 MHz (line-exact polling)
}
void HWUpload(u8 page) __banked
{
 u8 i,f=hw->frame,count=(f&1)?24:16;
 // High bits of bitmap_recent track sprite tables independently of the
 // bitmap page: bit 7 displayed table, bit 6 table prepared for the flip.
 // Low bits retain the two bitmap-texture replacement hints.
 u8 table=(hw->bitmap_recent&128)?0:1;
 u16 base=table?0x2800:0x2000;
 hw->bitmap_recent=(hw->bitmap_recent&~64)|(table?64:0);
 if(active_level==2){HWMinesUpload(page);return;}
 if(!hw->visible[page])return;
 if(hw->cached[table]!=f){
  VDP_CommandWait();
  if(f<12)HeroData0(f,base);
  else if(f<24)HeroData1(f-12,base);
  else if(f<36)HeroData2(f-24,base);
  else if(f<48)HeroData3(f-36,base);
  else HeroData4(f-48,base);
  hw->cached[table]=f;
 }
 {
  // Rows of 8 sprites (two 16-px columns of 4): unrolled, pointer-walked, no per-entry shifts.
  u8 y=hw->y-1,x0=hw->x,x1=hw->x+16,*p=hw_attributes;
  const u8* idx=(f<32 && !(f&1))?hero_indices[f/2]:0;
  u8 r;
  for(r=0;r<count;r+=8,y+=16){
#define HWENT(k,xv) *p++=y;*p++=xv;*p++=idx?idx[r+k]:(u8)((r+k)<<2);*p++=0;
   HWENT(0,x0) HWENT(1,x0) HWENT(2,x0) HWENT(3,x0)
   HWENT(4,x1) HWENT(5,x1) HWENT(6,x1) HWENT(7,x1)
#undef HWENT
  }
 }
 hw_attributes[count*4]=216;
 VDP_CommandWait();
 VPF(hw_attributes,base+0x600,1,count*4+1);
}
// Prepares the sprite half of the vblank flip (R#5/R#6/R#11/R#8); the ISR writes it.
void HWFlip(u8 page) __banked
{
 u8 table=(hw->bitmap_recent>>6)&1,visible=hw->visible[page] || (active_level==2 && hw_tomato[page]!=255);
 hw->bitmap_recent=(hw->bitmap_recent&~128)|(table?128:0);
 if(visible)VDP_RegWriteBakMask(1,(u8)~(R01_ST|R01_MAG),R01_ST);
 flip_r6=active_level==2?32+table:(hw->frame<32 && !(hw->frame&1))?32+hero_group[hw->frame/2]:(table?37:36);
 flip_r11=2;
 flip_r5=active_level==2?(table?55:39):(table?95:79);
 flip_r8=visible?0x28:0x2A;   // R#8: TP (color 0 opaque) + VR (64K/128K VRAM) [+ sprite disable]
}
void HeroBitmap0(u8 page,DrawObject* ob,u8 frame) __banked;
void HeroBitmap1(u8 page,DrawObject* ob,u8 frame) __banked;
void HeroBitmap(u8 page,DrawObject* ob) __banked
{
 u8 f=ob->frame;
 if(f>=36)f-=18;else if(f>=30)f-=14;
 if(f<14)HeroBitmap0(page,ob,f);else HeroBitmap1(page,ob,f-14);
}


// Two independent 32-row textures per scrolling page. Tall images own both.
// Tags are stage-local and HWHide invalidates them on every map/title change.
u16 HWTexture(u8 page,u8 frame,u8 height) __banked
{
 u8 ty=hardware_scroll?(camera_py+192)&255:212,bit=1<<page,slot=0,hit=0;
#if FIXED_MINES_TEXTURES
 // Shared fixed slots: wide Tater/mouth at (16,565), Aspar at (144,565).
 if(active_level==2 && height<=32 && ((frame>=108 && frame<116) || (frame>=136 && frame<140) || frame==128 || frame==129 || frame==134 || frame==135)){
  slot=frame>=108 && frame<116;
  if(slot){hit=fixed_mines_tags[1]==frame;fixed_mines_tags[1]=frame;}
  else{hit=fixed_mines_tags[0]==frame;fixed_mines_tags[0]=frame;}
  return (u16)slot|((u16)hit<<8);
 }
#endif


 if(hw->bitmap_y[page]!=ty){hw->bitmap_cached[page]=hw->bitmap_second[page]=255;hw->bitmap_tall&=~bit;hw->bitmap_y[page]=ty;}
 if(!hardware_scroll || height>32){
  hit=hw->bitmap_cached[page]==frame;
  hw->bitmap_cached[page]=frame;hw->bitmap_second[page]=255;hw->bitmap_tall|=bit;
  return (u16)ty|((u16)hit<<8);
 }
 if(hw->bitmap_tall&bit){hw->bitmap_cached[page]=hw->bitmap_second[page]=255;hw->bitmap_tall&=~bit;}
 if(hw->bitmap_cached[page]==frame)hit=1;
 else if(hw->bitmap_second[page]==frame){slot=1;hit=1;}
 else if(hw->bitmap_cached[page]==255)hw->bitmap_cached[page]=frame;
 else if(hw->bitmap_second[page]==255){slot=1;hw->bitmap_second[page]=frame;}
 else if(hw->bitmap_recent&bit)hw->bitmap_cached[page]=frame;
 else {slot=1;hw->bitmap_second[page]=frame;}
 if(slot)hw->bitmap_recent|=bit;else hw->bitmap_recent&=~bit;
 return (u16)(u8)(ty+slot*32)|((u16)hit<<8);
}
