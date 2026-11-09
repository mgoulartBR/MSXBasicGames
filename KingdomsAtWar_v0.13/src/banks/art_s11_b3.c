#include "../shared.h"
u8 line_clear(u8 x,u8 y,u8 tx,u8 ty,u8 water,u8 ignore) __banked {
 i16 dx=(i16)tx-x,dy=(i16)ty-y,sx=dx<0?-1:1,sy=dy<0?-1:1,err,e;
 if(dx<0)dx=-dx;if(dy<0)dy=-dy;dy=-dy;err=dx+dy;
 while(x!=tx || y!=ty) {
  e=err*2;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}
  if(x!=tx || y!=ty){u8 t=tile(x,y);if(t==ignore)continue;if(t==1 || t==4 || t>=10 || (t==2 && !water))return 0;}
 }
 return 1;
}
u8 military_damage(u8 kind,u8 owner) __banked {return unit_damage[kind]+(!owner && research_level && kind>=1 && kind<=5?2:0);}
u8 structure_max_hp(u8 id) __banked {return buildings[id].type==0?(building_owner[id]?180:240):96;}
void structure_dirty(u8 id) __banked {
 u8 y=buildings[id].y;if(y){video_tile(buildings[id].x+1,y-1);video_tile(buildings[id].x+2,y-1);}hud_dirty=3;sprites_dirty=3;
}
static void hp_text(const c8* name,u8 hp,u8 max) {
 c8 text[30];u8 i=0;while(*name && i<14)text[i++]=*name++;text[i++]=' ';text[i++]='H';text[i++]='P';text[i++]=' ';
 text[i++]='0'+hp/100;text[i++]='0'+hp/10%10;text[i++]='0'+hp%10;text[i++]='/';text[i++]='0'+max/100;text[i++]='0'+max/10%10;text[i++]='0'+max%10;text[i]=0;hud_context_text(text);
}
u8 structure_context(void) __banked {
 u8 id;if(!building_selected)return 0;id=building_selected-1;hp_text(build_names[buildings[id].type],building_hp[id],structure_max_hp(id));return 1;
}
u8 repair_context(void) __banked {
 u8 id;if(!WORKER(selected) || repair_target[selected]==NONE)return 0;id=repair_target[selected];hp_text("REPAIR",building_hp[id],structure_max_hp(id));return 1;
}
void tower_tick(void) __banked {
 u8 id,j,k,best,best_d,d,n,sx,sy;Building* b;
 for(id=0;id<building_count;++id) {
  b=&buildings[id];if(b->state!=2 || b->type!=6 || !building_hp[id])continue;
  if(tower_cooldown[id]){--tower_cooldown[id];continue;}
  /* Idle scans are staggered. Active arrows remain in the normal projectile pool. */
  if((sim_ticks+id)&7)continue;
  best=NONE;best_d=255;n=building_size(b->type);
  for(j=0;j<RN;++j)if(combat_alive(j) && units[j].owner!=building_owner[id] && !(WORKER(j) && mine_phase[j]==2) && (building_owner[id] || fog_target(j))) {
   sx=units[j].x<b->x?b->x:units[j].x>=b->x+n?b->x+n-1:units[j].x;
   sy=units[j].y<b->y?b->y:units[j].y>=b->y+n?b->y+n-1:units[j].y;
   d=(units[j].x>sx?units[j].x-sx:sx-units[j].x)+(units[j].y>sy?units[j].y-sy:sy-units[j].y);
   if(d && d<=6 && d<best_d && line_clear(sx,sy,units[j].x,units[j].y,1,10+id)){best=j;best_d=d;}
  }
  if(best==NONE)continue;
  for(k=RN;k<PS;++k)if(!shot_time[k])break;if(k==PS)continue;
  sx=b->x+n/2;sy=b->y+n/2;
  shot_x[k]=(u16)sx*16+8;shot_y[k]=(u16)sy*16+4;shot_dx[k]=(i16)units[best].x-sx;shot_dy[k]=(i16)units[best].y-sy;
  shot_target[k]=best;shot_owner[k]=building_owner[id];shot_damage[k]=6+(!building_owner[id] && research_level?2:0);shot_time[k]=16;shot_active|=((u16)1<<k);
  tower_cooldown[id]=42;++tower_shots;sprites_dirty=3;
 }
}
