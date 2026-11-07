#include "../shared.h"
/* Fog is part of background restoration, never a full-screen redraw overlay. */
/* Window masks for one 8-cell byte: columns >= n, columns <= n. */
static const u8 col_from[8]={255,254,252,248,240,224,192,128};
static const u8 col_to[8]={1,3,7,15,31,63,127,255};
/* True when any cell of the camera window is flagged in the page's change map. */
static u8 window_flagged(const u8* map,u8 left,u8 right,u8 top,u8 bottom) {
 u8 y,q,lq=left>>3,rq=right>>3;
 for(y=top;y<=bottom;++y) {
  const u8* row=map+((u16)y<<2);
  for(q=lq;q<=rq;++q) {u8 m=255;if(q==lq)m&=col_from[left&7];if(q==rq)m&=col_to[right&7];if(row[q]&m)return 1;}
 }
 return 0;
}
void fog_cover(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy) __banked {
 i16 right=x+w,bottom=y+h,x0,x1,top,bot;
 u8 c,c2,r,c0,c1,r0,r1,state,hh;
 if(x<(i16)cx)x=cx;if(y<(i16)cy)y=cy;if(right>(i16)cx+256)right=cx+256;if(bottom>(i16)cy+160)bottom=cy+160;
 if(x>=right || y>=bottom)return;
 c0=(u16)x>>4;c1=(u16)(right-1)>>4;r0=(u16)y>>4;r1=(u16)(bottom-1)>>4;
 for(r=r0;r<=r1;++r) {
  const u8* vis=fog_visible+((u16)r<<2);u8 q,all=1,lq=c0>>3,rq=c1>>3;
  /* Fully visible rows need no cover at all. */
  for(q=lq;q<=rq;++q) {u8 m=255;if(q==lq)m&=col_from[c0&7];if(q==rq)m&=col_to[c1&7];if((vis[q]&m)!=m){all=0;break;}}
  if(all)continue;
  top=(i16)r<<4;bot=top+16;if(top<y)top=y;if(bot>bottom)bot=bottom;hh=bot-top;
  for(c=c0;c<=c1;) {
   state=fog_at(c,r);
   if(state==2){++c;continue;}
   c2=c+1;
#ifdef KAW_V9990
   /* Equal-state neighbours share one copy from a 256-pixel-wide strip. */
   while(c2<=c1 && c2-c<15 && fog_at(c2,r)==state)++c2;
#endif
   x0=(i16)c<<4;if(x0<x)x0=x;x1=(i16)c2<<4;if(x1>right)x1=right;
#ifdef KAW_V9990
   clip_copy(x0,top,(state?256:0)+(x0&15),(state?1248:1264)+(top&15),x1-x0,hh,p,cx,cy);
#else
   clip_copy(x0,top,(state?64:80)+(x0&15),656+(top&15),x1-x0,hh,p,cx,cy);
#endif
   c=c2;
  }
 }
}
u8 fog_page_pending(u8 p) __banked {
 if(fog_checked_epoch[p]==fog_epoch && fog_checked_x[p]==cam_x && fog_checked_y[p]==cam_y)return fog_pending[p];
 fog_checked_epoch[p]=fog_epoch;fog_checked_x[p]=cam_x;fog_checked_y[p]=cam_y;
 fog_pending[p]=window_flagged(fog_changed[p],cam_x>>4,(cam_x+255)>>4,cam_y>>4,(cam_y+159)>>4);
 return fog_pending[p];
}
#ifdef KAW_V9990
/* V9990: restoring from the resident world is a single cheap blitter copy, so every changed cell of the
 * window is covered by ONE bounding rectangle (redrawing the extra unchanged pixels is harmless, and
 * the actors touching it are redrawn). This replaces the per-run scans and per-run copies. */
