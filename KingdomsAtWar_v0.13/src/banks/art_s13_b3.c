#include "../shared.h"
void economy(void) __banked {
 u8 i;
 for(i=0;i<unit_count;++i) {
  if(!worker_task[i] || !WORKER(i))continue;
  Unit* u=&units[i];u8 task=worker_task[i];u16 cell=((u16)node_y[i]<<5)+node_x[i];
  if(u->progress || mine_phase[i]==1 || mine_phase[i]==3)continue;
  if(task==WAIT_NODE || task==WAIT_BASE) {
   if(work_timer[i]) {--work_timer[i];continue;}
   if(task==WAIT_NODE && (!stock[cell] || (terrain[cell]!=1 && terrain[cell]!=4))) {worker_task[i]=0;u->state=ARRIVED;continue;}
   if(work_route(i,task==WAIT_NODE?node_x[i]:buildings[deposit_id[i]].x,task==WAIT_NODE?node_y[i]:buildings[deposit_id[i]].y)) {
    worker_task[i]=task==WAIT_NODE?TO_NODE:TO_BASE;
   } else work_timer[i]=30;
  } else if(task==HARVEST) {
   if(!stock[cell]) {worker_task[i]=0;u->state=ARRIVED;if(mine_phase[i]) {mine_phase[i]=3;fog_refresh=1;sprites_dirty=3;}continue;}
   if(work_timer[i]) {--work_timer[i];continue;}
   cargo[i]=stock[cell]<8?stock[cell]:8;cargo_kind[i]=worker_kind[i];stock[cell]-=cargo[i];++pickups;
   if(!stock[cell])resource_empty(node_x[i],node_y[i]);
   if(mine_phase[i]) {mine_phase[i]=3;fog_refresh=1;sprites_dirty=3;}
   worker_task[i]=WAIT_BASE;work_timer[i]=0;work_tries[i]=0;u->state=PLANNING;
  } else if(u->state==BLOCKED) {
   /* A busy door or a temporary wall never destroys the collection order. */
   worker_task[i]=task==TO_NODE?WAIT_NODE:WAIT_BASE;
   if(work_tries[i]<6)++work_tries[i];work_timer[i]=15+work_tries[i]*15;
   ++economy_retries;
  }
  else if(u->state==ARRIVED) {
   if(task==TO_NODE && work_adjacent(i,node_x[i],node_y[i])) {
    work_tries[i]=0;worker_task[i]=HARVEST;work_timer[i]=89;u->state=5;
    if(worker_kind[i]==2) {mine_phase[i]=1;mine_step[i]=0;}
    sprites_dirty=3;
   } else if(task==TO_BASE && work_adjacent(i,buildings[deposit_id[i]].x,buildings[deposit_id[i]].y)) {
    if(cargo_kind[i]==1)wood+=cargo[i];else gold+=cargo[i];
    hud_dirty=3;
    if(cargo[i])++deposits;
    work_tries[i]=0;cargo[i]=0;cargo_kind[i]=0;worker_task[i]=worker_kind[i]?WAIT_NODE:0;work_timer[i]=0;
   } else {worker_task[i]=task==TO_NODE?WAIT_NODE:WAIT_BASE;work_timer[i]=0;}
  }
 }
}
u8 repair_order(u8 id) __banked {
 u8 i;
 if(id>=building_count || building_owner[id] || buildings[id].state!=2 || !building_hp[id] || building_hp[id]>=structure_max_hp(id))return 0;
 for(i=0;i<unit_count;++i)if(WORKER(i) && (selection_mask&(1<<i))) {
  if(unit_build[i]!=NONE){buildings[unit_build[i]].worker=NONE;unit_build[i]=NONE;builder_working&=~(1<<i);}
  work_stop(i);worker_task[i]=0;work_timer[i]=0;work_route(i,buildings[id].x,buildings[id].y);repair_target[i]=id;repair_pending|=1<<i;repair_timer[i]=30;
 }
 hud_dirty=3;sprites_dirty=3;return 1;
}
void repair_tick(void) __banked {
 u8 i,old=repair_active;repair_active=repair_pending=0;
 for(i=0;i<unit_count;++i)if(repair_target[i]!=NONE) {
  u8 id=repair_target[i],max;
  if(!WORKER(i) || id>=building_count || building_owner[id] || buildings[id].state!=2 || !building_hp[id]){repair_target[i]=NONE;continue;}
  max=structure_max_hp(id);
  if(building_hp[id]>=max){repair_target[i]=NONE;hud_dirty=3;continue;}
  repair_pending|=1<<i;
  if(units[i].progress || mine_phase[i] || units[i].state==MOVING || requested[i] || job_unit==i)continue;
  if(!work_adjacent(i,buildings[id].x,buildings[id].y)) {
   if(!(sim_ticks&15)){work_route(i,buildings[id].x,buildings[id].y);repair_target[i]=id;}continue;
  }
  if(!wood)continue;
  repair_active|=1<<i;
  if(repair_timer[i]){--repair_timer[i];continue;}
  {u8 hp=max-building_hp[id];if(hp>4)hp=4;--wood;++repair_spent;repair_healed+=hp;building_hp[id]+=hp;repair_timer[i]=29;structure_dirty(id);}
 }
 if(old!=repair_active || (repair_active && !(sim_ticks&7)))sprites_dirty=3;
}
