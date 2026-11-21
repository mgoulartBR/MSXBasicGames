#include "render_fronts.h"
#include "scroll_experiments.h"
#include "sprite_color_pass.h"
// Exact opaque interiors use one direct copy; boundaries retain AND/OR.
#include "game.h"
#include "combat.h"
#include "combat_assets.h"
#include "world.h"
#include "fast_stream_data.h"
#include "fast_stream_choices.h"
#include "render_cost.h"
extern volatile u8 hardware_scroll;
extern volatile u16 camera_px,camera_py;
void FastStream(u8 page,DrawObject* ob,u16 texture_y,u8 texture_x,u8 iw,u8 ox,u8 id) __banked
{
 const u8* data=stream_parts[id];
 u8 count=*data++,n,opaque,dx,dy,cw,ch;
 u8 fixed=FIXED_MINES_TEXTURES && active_level==2,source_x=texture_x;
 i16 left=ob->x+ox-camera_px,top=ob->y+4-camera_py;
 i16 l,t,r,b,x,y;u16 base=(u16)page*256;
 if(left>=256 || top>=(hardware_scroll?192:176) || left+iw<=0 || top+(iw==40?16:32)<=0){ob->visible=0;return;}
#if EXP_COST_CHOICE
 // All full-view plans already win; consult the table only at a clip edge.
 if(left<0 || top<0 || left+iw>256 || top+(iw==40?16:32)>(hardware_scroll?192:176)){
 u8 whole[6]={1,0,0,0,0,0};
 whole[3]=iw;whole[4]=iw==40?16:32;
 dx=hardware_scroll?(ob->x+ox)&255:left;
 dy=hardware_scroll?(ob->y+4)&255:top+16;
 if(WholeChoice(render_choices[id],left,top,iw,iw==40?16:32,dx,dy,texture_y,1,hardware_scroll?192:176)){
  data=whole+1;count=1;
 }
 }
#endif
 for(n=0;n<count;++n,data+=5){
  l=data[0];t=data[1];r=data[2];b=data[3];opaque=data[4];
  if(left+l<0)l=-left;if(top+t<0)t=-top;
  if(left+r>256)r=256-left;if(top+b>(hardware_scroll?192:176))b=(hardware_scroll?192:176)-top;
  for(y=t;y<b;y+=ch){
   dy=hardware_scroll?(ob->y+4+y)&255:top+y+16;
   ch=b-y;if((u16)dy+ch>256)ch=256-dy;
   if(!fixed && (u16)(u8)(texture_y+y)+ch>256)ch=256-(u8)(texture_y+y);
   for(x=l;x<r;x+=cw){
    dx=hardware_scroll?(ob->x+ox+x)&255:left+x;
    cw=r-x;if((u16)dx+cw>256)cw=256-dx;
    g_VDP_Command.DX=dx;g_VDP_Command.DY=base+dy;
    g_VDP_Command.NX=cw;g_VDP_Command.NY=ch;g_VDP_Command.ARG=0;
    // One-pass TIMP image (0 = transparent, 9 = the second black): opaque interiors are a direct copy, the rest one TIMP.
    g_VDP_Command.SX=source_x+x;g_VDP_Command.SY=fixed?texture_y+y:base+(u8)(texture_y+y);
    g_VDP_Command.CMD=opaque?(((source_x+x|dx|cw)&1)?VDP_CMD_LMMM+VDP_OP_IMP:VDP_CMD_HMMM):VDP_CMD_LMMM+VDP_OP_TIMP;
    VDP_CommandSetupR32();
   }
  }
 }
}
