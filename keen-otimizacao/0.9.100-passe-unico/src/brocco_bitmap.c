// SPDX-License-Identifier: GPL-2.0-or-later
// Original Brocco masks/colors streamed from PackBits data.
#include "pass.h"
#include "combat_assets.h"
#include "hw_tomato.h"
#include "world.h"
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);
void BroccoDecode(u8 frame) __banked;
extern volatile u8 hardware_scroll;
extern volatile u16 camera_px,camera_py;
void BroccoBitmap(u8 page,DrawObject* ob) __banked
{
 const SpriteDef* sp=sprites[LEVEL_INDEX]+ob->frame;

 u8 row,texture_y,w,h,iw=sp->w,ih=sp->h,view_h=hardware_scroll?192:176;
 u16 x,y,dx,dy;i16 left=ob->x+SPR_OX(sp)-camera_px,top=ob->y+SPR_OY(sp)-camera_py,l=0,t=0,r=iw,b=ih;
 if(left>=256 || top>=view_h || left+iw<=0 || top+ih<=0){ob->visible=0;return;}
 if(left<0)l=-left;if(top<0)t=-top;if(left+r>256)r=256-left;if(top+b>view_h)b=view_h-top;
 dx=HWTexture(page,ob->frame,ih);texture_y=dx;VDP_CommandWait();
 if(!(dx&256)){
  BroccoDecode(ob->frame);
  for(row=0;row<ih;++row)VPF(world_map+(active_level==8?8640:active_level>=9?3600:4000)+(u16)row*(iw>>1),(u16)page*32768+(u16)(u8)(texture_y+row)*128+64,0,iw>>1);
 }
 for(y=t;y<b;y+=h){
  dy=hardware_scroll?(ob->y+SPR_OY(sp)+y)&255:16+top+y;h=b-y;if(hardware_scroll && dy+h>256)h=256-dy;if(((texture_y+y)&255)+h>256)h=256-((texture_y+y)&255);if(((texture_y+y)&255)+h>256)h=256-((texture_y+y)&255);
  for(x=l;x<r;x+=w){dx=hardware_scroll?(ob->x+SPR_OX(sp)+x)&255:left+x;w=r-x;if(hardware_scroll && dx+w>256)w=256-dx;
   g_VDP_Command.SX=128+x;g_VDP_Command.SY=(u16)page*256+((texture_y+y)&255);
   g_VDP_Command.DX=dx;g_VDP_Command.DY=(u16)page*256+dy;g_VDP_Command.NX=w;g_VDP_Command.NY=h;g_VDP_Command.ARG=0;
   g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_TIMP;VDP_CommandSetupR32();}
 }
}
