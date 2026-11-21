#include "render_fronts.h"
// SPDX-License-Identifier: GPL-2.0-or-later
// Full Asparagusto walking frames. Cached in hidden rows, shared with hero bitmap.
#include "mines.h"
#include "hw_tomato.h"
#include "tater_images.h"
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);
void FastStream(u8 page,DrawObject* ob,u16 texture_y,u8 texture_x,u8 iw,u8 ox,u8 id) __banked;
extern volatile u8 hardware_scroll;
extern volatile u16 camera_px,camera_py;
void MinesTaterBitmap(u8 page,DrawObject* ob) __banked
{
 u8 row,w,h,part,texture_x=128,iw=64,ih=32,view_h=hardware_scroll?192:176;
 u16 x,y,dx,dy,texture_y; i16 left=ob->x+8-camera_px,top=ob->y+4-camera_py,l=0,t=0,r=64,b=32;
 const u8* pixels=tater_images[ob->frame<130?ob->frame-128:ob->frame-132];
 if(left>=256 || top>=view_h || left+iw<=0 || top+ih<=0){ob->visible=0;return;}
 if(left<0)l=-left;if(top<0)t=-top;if(left+r>256)r=256-left;if(top+b>view_h)b=view_h-top;
 dx=HWTexture(page,ob->frame,ih);texture_y=dx&255;
#if FIXED_MINES_TEXTURES
 if(active_level==2){texture_x=(dx&255)?144:16;texture_y=565;}
#endif
 VDP_CommandWait();
 if(!(dx&256)){
  for(row=0;row<ih;++row)VPF(pixels+(u16)row*(iw>>1),(
#if FIXED_MINES_TEXTURES
   active_level==2?((texture_y-512+row)*128+(texture_x>>1)):
#endif
   (u16)page*32768+(u16)(u8)(texture_y+row)*128+64),FIXED_MINES_TEXTURES && active_level==2?1:0,iw>>1);
 }
 FastStream(page,ob,texture_y,texture_x,iw,8,12+(ob->frame<130?ob->frame-128:ob->frame-132));
}
