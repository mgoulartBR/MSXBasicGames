#ifdef KAW_V9990
#include "video9990.h"
#else
#include "video9968.h"
#endif
const u16 render_bit[RN]={1,2,4,8,16,32,64,128,256,512};
#ifndef KAW_V9990
const u8 small_tx[]={0,16,0,16},small_ty[]={0,0,16,16};
const u8 large_tx[]={0,16,32,0,16,32,0,16,32},large_ty[]={0,0,0,16,16,16,32,32,32};
#endif
u8 tile_buffer[128],boot_poses;
i16 strip_l[2],strip_t[2],strip_r[2],strip_b[2];u8 strip_n;
u8 box_old[2];u16 box_l[2],box_t[2],box_r[2],box_b[2];
u8 cache_key[CACHE_COUNT],cache_stamp[CACHE_COUNT],cache_tick,unit_cache[RN];
u16 frame_uploads,frame_hits,rendered_frames;
#include "render_types.h"
Rect rect_now[RN],rect_old[2][RN];
i16 actor_x[RN],actor_y[RN];u8 actor_visible[RN],actor_key[RN],old_hp[2][RN],old_hit[2][RN],actor_sel[2],old_key[2][RN],field_valid[2];
u16 field_x[2],field_y[2];u8 changed_x[2][32],changed_y[2][32],changed_count[2],ghost_old[2],ghost_size[2],ghost_color_old[2];u16 ghost_x[2],ghost_y[2];
u8 intersect(const Rect* a,const Rect* b) {return a->w && b->w && a->x+a->w>b->x && b->x+b->w>a->x && a->y+a->h>b->y && b->y+b->h>a->y;}
void clip_copy(i16 wx,i16 wy,u16 sx,u16 sy,u8 w,u8 h,u8 p,u16 cx,u16 cy) {
 i16 left=wx,top=wy,right=wx+w,bottom=wy+h,source_top;
 if(left<(i16)cx)left=cx;if(top<(i16)cy)top=cy;if(right>(i16)cx+256)right=cx+256;if(bottom>(i16)cy+160)bottom=cy+160;
 if(left>=right||top>=bottom)return;
 sx+=left-wx;sy+=top-wy;source_top=top;
#ifdef KAW_V9990
 gpu_copy(sx,sy,left-cx,(u16)p*256+top-cy,right-left,bottom-top,1);
#else
 {u8 l8=(u8)left,t8=(u8)top;u16 W=right-left,H=bottom-top;
  if((u16)l8+W<=256 && (u16)t8+H<=256){gpu_copy(sx,sy,l8,(u16)p*256+t8,W,H,1);return;}}
 /* Split at physical wrap boundaries; the source image is not circular. */
 while(top<bottom) {i16 x=left;u16 hh=256-(top&255);if(hh>bottom-top)hh=bottom-top;
  while(x<right) {u16 ww=256-(x&255);if(ww>right-x)ww=right-x;gpu_copy(sx+x-left,sy+top-source_top,(u8)x,(u16)p*256+(u8)top,ww,hh,1);x+=ww;}
  top+=hh;
 }
#endif
}
/* Opaque terrain, transparent buildings over grass. */
static void paint_world(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy,u8 overlay) {
 i16 xx,yy,left=x,top=y,right=x+w,bottom=y+h;
#ifndef KAW_V9990
 if(left<(i16)cx)left=cx;if(top<(i16)cy)top=cy;if(right>(i16)cx+256)right=cx+256;if(bottom>(i16)cy+160)bottom=cy+160;
#endif
 if(left<0)left=0;if(top<0)top=0;if(right>512)right=512;if(bottom>512)bottom=512;
 for(yy=top;yy<bottom;) {u8 hh=16-(yy&15),cy8=(u16)yy>>4,cell=(u16)left>>4;
  if(hh>bottom-yy)hh=bottom-yy;
  for(xx=left;xx<right;++cell) {u8 ww=16-(xx&15),t=fog_art(cell,cy8),phy=tile_source[t];u16 dx,dy;
   if(ww>right-xx)ww=right-xx;
#ifdef KAW_V9990
   if(overlay) {dx=xx-cx;dy=(u16)p*256+yy-cy;}else {dx=xx;dy=512+yy;}
#else
   dx=(u8)xx;dy=(u16)p*256+(u8)yy;
#endif
   if(!overlay && t>=64)gpu_fill(dx,dy,ww,hh,3);
   gpu_copy(((phy&15)<<4)+(xx&15),STATIC_Y+((phy>>4)<<4)+(yy&15),dx,dy,ww,hh,t>=64);xx+=ww;++terrain_copies;
  }yy+=hh;
 }
#ifndef KAW_V9990
 if(!overlay)fog_cover(x,y,w,h,p,cx,cy);
#endif
}
#ifdef KAW_V9990
/* V9990: the resident world already carries the fog look (unknown = solid, remembered = dither),
 * so restoring the screen is one copy and scrolling never re-evaluates the fog. */
