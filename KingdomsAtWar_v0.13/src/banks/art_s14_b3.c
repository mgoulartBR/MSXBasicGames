#include "../shared.h"
/* Fog is part of background restoration, never a full-screen redraw overlay. */
void fog_cover(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy) __banked {
 i16 xx,yy,right=x+w,bottom=y+h;
 if(x<(i16)cx)x=cx;if(y<(i16)cy)y=cy;if(right>(i16)cx+256)right=cx+256;if(bottom>(i16)cy+160)bottom=cy+160;
 for(yy=y;yy<bottom;) {u8 hh=16-(yy&15);if(hh>bottom-yy)hh=bottom-yy;
  for(xx=x;xx<right;) {u8 ww=16-(xx&15),state=fog_at(xx>>4,yy>>4);u16 sx,sy;
   if(ww>right-xx)ww=right-xx;
   if(state<2) {
#ifdef KAW_V9990
    /* Equal-state neighbours share one copy from a 256-pixel-wide strip. */
    i16 x0=xx;u16 run=ww;xx+=ww;
    while(xx<right && run<240 && fog_at(xx>>4,yy>>4)==state) {u8 w2=right-xx>16?16:right-xx;run+=w2;xx+=w2;}
    sx=state?256:0;sy=state?1248:1264;
    clip_copy(x0,yy,sx+(x0&15),sy+(yy&15),run,hh,p,cx,cy);
    continue;
#else
    sx=state?64:80;sy=656;
    clip_copy(xx,yy,sx+(xx&15),sy+(yy&15),ww,hh,p,cx,cy);
#endif
   }
   xx+=ww;
  }yy+=hh;
 }
}
u8 fog_page_pending(u8 p) __banked {
 u8 x,y;
 if(fog_checked_epoch[p]==fog_epoch && fog_checked_x[p]==cam_x && fog_checked_y[p]==cam_y)return fog_pending[p];
 fog_checked_epoch[p]=fog_epoch;fog_checked_x[p]=cam_x;fog_checked_y[p]=cam_y;fog_pending[p]=0;
 for(y=cam_y>>4;y<=(cam_y+159)>>4;++y)for(x=cam_x>>4;x<=(cam_x+255)>>4;++x) {
  u16 c=((u16)y<<5)+x;if(fog_changed[p][c>>3]&(1<<(c&7))){fog_pending[p]=1;return 1;}
 }
 return 0;
}
/* Scan byte masks, skipping empty spans and combining adjacent dirty cells.
 * Unchanged cells and damage outside this camera remain untouched. */
u16 fog_damage(u8 p,u16 mask) __banked {
 u8 y,left=cam_x>>4,right=(cam_x+255)>>4,top=cam_y>>4,bottom=(cam_y+159)>>4;
 for(y=top;y<=bottom;++y) {
  u8* row=fog_changed[p]+((u16)y<<2);u8 x=left,v=row[x>>3]>>(x&7),start=NONE;
  while(x<=right) {
   if(!(x&7))v=row[x>>3];
   if(v&1) {if(start==NONE)start=x;}
   else if(start!=NONE) {i16 l=(u16)start<<4,r=(u16)x<<4,t=(u16)y<<4;u8 j;
    for(j=0;j<RN;++j)if(!(mask&render_bit[j])) {Rect* a=rect_now+j;if(a->w && a->h && a->x<r && a->x+a->w>l && a->y<t+16 && a->y+a->h>t)mask|=render_bit[j];}
    start=NONE;}
   if(!v)x=(x|7)+1;else {++x;v>>=1;}
  }
  if(start!=NONE) {x=right+1;i16 l=(u16)start<<4,r=(u16)x<<4,t=(u16)y<<4;u8 j;
    for(j=0;j<RN;++j)if(!(mask&render_bit[j])) {Rect* a=rect_now+j;if(a->w && a->h && a->x<r && a->x+a->w>l && a->y<t+16 && a->y+a->h>t)mask|=render_bit[j];}
    }
  
 }
 return mask;
}
void fog_apply(u8 p) __banked {
 fog_pending[p]=0;
 u8 y,left=cam_x>>4,right=(cam_x+255)>>4,top=cam_y>>4,bottom=(cam_y+159)>>4;
 for(y=top;y<=bottom;++y) {
  u8* row=fog_changed[p]+((u16)y<<2);u8 x=left,v=row[x>>3]>>(x&7),start=NONE;
  while(x<=right) {
   if(!(x&7))v=row[x>>3];
   if(v&1) {if(start==NONE)start=x;}
   else if(start!=NONE) {restore_field((u16)start<<4,(u16)y<<4,(u16)(x-start)<<4,16,p,cam_x,cam_y);start=NONE;}
   if(!v)x=(x|7)+1;else {++x;v>>=1;}
  }
  if(start!=NONE) {x=right+1;restore_field((u16)start<<4,(u16)y<<4,(u16)(x-start)<<4,16,p,cam_x,cam_y);}
  {u8 q;for(q=left>>3;q<=right>>3;++q) {u8 m=255;if(q==(left>>3))m<<=left&7;if(q==(right>>3))m&=255>>(7-(right&7));row[q]&=~m;}}
 }
}