static u8 fb_l,fb_t,fb_r,fb_b;
static u8 fog_bbox(u8 p) {
 u8 left=cam_x>>4,right=(cam_x+255)>>4,top=cam_y>>4,bottom=(cam_y+159)>>4;
 u8 lq=left>>3,rq=right>>3,y,q,b,m,ymin=255,ymax=0,c0=0,c1=0,c2=0,c3=0;
 for(y=top;y<=bottom;++y) {
  const u8* row=fog_changed[p]+((u16)y<<2);u8 any=0;
  for(q=lq;q<=rq;++q) {
   m=255;if(q==lq)m&=col_from[left&7];if(q==rq)m&=col_to[right&7];
   b=row[q]&m;
   if(b) {any=1;if(q==0)c0|=b;else if(q==1)c1|=b;else if(q==2)c2|=b;else c3|=b;}
  }
  if(any) {if(ymin==255)ymin=y;ymax=y;}
 }
 if(ymin==255)return 0;
 {u8 col[4];col[0]=c0;col[1]=c1;col[2]=c2;col[3]=c3;
  for(q=lq;!col[q];++q);
  for(b=0;!(col[q]&fog_bit_table[b]);++b);
  fb_l=(q<<3)+b;
  for(q=rq;!col[q];--q);
  for(b=7;!(col[q]&fog_bit_table[b]);--b);
  fb_r=(q<<3)+b;}
 fb_t=ymin;fb_b=ymax;return 1;
}
u8 fog_query(u8 p,i16* l,i16* t,i16* r,i16* b) __banked {
 if(!fog_bbox(p))return 0;
 *l=(i16)fb_l<<4;*r=((i16)fb_r+1)<<4;*t=(i16)fb_t<<4;*b=((i16)fb_b+1)<<4;return 1;
}
void fog_apply(u8 p) __banked {
 u8 y,left=cam_x>>4,right=(cam_x+255)>>4,top=cam_y>>4,bottom=(cam_y+159)>>4;
 fog_pending[p]=0;
 if(fog_bbox(p))restore_field((u16)fb_l<<4,(u16)fb_t<<4,(u16)(fb_r-fb_l+1)<<4,(u16)(fb_b-fb_t+1)<<4,p,cam_x,cam_y);
 for(y=top;y<=bottom;++y) {
  u8* row=fog_changed[p]+((u16)y<<2);u8 q;
  for(q=left>>3;q<=right>>3;++q) {u8 m=255;if(q==(left>>3))m&=col_from[left&7];if(q==(right>>3))m&=col_to[right&7];row[q]&=~m;}
 }
}
#else
/* Scan byte masks, skipping empty spans and combining adjacent dirty cells.
 * Unchanged cells and damage outside this camera remain untouched. */
static u16 fog_hit(u16 mask,u8 c0,u8 c1,u8 y) {
 i16 l=(i16)c0<<4,r=(i16)c1<<4,t=(i16)y<<4,b=t+16;u8 k;
 for(k=0;k<dm_n;++k)if(!(mask&dm[k].bit) && dm[k].l<r && dm[k].r>l && dm[k].t<b && dm[k].b>t)mask|=dm[k].bit;
 return mask;
}
u16 fog_damage(u8 p,u16 mask) __banked {
 u8 y,left=cam_x>>4,right=(cam_x+255)>>4,top=cam_y>>4,bottom=(cam_y+159)>>4;
 /* Visible actors were collected by damage_mask (dm_*); a flagged run only needs those. */
 for(y=top;y<=bottom;++y) {
  u8* row=fog_changed[p]+((u16)y<<2);u8 x=left,v=row[x>>3]>>(x&7),start=NONE;
  while(x<=right) {
   if(!(x&7))v=row[x>>3];
   if(v&1) {if(start==NONE)start=x;}
   else if(start!=NONE) {mask=fog_hit(mask,start,x,y);start=NONE;}
   if(!v)x=(x|7)+1;else {++x;v>>=1;}
  }
  if(start!=NONE)mask=fog_hit(mask,start,right+1,y);
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
#endif