void video_world_cell(u8 x,u8 y) {
 u8 st=fog_at(x,y);u16 dx=(u16)x*16,dy=512+(u16)y*16;
 if(!st){gpu_copy(16,1248,dx,dy,16,16,0);return;}
 paint_world(dx,(u16)y*16,16,16,0,0,0,0);
 if(st==1)gpu_copy(256,1248,dx,dy,16,16,1);
}
void video_world_tile(u8 x,u8 y) {paint_world((u16)x*16,(u16)y*16,16,16,0,0,0,0);}
void video_world_dither(u8 x,u8 y,u8 n) {gpu_copy(256,1248,(u16)x*16,512+(u16)y*16,(u16)n*16,16,1);}
#endif
void world_fill(i16 x,i16 y,u8 w,u8 h,u8 p,u8 c) {
 i16 right=x+w,bottom=y+h;if(x<(i16)cam_x)x=cam_x;if(y<(i16)cam_y)y=cam_y;if(right>(i16)cam_x+256)right=cam_x+256;if(bottom>(i16)cam_y+160)bottom=cam_y+160;
 while(y<bottom) {i16 xx=x;u16 hh=256-(y&255);if(hh>bottom-y)hh=bottom-y;
  while(xx<right) {u16 ww=256-(xx&255);if(ww>right-xx)ww=right-xx;
#ifdef KAW_V9990
   gpu_fill(xx-cam_x,(u16)p*256+y-cam_y,ww,hh,c);
#else
   gpu_fill((u8)xx,(u16)p*256+(u8)y,ww,hh,c);
#endif
   xx+=ww;
  }y+=hh;
 }
}
/* Restore only damaged screen pixels from the resident world or tile atlas. */
void restore_field(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy) {
#ifdef KAW_V9990
 i16 right=x+w,bottom=y+h;
 if(x<(i16)cx)x=cx;if(y<(i16)cy)y=cy;if(right>(i16)cx+256)right=cx+256;if(bottom>(i16)cy+160)bottom=cy+160;
 if(right>x && bottom>y)gpu_copy(x,512+y,x-cx,(u16)p*256+y-cy,right-x,bottom-y,0);
#else
 paint_world(x,y,w,h,p,cx,cy,0);
#endif
}
static void erase_border(u8 p,u16 l,u16 t,u16 r,u16 b) {
 restore_field(l,t,r-l+1,1,p,field_x[p],field_y[p]);restore_field(l,b,r-l+1,1,p,field_x[p],field_y[p]);
 restore_field(l,t,1,b-t+1,p,field_x[p],field_y[p]);restore_field(r,t,1,b-t+1,p,field_x[p],field_y[p]);
}
static void box_draw(u8 p) {
 u16 l=drag_l,t=drag_t,r=drag_r,b=drag_b;
 if(r<cam_x || l>=cam_x+256 || b<cam_y || t>=cam_y+160)return;
 if(l<cam_x)l=cam_x;if(r>=cam_x+256)r=cam_x+255;if(t<cam_y)t=cam_y;if(b>=cam_y+160)b=cam_y+159;
 if(r>l){world_fill(l,t,r-l,1,p,15);world_fill(l,b,r-l,1,p,15);}
 world_fill(l,t,1,b-t+1,p,15);world_fill(r,t,1,b-t+1,p,15);
}
#ifndef KAW_V9990
static u8 cache_frame(u8 key) {
 u8 i,best=255,age=0,q,n=kind_sizes[key/24]>>4;u16 x,y;const u8* b=pose_bounds+(u16)key*4;
 for(i=0;i<CACHE_COUNT;++i)if(cache_key[i]==key) {cache_stamp[i]=cache_tick;++frame_hits;return i;}
 for(i=0;i<CACHE_COUNT;++i) {
  u8 j,protected=0;
  if(cache_key[i]==NONE) {best=i;break;}
  for(j=0;j<RN;++j)if(actor_visible[j] && actor_key[j]==cache_key[i])protected=1;
  if(!protected && (best==255 || (u8)(cache_tick-cache_stamp[i])>age)) {best=i;age=cache_tick-cache_stamp[i];}
 }
 if(best==255)best=0;
 asset_begin(poses+key);gpu_wait();
 __asm di __endasm;SET_BANK_SEGMENT(3,asset_bank);__asm ei __endasm;
 for(q=0;q<n*n;++q) {
  u8 tx=(n==2?small_tx:large_tx)[q],ty=(n==2?small_ty:large_ty)[q],l=b[0]>tx?b[0]:tx,t=b[1]>ty?b[1]:ty;
  u8 rr=b[0]+b[2]<tx+16?b[0]+b[2]:tx+16,bb=b[1]+b[3]<ty+16?b[1]+b[3]:ty+16,w;
  l&=254;rr=(rr+1)&254;if(l>=rr || t>=bb)continue;w=(rr-l)>>1;
  cache_xy(best,q,&x,&y);x+=l-tx;y+=t-ty;
  up_src=packed_src;up_y=y;up_xb=(u8)(x>>1);up_rows=bb-t;up_w=w;upload_piece();packed_src=up_src;
 }
 __asm di __endasm;SET_BANK_SEGMENT(3,3);__asm ei __endasm;poll_input();
 cache_key[best]=key;cache_stamp[best]=cache_tick;++frame_uploads;return best;
}
#endif
static void draw_actor(u8 i,u8 p) {
 u8 sz=kind_sizes[unit_kind[i]],key=actor_key[i],q,n=sz>>4;const u8* b=pose_bounds+(u16)key*4;u8 crop=WORKER(i) && mine_phase[i]?mine_step[i]*4:0;
 if(i<N && (selection_mask&render_bit[i]))clip_copy(actor_x[i]+sz/2-8,actor_y[i]+sz-16,0,FONT_Y+16,16,16,p,cam_x,cam_y);
#ifdef KAW_V9990
 {u8 top=b[1];if(crop>top)top=crop;if(top<b[1]+b[3])clip_copy(actor_x[i]+b[0],actor_y[i]+top,(key%10)*48+b[0],1280+(u16)(key/10)*48+top,b[2],b[1]+b[3]-top,p,cam_x,cam_y);}
#else
 /* One copy per pose row when its pieces are neighbours in the pose cache (they usually are). */
 {u8 row,slot=unit_cache[i],nn=n<<4;
  for(row=0;row<n;++row) {
   u8 ty=row<<4,top=ty<b[1]?b[1]:ty,bottom=ty+16,left=b[0],right=b[0]+b[2],q0,q1,t0;u16 sx,sy;
   if(bottom>b[1]+b[3])bottom=b[1]+b[3];
   if(top<crop)top=crop;
   if(right>nn)right=nn;
   if(top>=bottom || left>=right)continue;
   q0=row*n+(left>>4);q1=row*n+((right-1)>>4);t0=(slot<7?slot:slot-7)*9+q0;
   cache_xy(slot,q0,&sx,&sy);
   if((t0&15)+(q1-q0)<16) {
    clip_copy(actor_x[i]+left,actor_y[i]+top,sx+left-((left>>4)<<4),sy+top-ty,right-left,bottom-top,p,cam_x,cam_y);
   } else {
    for(q=q0;q<=q1;++q) {
     u8 tx=(q-row*n)<<4,l=left>tx?left:tx,r=right<tx+16?right:tx+16;
     cache_xy(slot,q,&sx,&sy);
     clip_copy(actor_x[i]+l,actor_y[i]+top,sx+l-tx,sy+top-ty,r-l,bottom-top,p,cam_x,cam_y);
    }
   }
  }
 }
#endif
 /* Indexed health bar and a brief contact flash, inside damage bounds. */
 if(units[i].owner || unit_hp[i]<unit_max_hp[unit_kind[i]] || combat_target[i]!=NONE) {i16 x=actor_x[i]+sz/2-8,y=actor_y[i]+b[1]-4;u8 width=(u16)unit_hp[i]*16/unit_max_hp[unit_kind[i]];
  world_fill(x,y,16,2,p,12);if(width)world_fill(x,y,width,2,p,hurt_flash[i]>4?15:units[i].owner?9:5);
  if(hurt_flash[i]>4){world_fill(x+6,y+12,5,1,p,15);world_fill(x+8,y+10,1,5,p,15);}
 }
 /* Restore only opaque building pixels in front of this unit. */
 for(q=0;q<building_count;++q) {
  Building* bl=&buildings[q];u8 side=building_size(bl->type)*16;Rect br; i16 x,y,right,bottom;
  if(!bl->state || (u16)(bl->y*16+side)<(u16)(units[i].py+16))continue;
  br.x=bl->x*16;br.y=bl->y*16;br.w=br.h=side;if(!intersect(&br,rect_now+i))continue;
  x=br.x>rect_now[i].x?br.x:rect_now[i].x;y=br.y>rect_now[i].y?br.y:rect_now[i].y;right=br.x+side<rect_now[i].x+rect_now[i].w?br.x+side:rect_now[i].x+rect_now[i].w;bottom=br.y+side<rect_now[i].y+rect_now[i].h?br.y+side:rect_now[i].y+rect_now[i].h;
  if(x<(i16)cam_x)x=cam_x;if(y<(i16)cam_y)y=cam_y;if(right>(i16)cam_x+256)right=cam_x+256;if(bottom>(i16)cam_y+160)bottom=cam_y+160;
  if(right>x&&bottom>y)paint_world(x,y,right-x,bottom-y,p,cam_x,cam_y,1);
 }
}
void video_sprites(void) {
 u8 p=shown^1,i,j,k,camera,order[RN],count=0,tiles_changed=changed_count[shown^1];u16 mask=0,before,restore;u8 fog_page_dirty=0;u8 ghost=placement_mode && mouse_y<160,ghost_changed=0,box=drag_down&&drag_moved,box_changed,shift=0,inc=0;
 ++cache_tick;
 for(i=0;i<RN;++i) {
  if(!combat_alive(i) || units[i].px+56<cam_x || units[i].px>cam_x+288 || units[i].py+32<cam_y || units[i].py>cam_y+208) {actor_visible[i]=0;rect_now[i].w=0;actor_key[i]=NONE;continue;}
  const u8* b;u8 size=kind_sizes[unit_kind[i]],f=unit_art_frame(i),crop=WORKER(i)&&mine_phase[i]?mine_step[i]*4:0;Rect* a=rect_now+i;
  actor_visible[i]=unit_position(i);actor_x[i]=unit_draw_x+cam_x;actor_y[i]=unit_draw_y+cam_y;actor_key[i]=unit_kind[i]*24+f;b=pose_bounds+(u16)actor_key[i]*4;
  a->x=actor_x[i]+b[0];a->y=actor_y[i]+b[1];a->w=b[2];a->h=b[3];
  {i16 left=actor_x[i]+size/2-8;if(a->x>left){a->w+=a->x-left;a->x=left;}if(a->x+a->w<left+16)a->w=left+16-a->x;if(units[i].owner || unit_hp[i]<unit_max_hp[unit_kind[i]] || combat_target[i]!=NONE){a->y-=4;a->h+=4;}}
  if(crop>b[1]) {a->y+=crop-b[1];a->h=crop<b[1]+b[3]?b[1]+b[3]-crop:0;}
  if(i<N && (selection_mask&render_bit[i])) {i16 left=actor_x[i]+size/2-8,bot=actor_y[i]+size;if(a->x>left) {a->w+=a->x-left;a->x=left;}if(a->x+a->w<left+16)a->w=left+16-a->x;if(a->y+a->h<bot)a->h=bot-a->y;}
  if(!actor_visible[i] || !a->h || a->x+a->w<=(i16)cam_x || a->x>=(i16)cam_x+256 || a->y+a->h<=(i16)cam_y || a->y>=(i16)cam_y+160) {actor_visible[i]=0;a->w=0;}
  if(actor_visible[i]) {order[count++]=i;
#ifndef KAW_V9990
   {u8 s=unit_cache[i];if(s<CACHE_COUNT && cache_key[s]==actor_key[i]){cache_stamp[s]=cache_tick;++frame_hits;}else unit_cache[i]=cache_frame(actor_key[i]);}
#endif
  }
 }
 camera=!field_valid[p] || field_x[p]!=cam_x || field_y[p]!=cam_y;
 {const Rect* rn=rect_now;const Rect* ro=rect_old[p];const u8 *ohp=old_hp[p],*ohit=old_hit[p],*okey=old_key[p];const u16* bit=render_bit;u8 vf=field_valid[p];u16 selx=actor_sel[p]^selection_mask;
  for(i=0;i<RN;++i,++rn,++ro,++bit)
   if(!vf || unit_hp[i]!=ohp[i] || hurt_flash[i]!=ohit[i] || actor_key[i]!=okey[i] || ((const u16*)rn)[0]!=((const u16*)ro)[0] || ((const u16*)rn)[1]!=((const u16*)ro)[1] || ((const u16*)rn)[2]!=((const u16*)ro)[2] || (selx&*bit))mask|=*bit;}
 fog_page_dirty=fog_page_pending(p);
 ghost_changed=ghost!=ghost_old[p] || (ghost && (ghost_x[p]!=place_x*16 || ghost_y[p]!=place_y*16 || ghost_size[p]!=building_size(build_kind)*16 || ghost_color_old[p]!=place_valid));
 box_changed=box!=box_old[p] || (box && (drag_l!=box_l[p] || drag_t!=box_t[p] || drag_r!=box_r[p] || drag_b!=box_b[p]));
 /* Restoring and redrawing have different damage sets. An unchanged
  * overlapping actor needs another draw, never another background erase. */
 restore=mask;
 /* Incremental scroll: pixels of actors already on the page stay valid (the page is a circular buffer on
  * native, or was just shifted on V9990). Only the exposed strips are new, so only actors touching them
  * (plus the usual changed ones) are redrawn. */
 strip_n=0;
 if(camera && field_valid[p]) {
  i16 dx=(i16)cam_x-(i16)field_x[p],dy=(i16)cam_y-(i16)field_y[p];
#ifdef KAW_V9990
  if(dx>=-96 && dx<=96 && dy>=-64 && dy<=64)
#else
  if(dx>-256 && dx<256 && dy>-160 && dy<160)
#endif
  {
   inc=1;
   if(dx>0){strip_l[0]=field_x[p]+256;strip_r[0]=cam_x+256;strip_t[0]=cam_y;strip_b[0]=cam_y+160;strip_n=1;}
   else if(dx<0){strip_l[0]=cam_x;strip_r[0]=field_x[p];strip_t[0]=cam_y;strip_b[0]=cam_y+160;strip_n=1;}
   if(dy>0){strip_l[strip_n]=cam_x;strip_r[strip_n]=cam_x+256;strip_t[strip_n]=field_y[p]+160;strip_b[strip_n]=cam_y+160;++strip_n;}
   else if(dy<0){strip_l[strip_n]=cam_x;strip_r[strip_n]=cam_x+256;strip_t[strip_n]=cam_y;strip_b[strip_n]=field_y[p];++strip_n;}
  }
 }
 if(camera && !inc)mask=restore=1023;
 else {
  mask=damage_mask(p,mask,(box?1:0)|(box_changed?2:0)|(ghost?4:0)|(ghost_changed?8:0)|(fog_page_dirty?16:0));
 }
 if(!mask && !fog_page_dirty && !camera && !changed_count[p] && !box_changed && !ghost_changed && !shot_active && !arrow_old[p]) {
#ifdef KAW_V9990
  if(hud_stamp[p]==hud_swaps && hud_stamp[shown]==hud_swaps)return;
#else
  return;
#endif
 }
#ifdef KAW_V9990
 /* Scroll by moving the page's own pixels (one blitter copy) and painting only the exposed strips;
  * actors are then restored/redrawn at their new screen positions like in an ordinary frame. */
 if(inc) {
  i16 dx=(i16)cam_x-(i16)field_x[p],dy=(i16)cam_y-(i16)field_y[p];
  {
   u16 ow=256-(dx<0?-dx:dx),oh=160-(dy<0?-dy:dy),base=(u16)p*256;
   gpu_move((dx>0?dx:0)+(dx<0?ow-1:0),base+(dy>0?dy:0)+(dy<0?oh-1:0),(dx<0?-dx:0)+(dx<0?ow-1:0),base+(dy<0?-dy:0)+(dy<0?oh-1:0),ow,oh,(dx<0?4:0)|(dy<0?8:0));
   field_x[p]=cam_x;field_y[p]=cam_y;shift=1;
   if(dx>0)restore_field(cam_x+256-dx,cam_y,dx,160,p,cam_x,cam_y);else if(dx<0)restore_field(cam_x,cam_y,-dx,160,p,cam_x,cam_y);
   if(dy>0)restore_field(cam_x,cam_y+160-dy,256,dy,p,cam_x,cam_y);else if(dy<0)restore_field(cam_x,cam_y,256,-dy,p,cam_x,cam_y);
  }
 }
#endif
 if(field_valid[p]
#ifdef KAW_V9990
    && (!camera || shift)
#endif
 ) {
  if(arrow_old[p])for(i=0;i<PS;++i)if(arrow_old[p]&((u16)1<<i))restore_field(arrow_old_x[p][i]-2,arrow_old_y[p][i]-2,5,5,p,field_x[p],field_y[p]);
  if(box_old[p] && (box_changed || camera))erase_border(p,box_l[p],box_t[p],box_r[p],box_b[p]);
  for(i=0;i<RN;++i)if(restore&render_bit[i]) {Rect* a=rect_old[p]+i;if(a->w)restore_field(a->x,a->y,a->w,a->h,p,field_x[p],field_y[p]);}
  if(ghost_old[p] && (ghost_changed || camera))erase_border(p,ghost_x[p],ghost_y[p],ghost_x[p]+ghost_size[p]-1,ghost_y[p]+ghost_size[p]-1);
 }
#ifdef KAW_V9990
 if(camera && !shift)gpu_copy(cam_x,512+cam_y,0,(u16)p*256,256,160,0);
#else
 if(!field_valid[p] || cam_x>field_x[p]+255 || field_x[p]>cam_x+255 || cam_y>field_y[p]+159 || field_y[p]>cam_y+159)paint_world(cam_x,cam_y,256,160,p,cam_x,cam_y,0);
 else if(camera) {
  if(cam_x>field_x[p])paint_world(field_x[p]+256,cam_y,cam_x-field_x[p],160,p,cam_x,cam_y,0);else if(cam_x<field_x[p])paint_world(cam_x,cam_y,field_x[p]-cam_x,160,p,cam_x,cam_y,0);
  if(cam_y>field_y[p])paint_world(cam_x,field_y[p]+160,256,cam_y-field_y[p],p,cam_x,cam_y,0);else if(cam_y<field_y[p])paint_world(cam_x,cam_y,256,field_y[p]-cam_y,p,cam_x,cam_y,0);
 }
#endif
 for(i=0;i<changed_count[p];++i)restore_field(changed_x[p][i]*16,changed_y[p][i]*16,16,16,p,cam_x,cam_y);changed_count[p]=0;
 if(fog_page_dirty)fog_apply(p);
 field_valid[p]=1;field_x[p]=cam_x;field_y[p]=cam_y;
 for(i=1;i<count;++i) {k=order[i];j=i;while(j && units[order[j-1]].py>units[k].py) {order[j]=order[j-1];--j;}order[j]=k;}
 for(j=0;j<count;++j) {i=order[j];if(mask&render_bit[i])draw_actor(i,p);}
 structure_bars(p);
 arrow_draw(p);
 if(ghost && (fog_page_dirty || shot_active || ghost_changed || box_changed || camera || mask || tiles_changed)) {u8 sz=building_size(build_kind)*16,c=place_valid?10:7;world_fill(place_x*16,place_y*16,sz,1,p,c);world_fill(place_x*16,place_y*16+sz-1,sz,1,p,c);world_fill(place_x*16,place_y*16,1,sz,p,c);world_fill(place_x*16+sz-1,place_y*16,1,sz,p,c);ghost_x[p]=place_x*16;ghost_y[p]=place_y*16;ghost_size[p]=sz;ghost_color_old[p]=place_valid;}
 if(box && (fog_page_dirty || shot_active || box_changed || ghost_changed || camera || mask || tiles_changed))box_draw(p);
 box_old[p]=box;box_l[p]=drag_l;box_t[p]=drag_t;box_r[p]=drag_r;box_b[p]=drag_b;
 ghost_old[p]=ghost;actor_sel[p]=selection_mask;__builtin_memcpy(rect_old[p],rect_now,sizeof rect_now);__builtin_memcpy(old_key[p],actor_key,RN);__builtin_memcpy(old_hp[p],unit_hp,RN);__builtin_memcpy(old_hit[p],hurt_flash,RN);
 video_present(p);++rendered_frames;
}
u8 video_camera_ready(u16 y) {y;return 1;}
void video_camera(void) {sprites_dirty|=1;}
void video_stream(void) {}
void video_tile(u8 x,u8 y) {
 u16 cell=((u16)y<<5)+x;if(x>=32 || y>=32 || fog_at(x,y)!=2)return;fog_memory[cell]=art_map[cell];video_known_tile(x,y);
}
void video_known_tile(u8 x,u8 y) {
#ifdef KAW_V9990
 paint_world(x*16,y*16,16,16,0,0,0,0);
#endif
 {u8 p;u16 xx=(u16)x*16,yy=(u16)y*16;for(p=0;p<2;++p) {
  if(!field_valid[p] || xx+16<=field_x[p] || xx>=field_x[p]+256 || yy+16<=field_y[p] || yy>=field_y[p]+160)continue;
  if(changed_count[p]<32){u8 n=changed_count[p]++;changed_x[p][n]=x;changed_y[p][n]=y;}else field_valid[p]=0;
 }}
 sprites_dirty|=1;
}
static void loading_text(u8 x,u8 y,const c8* text) {
 while(*text) {u8 c=*text++-32;gpu_copy((c&31)*8,FONT_Y+(c>>5)*8,x,y,8,8,0);x+=8;}
}
void video_init(void) {
 u8 i,j,q,chunk;u16 x,y;
 VDP_SetMode(VDP_MODE_SCREEN5);VDP_SetFrequency(VDP_FREQ_60HZ);VDP_EnableVBlank(TRUE);VDP_EnableDisplay(FALSE);
#ifndef KAW_V9990
 video_ready=present_pending=0;vframes=0;display_x=display_y=0;
#ifdef KAW_PREVIEW
 vport=0x99;chip_id=2;
#else
 for(i=0;i<2;++i) {vport=i?0x89:0x99;__asm di __endasm;vp_out(vport+3,0);raw_reg(21,0x3A);raw_reg(15,1);chip_id=(vp_in(vport)>>1)&31;raw_reg(15,0);__asm ei __endasm;if(chip_id==3||chip_id==4)break;}
 if(chip_id!=3&&chip_id!=4) {VDP_SetMode(VDP_MODE_SCREEN0);VDP_EnableDisplay(TRUE);BIOS_TextPrint("V9968 NOT DETECTED\r\nKAW68PV = V9958 PREVIEW");for(;;)Halt();}
#endif
 reg(0,6);reg(1,2);reg(2,0x1F);reg(5,0xA7);reg(6,0x2B);reg(7,1);reg(8,8);reg(9,0x80);reg(10,0);reg(11,2);reg(18,0);reg(23,0);reg(25,0);reg(26,0);reg(27,0);reg(16,0);
 __asm di __endasm;for(i=0;i<32;++i)vp_out(vport+1,palette68[i]);__asm ei __endasm;
 for(i=0;i<CACHE_COUNT;++i)cache_key[i]=NONE;
#else
 V9_SetPort(V9_P07,2);V9_SetPort(V9_P07,0);V9_SetScreenMode(V9_MODE_B1);V9_SetDisplayEnable(FALSE);V9_SetCursorEnable(FALSE);V9_SetInterrupt(V9_INT_NONE);V9_SetImageSpaceWidth(V9_R06_WIDH_512);V9_SetColorMode(V9_COLOR_BP4);V9_SelectPaletteBP4(0);V9_SetBackgroundColor(1);V9_SetCommandWriteMask(0xFFFF);
 for(i=0;i<64;++i)V9_SetPaletteEntry(i,palette9+(i&15)*3);
 V9_SetScrolling(0,0);
#endif
 for(i=0;i<2;++i)field_valid[i]=changed_count[i]=ghost_old[i]=box_old[i]=0;
#ifdef KAW_V9990
 hud_stamp[0]=hud_stamp[1]=65535;
#endif
 shown=0;cache_tick=frame_uploads=frame_hits=rendered_frames=0;
 for(chunk=0;chunk<3;++chunk) {asset_begin(static_assets+chunk);for(i=0;i<64;++i) {u8 t=chunk*64+i;asset_read(tile_buffer,128);gpu_write_tile((t&15)*16,STATIC_Y+(t>>4)*16,tile_buffer);}}
 asset_begin(&font_asset);for(i=0;i<16;++i) {asset_read(tile_buffer,128);
#ifdef KAW_V9990
 V9_WriteVRAM((u32)(FONT_Y+i)*256,tile_buffer,128);
#else
 vwrite((u32)(FONT_Y+i)*128,tile_buffer,128);
#endif
 }
 gpu_write_tile(0,FONT_Y+16,ring_pixels);
 for(i=0;i<128;++i)tile_buffer[i]=(i&8)?0x01:0x10;
#ifdef KAW_V9990
 {u8 k;for(k=0;k<16;++k)gpu_write_tile(256+k*16,1248,tile_buffer);for(i=0;i<128;++i)tile_buffer[i]=0x11;for(k=0;k<16;++k)gpu_write_tile(k*16,1264,tile_buffer);}
#else
 gpu_write_tile(64,656,tile_buffer);for(i=0;i<128;++i)tile_buffer[i]=0x11;gpu_write_tile(80,656,tile_buffer);
#endif

#ifdef KAW_V9990
 for(i=0;i<16;++i) {for(j=0;j<128;++j)tile_buffer[j]=i*17;gpu_write_tile((u16)i*16,1248,tile_buffer);}
#endif
 gpu_fill(0,0,256,212,1);loading_text(64,80,"KINGDOMS AT WAR");loading_text(88,112,"CARREGANDO");gpu_fill(32,136,192,4,12);gpu_wait();
#ifdef KAW_V9990
 V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);
