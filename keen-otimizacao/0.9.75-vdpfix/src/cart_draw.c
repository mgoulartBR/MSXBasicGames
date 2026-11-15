// SPDX-License-Identifier: GPL-2.0-or-later
// Stream original actors in <=24-row bands outside the hidden viewport.
#include "cart.h"
#include "keydoor.h"
#include "cart_images.h"
#include "object_197.h"
#include "object_198.h"
#include "object_211.h"
#include "object_210.h"
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);
void CartDraw(u8 page,DrawObject* ob) __banked
{
    u8 row,texture_y,w,h,part,band,rows,band_end,iw=48,ih=24,view_h=hardware_scroll?192:176;
    u16 x,y,dx,dy;
    i16 left=ob->x-camera_px,top=ob->y-camera_py,l=0,t=0,r,b;
    const u8* pixels;
    if(ob->frame<KEY_FRAME)pixels=cart_images[ob->frame-CART_FRAME];
    else if(ob->frame==KEY_FRAME || ob->frame==KEY_FRAME+1){iw=16;ih=8;pixels=ob->frame==KEY_FRAME?object_197:object_198;}
    else if(ob->frame==DOOR_FRAME){iw=16;ih=64;pixels=object_211;}
    else {iw=8;ih=16;pixels=object_210;}
    r=iw;b=ih;
    if(left>=256 || top>=view_h || left+iw<=0 || top+ih<=0){ob->visible=0;return;}
    if(left<0)l=-left;if(top<0)t=-top;if(left+r>256)r=256-left;if(top+b>view_h)b=view_h-top;
    // V9958 leaves 64 circular rows unused; MSX2 leaves rows 212..255 unused.
    texture_y=hardware_scroll?(camera_py+192)&255:212;
    if(texture_y>232)texture_y=0;
    for(band=t;band<b;band+=rows){
        rows=b-band;if(rows>24)rows=24;band_end=band+rows;
        VDP_CommandWait();
        for(row=0;row<rows;++row)VPF(pixels+(u16)(band+row)*iw,(u16)page*32768+(u16)(texture_y+row)*128,0,iw);
        for(part=0;part<2;++part){
            for(y=band;y<band_end;y+=h){
                dy=hardware_scroll?(ob->y+y)&255:16+top+y;
                h=band_end-y;if(hardware_scroll && dy+h>256)h=256-dy;
                for(x=l;x<r;x+=w){
                    dx=hardware_scroll?(ob->x+x)&255:left+x;
                    w=r-x;if(hardware_scroll && dx+w>256)w=256-dx;
                    g_VDP_Command.SX=(part?iw:0)+x;g_VDP_Command.SY=(u16)page*256+texture_y+y-band;
                    g_VDP_Command.DX=dx;g_VDP_Command.DY=(u16)page*256+dy;
                    g_VDP_Command.NX=w;g_VDP_Command.NY=h;g_VDP_Command.ARG=0;
                    g_VDP_Command.CMD=VDP_CMD_LMMM+(part?VDP_OP_OR:VDP_OP_AND);VDP_CommandSetupR32();
                }
            }
        }
    }
}
