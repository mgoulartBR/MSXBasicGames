#include "bay.c"
#include "bay_static.c"
#include "msxgl.h"
#include "tuberia_extra.h"
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);
extern u16 wcy;
extern volatile u8 hardware_scroll;
u16 WorldExtra(u16 pattern) __banked
{
 u8 row,y=hardware_scroll?(wcy+176)&255:212;
 const u8* p=tuber_extra[pattern-512];if(y>240)y=0;
 VDP_CommandWait();for(row=0;row<16;++row)VPF(p+row*8,(u16)(y+row)*128,0,8);
 return y;
}


#include "brocco_images.h"
#include "combat_assets.h"
void BroccoDecode(u8 frame) __banked{
 const u8* p=brocco_data[frame-136];u8 count,value,tag;
 const SpriteDef* sp=sprites[LEVEL_INDEX]+frame;
 u8* out=world_map+(active_level==8?8640:active_level>=9?3600:4000);
 u8* end=out+(u16)(sp->w>>1)*sp->h;
 while(out<end){
  tag=*p++;count=(tag&127)+1;
  if(tag&128){do{*out++=*p++;}while(--count);}
  else{value=*p++;do{*out++=value;}while(--count);}
 }
}
