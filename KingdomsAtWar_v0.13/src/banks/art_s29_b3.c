#include "../shared.h"
u8 has_building(u8 type) __banked {
 u8 i;for(i=0;i<building_count;++i)if(!building_owner[i] && buildings[i].state==2 && buildings[i].type==type)return 1;return 0;
}
u8 population_limit(void) __banked {
 u8 i,n=4;for(i=1;i<building_count;++i)if(!building_owner[i] && buildings[i].state==2 && buildings[i].type==1)n+=2;
 return n>N?N:n;
}
u8 population_free(void) __banked {
 u8 i,n=0,cap=population_limit();for(i=0;i<unit_count;++i)if(unit_hp[i])++n;for(i=0;i<building_count;++i)if(buildings[i].training && buildings[i].training<7)++n;
 return n>=cap?0:cap-n;
}
u8 build_unlocked(u8 kind) __banked {
 if(kind==4)return has_building(2) && has_building(3);
 if(kind==5 || kind==7)return has_building(4);
 if(kind==6)return has_building(3);
 return 1;
}
void building_refresh(u8 id) __banked {
 Building* b=&buildings[id];u8 x,y,n=building_size(b->type);fog_refresh=1;fog_build_dirty=1;
 for(y=b->y;y<b->y+n;++y)for(x=b->x;x<b->x+n;++x) {
  nav_base_dirty=1;terrain[((u16)y<<5)+x]=b->state?10+id:0;
  art_map[((u16)y<<5)+x]=art_index(x,y);video_tile(x,y);
 }
}
u8 placement_valid(u8 x,u8 y) __banked {
 u8 dx,dy,i,n=building_size(build_kind);
 if(!build_unlocked(build_kind) || x>32-n || y>32-n || wood<build_wood[build_kind] || gold<build_gold[build_kind])return 0;
 for(i=1;i<BMAX;++i)if(!buildings[i].state)break;
 if(i==BMAX)return 0;
 for(dy=y;dy<y+n;++dy)for(dx=x;dx<x+n;++dx) {
  u8 a=art_tile(dx,dy);
  if(fog_at(dx,dy)!=2 || tile(dx,dy)!=0 || (a>=29 && a<=58) || ((dx==9 || dx==10) && dy==9))return 0;
  for(i=0;i<RN;++i) {
   Unit* u=&units[i];
   if(combat_alive(i) && ((u->x==dx && u->y==dy) || (u->progress && u->nx==dx && u->ny==dy) || ((u->state==PLANNING || u->state==MOVING) && u->tx==dx && u->ty==dy)))return 0;
  }
 }
 return 1;
}
void build_assign(u8 id,u8 who) __banked {
 Building* b=&buildings[id];
 if(b->worker!=NONE) {builder_working&=~(1<<b->worker);unit_build[b->worker]=NONE;}
 if(unit_build[who]!=NONE)buildings[unit_build[who]].worker=NONE;
 repair_target[who]=NONE;repair_timer[who]=0;repair_active&=~(1<<who);b->worker=who;unit_build[who]=id;work_stop(who);worker_task[who]=0;
 work_route(who,b->x,b->y);sprites_dirty=3;
}
u8 building_order(u8 x,u8 y) __banked {
 u8 i,id=tile(x,y);Building* b;
 if(id<10)return 0;id-=10;if(building_owner[id])return 1;b=&buildings[id];
 if(b->state==2 && building_hp[id]<structure_max_hp(id))return repair_order(id);
 if(b->state==1) {
  for(i=0;i<unit_count;++i)if(WORKER(i) && (selection_mask&(1<<i))) {build_assign(id,i);return 1;}
 } else if(b->type==3) {
  for(i=0;i<unit_count;++i)if(WORKER(i) && (selection_mask&(1<<i)) && cargo_kind[i]==1 && cargo[i]) {
   work_stop(i);worker_kind[i]=0;worker_task[i]=WAIT_BASE;work_timer[i]=0;work_tries[i]=0;
  }
  return 1;
 }
 return 0;
}
void placement_choose(u8 kind) __banked {
 if(!WORKER(selected))return;
 build_kind=kind;place_worker=selected;placement_mode=1;building_selected=0;hud_dirty=3;sprites_dirty=3;
}
void placement_toggle(void) __banked {placement_choose(placement_mode?(build_kind==7?1:build_kind+1):build_kind);}
void placement_confirm(void) __banked {
 Building* b;u8 id;
 if(!WORKER(place_worker) || !placement_valid(place_x,place_y))return;
 for(id=1;id<BMAX;++id)if(!buildings[id].state)break;
 building_owner[id]=0;building_hp[id]=96;tower_cooldown[id]=0;
 b=&buildings[id];b->type=build_kind;b->state=1;b->x=place_x;b->y=place_y;b->worker=NONE;
 b->remaining=build_kind==2?180:240;b->training=0;b->time=0;b->rallyx=place_x;b->rallyy=place_y+building_size(build_kind);
 if(id>=building_count)building_count=id+1;
 wood-=build_wood[build_kind];gold-=build_gold[build_kind];placement_mode=0;
 building_refresh(id);build_assign(id,place_worker);hud_dirty=3;sprites_dirty=3;
}
u8 recruit_allowed(u8 id,u8 kind) __banked {
 u8 type=buildings[id].type;
 if(kind==6)return type==4 && !research_level;
 if(kind==0)return type==0;
 if(kind==1 || kind==2)return type==2;
 if(kind==3)return type==2 && has_building(3);
 if(kind==4)return type==5;
 return kind==5 && type==7;
}
void train_kind(u8 kind) __banked {
 Building* b;
 if(!building_selected)return;b=&buildings[building_selected-1];
 if(b->state!=2 || b->training || !recruit_allowed(building_selected-1,kind))return;
 if(kind==6) {
  u8 j;for(j=0;j<building_count;++j)if(buildings[j].training==7)return;
  if(wood<16 || gold<16)return;wood-=16;gold-=16;b->time=180;
 } else {
  if(!population_free() || wood<recruit_wood[kind] || gold<recruit_gold[kind])return;
  wood-=recruit_wood[kind];gold-=recruit_gold[kind];b->time=kind==0?90:kind>=4?180:120;
 }
 b->training=kind+1;hud_dirty=3;
}
void train_start(void) __banked {
 u8 kind;if(!building_selected)return;
 kind=buildings[building_selected-1].type;
 train_kind(kind==0?0:kind==2?1:kind==5?4:kind==7?5:6);
}
void production_cancel(void) __banked {
 u8 id=building_selected?building_selected-1:WORKER(selected)?unit_build[selected]:NONE;
 Building* b;if(id==NONE)return;b=&buildings[id];
 if(b->state==1) {
  wood+=build_wood[b->type];gold+=build_gold[b->type];b->state=0;
  if(b->worker!=NONE) {builder_working&=~(1<<b->worker);unit_build[b->worker]=NONE;work_stop(b->worker);}
  building_refresh(id);building_selected=0;
 } else if(b->training) {
  u8 k=b->training-1;
  wood+=k==6?16:recruit_wood[k];gold+=k==6?16:recruit_gold[k];b->training=0;b->time=0;
 }
 hud_dirty=3;sprites_dirty=3;
}
void production(void) __banked {
 u8 id,old=builder_working;builder_working=0;
 for(id=0;id<building_count;++id) {
  Building* b=&buildings[id];u8 who,d,j,n;i16 x,y;
  if(b->state!=1 && !b->training)continue;
  who=b->worker;n=building_size(b->type);
  if(b->state==1 && who!=NONE) {
   if(!mine_phase[who] && !units[who].progress && units[who].state!=MOVING && !requested[who] && job_unit!=who) {
    if(work_adjacent(who,b->x,b->y)) {
     builder_working|=1<<who;
     if(b->remaining && !(--b->remaining%18))hud_dirty=3;
     if(!b->remaining) {b->state=2;unit_build[who]=NONE;b->worker=NONE;builder_working&=~(1<<who);building_refresh(id);hud_dirty=3;}
    } else if(!(sim_ticks%30))work_route(who,b->x,b->y);
   }
  }
  if(b->state!=2 || !b->training)continue;
  if(b->time) {if(!(--b->time%12))hud_dirty=3;continue;}
  if(b->training==7) {research_level=1;b->training=0;hud_dirty=3;continue;}
  for(d=0;d<n*4;++d) {
   x=b->x;y=b->y;
   if(d<n) {x+=d;y+=n;}else if(d<n*2) {x+=n;y+=d-n;}
   else if(d<n*3) {x+=d-n*2;--y;}else {--x;y+=d-n*3;}
   if(x<0||x>=32||y<0||y>=32||!free_at(x,y,NONE))continue;
   for(j=0;j<unit_count;++j)if((units[j].state==PLANNING || units[j].state==MOVING) && units[j].tx==x && units[j].ty==y)break;
   if(j<unit_count)continue;
   for(who=0;who<unit_count;++who)if(!unit_hp[who])break;
   if(who>=N)break;
   {Unit* u=&units[who];
    u->x=u->tx=u->nx=x;u->y=u->ty=u->ny=y;u->px=x*16;u->py=y*16;
    u->owner=u->len=u->pos=u->progress=u->phase=u->state=0;
    repair_target[who]=NONE;repair_timer[who]=0;fog_refresh=1;fog_build_dirty=1;shot_time[who]=0;unit_kind[who]=b->training-1;unit_hp[who]=unit_max_hp[unit_kind[who]];combat_target[who]=NONE;attack_cooldown[who]=attack_flash[who]=hurt_flash[who]=stance[who]=0;unit_build[who]=NONE;
    unit_walk_dist[who]=unit_walk_phase[who]=0;requested[who]=retry_wait[who]=retry_left[who]=unit_facing[who]=unit_frame[who]=worker_task[who]=mine_phase[who]=mine_step[who]=cargo[who]=cargo_kind[who]=worker_kind[who]=work_timer[who]=0;
    work_tries[who]=0;deposit_id[who]=0;
   }
   if(who==unit_count)++unit_count;b->training=0;hud_dirty=3;sprites_dirty=3;
   if(free_at(b->rallyx,b->rallyy,who))order_one(who,b->rallyx,b->rallyy);
   break;
  }
 }
 if(old!=builder_working)sprites_dirty=3;
}
void hud_context_text(const c8* text) __banked {
 u8 i,p=hud_draw;for(i=0;i<29;++i) {
  u8 c=*text?*text++:' ';
  if(hud_context[p][i]!=c) {video_char(8+i*8,176,c);hud_context[p][i]=c;}
 }
}
void hud_actions(void) __banked {
 u8 p=hud_draw,i,step=0,old;const c8* line;
 if(placement_mode) {
  c8 text[30];u8 k=0;const c8* name=build_names[build_kind];
  while(*name)text[k++]=*name++;text[k++]=' ';text[k++]='[';text[k++]='0'+build_wood[build_kind]/10;text[k++]='0'+build_wood[build_kind]%10;
  text[k++]=']';text[k++]='0'+build_gold[build_kind]/10;text[k++]='0'+build_gold[build_kind]%10;
  text[k++]=' ';text[k++]=build_unlocked(build_kind)?(place_valid?'O':'X'):'L';text[k]=0;hud_context_text(text);
 } else if(selection_count>1)group_context();
 else if(repair_context() || combat_context() || structure_context()) {}
 else if(building_selected)hud_context_text(build_names[buildings[building_selected-1].type]);
 else if(selected!=NONE)hud_context_text(recruit_names[unit_kind[selected]]);
 else hud_context_text("HUMAN KINGDOM");
 if(military_selected())line="STOP      HOLD";
 else if(WORKER(selected))line="HOU BAR SAW FOR STA TOW SIE";
 else if(building_selected) {
  Building* b=&buildings[building_selected-1];
  line=b->state==1?"WORK IN PROGRESS":b->type==0?"PEASANT [00]08":b->type==2?"SWORD    SPEAR    ARCHER":b->type==5?"KNIGHT [16]24":b->type==7?"RAM [24]16":b->type==4?(research_level?"ATK+2  ARMOR+1":"UPGRADE [16]16"):"";
  if(b->state==1)step=b->type==2?(180-b->remaining)/18:(240-b->remaining)/24;
  else if(b->training) {u16 total=b->training==1?90:b->training>=5?180:120;step=(total-b->time)*10/total;}
 } else line="";
 for(i=0;i<29;++i) {
  u8 c=*line?*line++:' ';
  if(hud_action_cache[p][i]!=c) {video_char(8+i*8,200,c);hud_action_cache[p][i]=c;}
 }
 /* Action row is rendered only when its context changes, tracked separately. */
 old=hud_progress[p][0];
 if(step>10)step=10;
 if(old!=step)for(i=0;i<10;++i)if(old==255 || (i<old)!=(i<step))video_char(8+i*8,192,i<step?'<':'>');
 hud_progress[p][0]=step;
}
