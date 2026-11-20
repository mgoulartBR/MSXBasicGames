#include "boobus.h"
#include "combat_assets.h"
#include "hw_tomato.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
// Large original 104-line sprites are streamed in 32-line strips through the
// atlas's reserved y96..127. Both display pages and scrolling wraps are preserved.
void BoobusBitmap(u8 page,DrawObject* ob) __banked
{
 const SpriteDef* sp=sprites[10]+ob->frame;u8 first,rows,w,h,view=hardware_scroll?192:176;
 u16 x,y,dx,dy;i16 left=ob->x+SPR_OX(sp)-camera_px,top=ob->y+SPR_OY(sp)-camera_py,l=0,t=0,r=sp->w,b=sp->h;
 if(left<0)l=-left;if(top<0)t=-top;if(left+r>256)r=256-left;if(top+b>view)b=view-top;
 if(l>=r || t>=b){ob->visible=0;return;}
 // Cache walking frames in the hidden portion of each scrolling page.
 // Wide jumping frames retain the protected atlas strip path below.
 if(hardware_scroll && sp->w<=64 && sp->h<=104){
  u8 ty=(camera_py+192)&255,half;
  if(hw->bitmap_cached[page]!=ob->frame || hw->bitmap_y[page]!=ty){
   BoobusTexture(ob->frame,page,ty,sp->w>>1,sp->h);
   hw->bitmap_cached[page]=ob->frame;hw->bitmap_y[page]=ty;hw->bitmap_second[page]=255;hw->bitmap_tall|=1<<page;
  }
  for(y=t;y<b;y+=h){
   half=y/52;dy=(ob->y+SPR_OY(sp)+y)&255;
   h=b-y;if(h>52-y%52)h=52-y%52;
   if(dy+h>256)h=256-dy;if(((ty+y%52)&255)+h>256)h=256-((ty+y%52)&255);
   for(x=l;x<r;x+=w){
    dx=(ob->x+SPR_OX(sp)+x)&255;w=r-x;if(dx+w>256)w=256-dx;
    g_VDP_Command.SX=(half?128:0)+x;
    g_VDP_Command.SY=(u16)page*256+((ty+y%52)&255);
    g_VDP_Command.DX=dx;g_VDP_Command.DY=(u16)page*256+dy;
    g_VDP_Command.NX=w;g_VDP_Command.NY=h;g_VDP_Command.ARG=0;
    g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_TIMP;VDP_CommandSetupR32();
   }
  }
  return;
 }
 hw->bitmap_cached[page]=hw->bitmap_second[page]=255;

 for(first=t;first<b;first+=rows){rows=b-first;if(rows>BOOBUS_STREAM_ROWS)rows=BOOBUS_STREAM_ROWS;BoobusRows(ob->frame,first,rows,sp->w>>1);
  for(y=first;y<first+rows;y+=h){dy=hardware_scroll?(ob->y+SPR_OY(sp)+y)&255:16+top+y;h=first+rows-y;if(hardware_scroll && dy+h>256)h=256-dy;
   for(x=l;x<r;x+=w){dx=hardware_scroll?(ob->x+SPR_OX(sp)+x)&255:left+x;w=r-x;if(hardware_scroll && dx+w>256)w=256-dx;
    g_VDP_Command.SX=x;g_VDP_Command.SY=512+BOOBUS_STREAM_ROW+y-first;g_VDP_Command.DX=dx;g_VDP_Command.DY=(u16)page*256+dy;
    g_VDP_Command.NX=w;g_VDP_Command.NY=h;g_VDP_Command.ARG=0;g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_TIMP;VDP_CommandSetupR32();
   }
  }
 }
}
