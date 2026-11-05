#include "../shared.h"
void projectile_tick(void) __banked {
 u8 i;shot_active=0;
 for(i=0;i<PS;++i)if(shot_time[i]) {
  shot_x[i]+=shot_dx[i];shot_y[i]+=shot_dy[i];sprites_dirty=3;
  if(!--shot_time[i])combat_impact(shot_target[i],shot_damage[i],shot_owner[i]);
  else shot_active|=((u16)1<<i);
 }
}
u16 arrow_damage(u8 p,u16 mask) __banked {
 u8 i,j;Rect a;if(!arrow_old[p])return mask;a.w=a.h=5;
 for(i=0;i<PS;++i)if(arrow_old[p]&((u16)1<<i)) {
  a.x=arrow_old_x[p][i]-2;a.y=arrow_old_y[p][i]-2;
  for(j=0;j<RN;++j)if(intersect(&a,rect_now+j))mask|=render_bit[j];
 }
 return mask;
}
void arrow_draw(u8 p) __banked {
 u8 i;arrow_old[p]=0;if(!shot_active)return;
 for(i=0;i<PS;++i)if(shot_time[i]) {
  i16 x=shot_x[i],y=shot_y[i];
  if(fog_at(x>>4,y>>4)!=2)continue;
  if(x+2<(i16)cam_x || x-2>=(i16)cam_x+256 || y+2<(i16)cam_y || y-2>=(i16)cam_y+160)continue;
  arrow_old[p]|=((u16)1<<i);arrow_old_x[p][i]=x;arrow_old_y[p][i]=y;
  if(shot_dx[i]) {world_fill(x-2,y,4,1,p,11);world_fill(x+(shot_dx[i]>0?2:-2),y-1,1,3,p,15);}
  else {world_fill(x,y-2,1,4,p,11);world_fill(x-1,y+(shot_dy[i]>0?2:-2),3,1,p,15);}
 }
}