#else
 reg(8,10);reg(1,0x62);
#endif

 for(i=0;i<2;++i) {hud_draw=i;video_fill(0,160,256,52,1);video_fill(0,160,256,2,10);video_text(8,168,"[WOOD");video_text(136,168,"]GOLD");video_text(8,184,"^TIME");video_text(136,184,"_FREE");}
 hud_draw=0;
#ifndef KAW_V9990
 vwrite(0x15800,cursor,32);for(i=0;i<32;++i)tile_buffer[i]=(i&15)?cursor[i-1]:0;vwrite(0x15820,tile_buffer,32);for(i=0;i<16;++i)tile_buffer[i]=15;vwrite(0x15000,tile_buffer,16);tile_buffer[0]=216;vwrite(0x15200,tile_buffer,1);
 for(i=0;i<8;++i) {for(j=0;j<4;++j)tile_buffer[j]=((cursor[i]&(128>>(j*2)))?240:0)|((cursor[i]&(64>>(j*2)))?15:0);vwrite((u32)(656+i)*128+8,tile_buffer,4);}
 for(i=0;i<2;++i) {field_valid[i]=changed_count[i]=ghost_old[i]=hud_pointer_saved[i]=0;}
#else
 /* All 104 poses coexist, independent of sprite pattern windows or limits. */
 boot_poses=0;for(i=0;i<168;++i)if(poses[i].bank) {u8 n=kind_sizes[i/24]>>4;asset_begin(poses+i);for(q=0;q<n*n;++q) {asset_read(tile_buffer,128);gpu_write_tile((i%10)*48+(q%n)*16,1280+(u16)(i/10)*48+(q/n)*16,tile_buffer);}++boot_poses;gpu_fill(32,136,(u16)boot_poses*192/120,4,10);}
 for(y=0;y<32;++y)for(x=0;x<32;++x)video_world_cell(x,y);
 for(i=0;i<128;++i)tile_buffer[i]=0;for(i=0;i<16;++i)tile_buffer[i*4]=cursor[i];V9_SetCursorPattern(0,tile_buffer);V9_SetCursorPalette(3);V9_SetCursorEnable(TRUE);V9_SetCursorDisplay(1,FALSE);
#endif
 gpu_wait();
}
void video_enable(void) {
#ifdef KAW_V9990
 hud_stamp[0]=hud_stamp[1]=65535;video_sprites();V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);video_irq_install();
#else
 gpu_wait();__asm di __endasm;BIOS_SetHookDirectCallback(H_KEYI,irq_video);video_ready=1;field_registers();raw_reg(1,0x62);__asm ei __endasm;
#endif
 video_enabled=1;video_cursor();
}
