#include "../shared.h"
void projectile_tick(void) __banked {
 u8 i;shot_active=0;
 for(i=0;i<PS;++i)if(shot_time[i]) {
  shot_x[i]+=shot_dx[i];shot_y[i]+=shot_dy[i];sprites_dirty=3;
  if(!--shot_time[i])combat_impact(shot_target[i],shot_damage[i],shot_owner[i]);
  else shot_active|=((u16)1<<i);
 }
}
