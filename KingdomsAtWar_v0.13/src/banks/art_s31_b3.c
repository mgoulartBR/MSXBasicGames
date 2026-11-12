#include "../shared.h"
void nav_retry(u8 id) __banked {
 units[id].len=units[id].pos=0;
 if(worker_task[id] || unit_build[id]!=NONE || repair_target[id]!=NONE || combat_target[id]!=NONE) {
  requested[id]=0;units[id].state=BLOCKED;++replans;
 } else if(retry_left[id]) {
  --retry_left[id];retry_wait[id]=15;requested[id]=1;
  units[id].state=PLANNING;++replans;
 } else {requested[id]=0;units[id].state=BLOCKED;}
}
/* Multi-goal BFS: reserve a reachable entrance, not a Manhattan guess.
 * One 128-byte bitset is reused by the single budgeted navigation job. */
static u8 nav_goal_free(u8 id,u16 cell) {
 u8 j,x=cell&31,y=cell>>5;
 if(!free_at(x,y,id))return 0;
 for(j=0;j<unit_count;++j)if(j!=id && (units[j].state==PLANNING || units[j].state==MOVING) && units[j].tx==x && units[j].ty==y)return 0;
 return 1;
}
static u8 mine_waiter(void) {
 u8 d,j;
 for(d=0;d<unit_count;++d) {
  j=mine_turn+d;if(j>=unit_count)j-=unit_count;
  if(WORKER(j) && !cargo[j] && !mine_phase[j] && !units[j].progress &&
   (worker_task[j]==WAIT_NODE || (worker_task[j]==TO_NODE && units[j].state!=MOVING)) &&
   tile(node_x[j],node_y[j])==4)return j;
 }
 return NONE;
}
static void nav_target(u8 id,i16 x,i16 y) {
 u8 j;u16 cell;
 if(x<0||y<0||x>=32||y>=32)return;
 cell=((u16)y<<5)+x;
 if(parent[cell]!=255 && parent[cell]!=254)return;
 for(j=0;j<unit_count;++j)if(j!=id &&
  (units[j].state==PLANNING || units[j].state==MOVING) && units[j].tx==x && units[j].ty==y)return;
 nav_targets[cell>>3]|=1<<(cell&7);
}
static void nav_border(u8 id,u8 x,u8 y,u8 n) {
 u8 d;for(d=0;d<n;++d) {
  nav_target(id,x+d,(i16)y-1);nav_target(id,x+d,y+n);
  nav_target(id,(i16)x-1,y+d);nav_target(id,x+n,y+d);
 }
}
/* Bounded rows: 32 cells between input polls. No BIOS mapping changes.
 * parent=255 for grass/road (terrain 0/3), otherwise 253. */
void nav_prepare(void) __naked {
 __asm
 ld hl,#_terrain
 ld de,#_parent
 ld b,#32
nav_row094$:
 ld c,#32
nav_cell094$:
 ld a,(hl)
 or a
 jr z,nav_free094$
 cp #3
 jr z,nav_free094$
 ld a,#253
 jr nav_store094$
nav_free094$:
 ld a,#255
nav_store094$:
 ld (de),a
 inc hl
 inc de
 dec c
 jr nz,nav_cell094$
 push bc
 push de
 push hl
 call _poll_input
 pop hl
 pop de
 pop bc
 djnz nav_row094$
 ret
 __endasm;
}
/* The parent map is kept pristine between jobs. A job leaves dirt only on the cells it enqueued
 * (queue[0..qtail)) and on the unit cells it blocked, so those alone are restored; terrain changes
 * (nav_base_dirty) force the full rebuild. */
static u8 nav_clean;static u16 nav_marks[2*RN];static u8 nav_mark_n;
/* Z80: restore the pristine parent map for a list of cells (253 blocked, 255 free by terrain). */
static void nav_restore(const u16* list,u16 n) __naked {list;n;__asm
 ld a,d
 or a,e
 ret z
 ld c,e
 ld b,d
nr_loop:
 ld e,(hl)
 inc hl
 ld d,(hl)
 inc hl
 push hl
 ld hl,#_terrain
 add hl,de
 ld a,(hl)
 or a,a
 jr z,nr_free
 cp a,#3
 jr z,nr_free
 ld a,#253
 jr nr_st
nr_free:
 ld a,#255
nr_st:
 ld hl,#_parent
 add hl,de
 ld (hl),a
 pop hl
 dec bc
 ld a,b
 or a,c
 jr nz,nr_loop
 ret
 __endasm;}
static void nav_reset(void) {
 if(nav_base_dirty) {nav_prepare();nav_base_dirty=0;nav_clean=1;return;}
 if(nav_clean)return;
 nav_restore(queue,qtail);
 nav_restore(nav_marks,nav_mark_n);
 qtail=qhead=0;nav_mark_n=0;nav_clean=1;
}
void nav_begin(u8 id) __banked {
 u16 i,start;u8 j;
 Unit* u=&units[id];
 job_unit=id;requested[id]=0;nav_work_mode=0;
 nav_reset();
 nav_mark_n=0;
 for(j=0;j<RN;++j)if(j!=id && combat_alive(j)) {
  u16 c=((u16)units[j].y<<5)+units[j].x;parent[c]=253;nav_marks[nav_mark_n++]=c;
  if(units[j].progress){c=((u16)units[j].ny<<5)+units[j].nx;parent[c]=253;nav_marks[nav_mark_n++]=c;}
 }
 nav_clean=0;
 start=((u16)u->y<<5)+u->x;job_goal=((u16)u->ty<<5)+u->tx;
 parent[start]=254;queue[0]=start;qhead=0;qtail=1;
 __builtin_memset(nav_targets,0,128);
 if(combat_target[id]!=NONE) {
  u8 t=combat_target[id];nav_work_mode=4;
  u8 x,y,n,r=unit_range[unit_kind[id]];i16 xx,yy;
  if(t&128) {Building* b=&buildings[t&127];x=b->x;y=b->y;n=building_size(b->type);}else {x=units[t].x;y=units[t].y;n=1;}
  for(yy=(i16)y-r;yy<y+n+r;++yy)for(xx=(i16)x-r;xx<x+n+r;++xx)
   if(xx>=0 && yy>=0 && xx<32 && yy<32 && combat_position(id,xx,yy,t))nav_target(id,xx,yy);
 } else if(repair_target[id]!=NONE) {
  Building* b=&buildings[repair_target[id]];nav_work_mode=5;nav_border(id,b->x,b->y,building_size(b->type));
 } else if(worker_task[id]==TO_NODE) {
  u8 x=node_x[id],y=node_y[id];nav_work_mode=1;
  if(tile(x,y)==4) {
   j=mine_waiter();
   if(j==id) {nav_target(id,9,9);nav_target(id,10,9);}
   else if(j!=NONE && (nav_goal_free(j,297) || nav_goal_free(j,298)))work_timer[j]=0;
  }
  else nav_border(id,x,y,object_size(x,y));
 } else if(worker_task[id]==TO_BASE) {
  nav_work_mode=2;
  for(j=0;j<building_count;++j) {
   Building* b=&buildings[j];
   if(!building_owner[j] && b->state==2 && (!j || (b->type==3 && cargo_kind[id]==1)))nav_border(id,b->x,b->y,building_size(b->type));
  }
 } else if(unit_build[id]!=NONE) {
  Building* b=&buildings[unit_build[id]];nav_work_mode=3;nav_border(id,b->x,b->y,building_size(b->type));
 }
 if(nav_work_mode) {
  for(i=0;i<128;++i)if(nav_targets[i])return;
  nav_retry(id);job_unit=NONE;
 }
}
void nav_finish(u16 cell) __banked {
 u8 len=0,i,tmp;
 Unit* u=&units[job_unit];
 u8* path=routes[job_unit];
 if(nav_work_mode) {
  u8 x=cell&31,y=cell>>5;
  if(nav_work_mode==1 && tile(node_x[job_unit],node_y[job_unit])==4) {mine_turn=job_unit+1;if(mine_turn==unit_count)mine_turn=0;}
  if(u->tx!=x || u->ty!=y)++alternate_approaches;
  u->tx=x;u->ty=y;
  if(nav_work_mode==2)for(i=0;i<building_count;++i) {
   Building* b=&buildings[i];
   /* The unit has not arrived yet; test the destination perimeter. */
   if(!building_owner[i] && b->state==2 && (!i || (b->type==3 && cargo_kind[job_unit]==1))) {
    u8 n=building_size(b->type);
    if((x>=b->x && x<b->x+n && (y+1==b->y || y==b->y+n)) ||
       (y>=b->y && y<b->y+n && (x+1==b->x || x==b->x+n))) {deposit_id[job_unit]=i;break;}
   }
  }
 }
 while(parent[cell]!=254) {
  u8 d=parent[cell];if(!(len&15))poll_input();
  if(len==ROUTE_MAX) { u->state=BLOCKED; job_unit=NONE; return; }
  path[len++]=d;
  if(d==0) --cell; else if(d==1) cell-=32; else if(d==2) ++cell; else cell+=32;
 }
 for(i=0;i<len/2;++i) { tmp=path[i]; path[i]=path[len-1-i]; path[len-1-i]=tmp; }
 u->len=len; u->pos=u->progress=u->phase=0;
 unit_walk_dist[job_unit]=unit_walk_phase[job_unit]=0;u->state=len?MOVING:ARRIVED; job_unit=NONE;
}
/* BFS expansion in Z80. Neighbour order is east, south, west, north (route compatibility). */
static u8 nav_budget;static u16 nav_arg;
static void nav_expand_bc(void) __naked {
 __asm
 ld a,c
 and a,#31
 ld e,a
 ; east
 ld a,e
 cp a,#31
 jr z,ne_south
 push bc
 ld hl,#_parent
 add hl,bc
 inc hl
 ld a,(hl)
 cp a,#255
 jr nz,ne_east_end
 ld (hl),#0
 ld h,b
 ld l,c
 inc hl
 call ne_push
ne_east_end:
 pop bc
ne_south:
 ld a,b
 cp a,#3
 jr c,ne_south_ok
 jr nz,ne_west
 ld a,c
 cp a,#0xE0
 jr nc,ne_west
ne_south_ok:
 push bc
 ld hl,#_parent
 add hl,bc
 ld de,#32
 add hl,de
 ld a,(hl)
 cp a,#255
 jr nz,ne_south_end
 ld (hl),#1
 ld h,b
 ld l,c
 add hl,de
 call ne_push
ne_south_end:
 pop bc
ne_west:
 ld a,c
 and a,#31
 jr z,ne_north
 push bc
 ld hl,#_parent
 add hl,bc
 dec hl
 ld a,(hl)
 cp a,#255
 jr nz,ne_west_end
 ld (hl),#2
 ld h,b
 ld l,c
 dec hl
 call ne_push
ne_west_end:
 pop bc
ne_north:
 ld a,b
 or a,a
 jr nz,ne_north_ok
 ld a,c
 cp a,#32
 ret c
ne_north_ok:
 ld hl,#_parent
 add hl,bc
 ld de,#0xFFE0
 add hl,de
 ld a,(hl)
 cp a,#255
 ret nz
 ld (hl),#3
 ld h,b
 ld l,c
 add hl,de
 ; fallthrough push
ne_push:
 ex de,hl
 ld hl,(_qtail)
 inc hl
 ld (_qtail),hl
 dec hl
 add hl,hl
 ld bc,#_queue
 add hl,bc
 ld (hl),e
 inc hl
 ld (hl),d
 ret
 __endasm;
}
static void nav_expand_run(void) __naked {
 __asm
 ld bc,(_nav_arg)
 jp _nav_expand_bc
 __endasm;
}
/* Runs expansions until the budget ends, the queue empties (returns 0xFFFF) or a goal/target cell is dequeued
 * (returned; the caller decides and, if it rejects it, expands it with nav_expand_run). */
static u16 nav_bfs_run(void) __naked {
 __asm
nb_loop:
 ld a,(_nav_budget)
 or a,a
 jr z,nb_none
 dec a
 ld (_nav_budget),a
 ld hl,(_qhead)
 ld de,(_qtail)
 or a,a
 sbc hl,de
 jr nc,nb_none
 ld hl,(_qhead)
 push hl
 add hl,hl
 ld de,#_queue
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 pop hl
 inc hl
 ld (_qhead),hl
 ld hl,(_nav_expansions)
 inc hl
 ld (_nav_expansions),hl
 ld a,(_nav_work_mode)
 or a,a
 jr z,nb_goal
 ld h,b
 ld l,c
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 ld de,#_nav_targets
 add hl,de
 ld a,c
 and a,#7
 ld e,a
 ld d,#0
 push hl
 ld hl,#_fog_bit_table
 add hl,de
 ld e,(hl)
 pop hl
 ld a,(hl)
 and a,e
 jr z,nb_expand
 ld d,b
 ld e,c
 ret
nb_goal:
 ld hl,(_job_goal)
 or a,a
 sbc hl,bc
 jr nz,nb_expand
 ld d,b
 ld e,c
 ret
nb_expand:
 call _nav_expand_bc
 jr nb_loop
nb_none:
 ld de,#0xFFFF
 ret
 __endasm;
}
/* Z80: 1 when any unit has a pending order (requested[] is N bytes). */
static u8 req_any(void) __naked {
 __asm
 ld hl,#_requested
 ld b,#8
 xor a,a
ra_lp:
 or a,(hl)
 inc hl
 djnz ra_lp
 ret
 __endasm;
}
void navigation(void) __banked {
 u8 budget=mode?24:8,i,j;
 if(job_unit==NONE) {
  if(!req_any())return;
  for(j=0;j<unit_count;++j) {
   i=nav_cursor+j;if(i>=unit_count)i-=unit_count;
   if(requested[i] && !retry_wait[i] && !units[i].progress) {nav_begin(i);nav_cursor=i+1;if(nav_cursor==unit_count)nav_cursor=0;break;}
  }
  if(job_unit==NONE) return;
 }
 nav_budget=budget;
 for(;;) {
  u16 cell=nav_bfs_run();
  if(cell==0xFFFF)break;
  if(!nav_work_mode || nav_goal_free(job_unit,cell)) {nav_finish(cell);return;}
  nav_arg=cell;nav_expand_run();
 }
 if(qhead==qtail && job_unit!=NONE) {
  if(nav_work_mode==1 && tile(node_x[job_unit],node_y[job_unit])==4) {mine_turn=job_unit+1;if(mine_turn==unit_count)mine_turn=0;}
  nav_retry(job_unit);job_unit=NONE;
 }
}
void work_stop(u8 id) __banked {
 Unit* u=&units[id];
 if(WORKER(id) && mine_phase[id]) {mine_phase[id]=3;fog_refresh=1;sprites_dirty=3;}
 if(job_unit==id)job_unit=NONE;
 requested[id]=0;u->len=u->pos=0;
 u->state=u->progress?MOVING:ARRIVED;
}
void deposit_choose(u8 id) __banked {
 u8 j,best=0;u16 distance=65535;
 for(j=0;j<building_count;++j) {
  Building* b=&buildings[j];u16 d;
  if(building_owner[j] || b->state!=2 || (j && (b->type!=3 || cargo_kind[id]!=1)))continue;
  d=(units[id].x>b->x?units[id].x-b->x:b->x-units[id].x)+(units[id].y>b->y?units[id].y-b->y:b->y-units[id].y);
  if(d<distance) {distance=d;best=j;}
 }
 deposit_id[id]=best;
}
u8 work_route(u8 id,u8 x,u8 y) __banked {
 /* The navigation job chooses and reserves a reachable approach later. */
 x;y;order_one(id,units[id].x,units[id].y);return 1;
}
u8 work_adjacent(u8 id,u8 x,u8 y) __banked {
 Unit* u=&units[id];u8 n=object_size(x,y);
 return (u->x>=x && u->x<x+n && (u->y+1==y || u->y==y+n)) || (u->y>=y && u->y<y+n && (u->x+1==x || u->x==x+n));
}
void resource_empty(u8 x,u8 y) __banked {
 u8 n=object_size(x,y),dx,dy;
 if(tile(x,y)==4)return; /* Empty cave remains for workers to leave safely. */
 nav_base_dirty=1;
 for(dy=0;dy<n;++dy)for(dx=0;dx<n;++dx)terrain[((u16)(y+dy)<<5)+x+dx]=0;
 /* The oak crown extends one row above its root; mines occupy two rows. */
 for(dy=y?y-1:0;dy<=y+1 && dy<32;++dy)for(dx=x;dx<=x+1 && dx<32;++dx) {
  art_map[((u16)dy<<5)+dx]=art_index(dx,dy);video_tile(dx,dy);
 }
}
/* Phase is 0..2 plus a class speed of 4, 8 or 10. */
const u8 move_speed[]={0,0,0,1,1,1,2,2,2,3,3,3,4};
const u8 move_remainder[]={0,1,2,0,1,2,0,1,2,0,1,2,0};
/* Z80 fast path of the movement step: a unit already inside a cell that stays inside it this tick.
 * Anything else (cell completed, no progress yet) returns 0 before touching state, and the C code runs. */
static u8 sf_i,sf_rate;
static u8 sim_fast(void) __naked {
 __asm
 push ix
 ld a,(_sf_i)
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,#_units
 add hl,de
 push hl
 pop ix
 ld a,13(ix)
 or a,a
 jp z,sf_bail
 ld b,a
 ld a,(_sf_rate)
 add a,14(ix)
 ld e,a
 ld d,#0
 ld hl,#_move_speed
 add hl,de
 ld c,(hl)
 ld a,b
 add a,c
 cp a,#16
 jp nc,sf_bail
 ld hl,#_move_remainder
 add hl,de
 ld a,(hl)
 ld 14(ix),a
 ld a,c
 or a,a
 jp z,sf_done
 ld a,8(ix)
 cp a,4(ix)
 jp z,sf_y
 ld l,0(ix)
 ld h,1(ix)
 ld e,c
 ld d,#0
 jp c,sf_xneg
 add hl,de
 jp sf_xst
sf_xneg:
 or a,a
 sbc hl,de
sf_xst:
 ld 0(ix),l
 ld 1(ix),h
 jp sf_walk
sf_y:
 ld l,2(ix)
 ld h,3(ix)
 ld e,c
 ld d,#0
 ld a,9(ix)
 cp a,5(ix)
 jp c,sf_ysub
 jp z,sf_ysub
 add hl,de
 jp sf_yst
sf_ysub:
 or a,a
 sbc hl,de
sf_yst:
 ld 2(ix),l
 ld 3(ix),h
sf_walk:
 ld a,(_sf_i)
 ld e,a
 ld d,#0
 ld hl,#_unit_walk_dist
 add hl,de
 ld a,(hl)
 add a,c
 cp a,#10
 jp c,sf_wst
 sub a,#10
 ld (hl),a
 ld hl,#_unit_walk_phase
 add hl,de
 ld a,(hl)
 inc a
 and a,#3
 ld (hl),a
 jp sf_prog
sf_wst:
 ld (hl),a
sf_prog:
 ld a,13(ix)
 add a,c
 ld 13(ix),a
 ld a,#3
 ld (_sprites_dirty),a
sf_done:
 ld a,#1
 pop ix
 ret
sf_bail:
 xor a,a
 pop ix
 ret
 __endasm;
}
/* Z80: which per-tick subsystems have anything to do. bit0 worker with a task, bit1 building under
 * construction or training, bit2 completed tower. */
static u8 sim_flags(void) __naked {
 __asm
 ld c,#0
 ld a,(_unit_count)
 or a,a
 jr z,sfl_b
 ld b,a
 ld hl,#_worker_task
sfl_w:
 ld a,(hl)
 or a,a
 jr nz,sfl_wy
 inc hl
 djnz sfl_w
 jr sfl_b
sfl_wy:
 set 0,c
sfl_b:
 ld a,(_building_count)
 or a,a
 jr z,sfl_end
 ld b,a
 ld hl,#_buildings
sfl_bl:
 inc hl
 ld d,(hl)
 dec hl
 ld a,d
 cp a,#1
 jr nz,sfl_s2
 ; enemy-owned construction sites need no production work (the AI times them itself)
 push hl
 push de
 ld a,(_building_count)
 sub a,b
 ld e,a
 ld d,#0
 ld hl,#_building_owner
 add hl,de
 ld a,(hl)
 pop de
 pop hl
 or a,a
 ld a,d
 jr nz,sfl_s2
 set 1,c
sfl_s2:
 cp a,#2
 jr nz,sfl_tr
 ld a,(hl)
 cp a,#6
 jr nz,sfl_tr
 set 2,c
sfl_tr:
 push hl
 ld de,#7
 add hl,de
 ld a,(hl)
 pop hl
 or a,a
 jr z,sfl_nx
 set 1,c
sfl_nx:
 ld de,#12
 add hl,de
 djnz sfl_bl
sfl_end:
 ld a,c
 ret
 __endasm;
}
static u8 ss_i,ss_stage;
/* Z80: next unit at or after `start` that needs the full C movement code (255 = none). Dead units, units that
 * just walk on inside a tile (sim_fast) and idle ones are handled here. ss_stage=1: retry_wait/sim_fast done. */
static u8 sim_scan(u8 start) __naked {
 start;
 __asm
 ld (_ss_i),a
 push ix
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,#_units
 add hl,de
 push hl
 pop ix
ss_lp:
 ld a,(_ss_i)
 ld hl,#_unit_count
 cp a,(hl)
 jp nc,ss_none
 ld e,a
 ld d,#0
 ld hl,#_unit_hp
 add hl,de
 ld a,(hl)
 or a,a
 jp z,ss_next
 ld hl,#_unit_kind
 add hl,de
 ld a,(hl)
 or a,a
 jp z,ss_ret0
 ld c,a
 ld hl,#_retry_wait
 add hl,de
 ld a,(hl)
 or a,a
 jp z,ss_nr
 dec (hl)
ss_nr:
 ld a,13(ix)
 or a,a
 jp z,ss_noprog
 ld a,e
 ld (_sf_i),a
 ld a,c
 cp a,#4
 jp z,ss_k4
 cp a,#5
 jp z,ss_k5
 ld a,#8
 jp ss_r
ss_k4:
 ld a,#10
 jp ss_r
ss_k5:
 ld a,#4
ss_r:
 ld (_sf_rate),a
 call _sim_fast
 or a,a
 jp nz,ss_next
 jp ss_ret1
ss_noprog:
 ld hl,#_requested
 add hl,de
 ld a,(hl)
 or a,a
 jp nz,ss_next
 ld a,15(ix)
 cp a,#2
 jp nz,ss_next
 jp ss_ret1
ss_next:
 ld hl,#_ss_i
 inc (hl)
 ld de,#16
 add ix,de
 jp ss_lp
ss_ret0:
 xor a,a
 ld (_ss_stage),a
 ld a,(_ss_i)
 pop ix
 ret
ss_ret1:
 ld a,#1
 ld (_ss_stage),a
 ld a,(_ss_i)
 pop ix
 ret
ss_none:
 ld a,#255
 pop ix
 ret
 __endasm;
}
void simulate(void) __banked {
 u8 i,animate;Unit* u=units;
 ++sim_ticks;
 if(edge_keys) {if(edge_hold<6)++edge_hold;}else edge_hold=0;
 if(++second_phase==30) {second_phase=0;++game_seconds;hud_dirty=3;ai_second();}
 animate=!(sim_ticks&7);
 if(animate && builder_working)sprites_dirty=3;
 /* One pass: cave animation changes no logical positions or reservations. */
 for(i=sim_scan(0);i!=255;i=sim_scan(i+1)) {
  u8 kind,speed;
  u=&units[i];kind=unit_kind[i];
  if(!ss_stage) {
  if(!kind) {
   if(mine_phase[i]==1) {
    if(++mine_step[i]==8){mine_phase[i]=2;fog_refresh=1;}
    sprites_dirty=3;
   } else if(mine_phase[i]==3) {
    if(mine_step[i])--mine_step[i];
    if(!mine_step[i])mine_phase[i]=0;
    sprites_dirty=3;
   }
   if(animate && worker_task[i]==HARVEST && !mine_phase[i])sprites_dirty=3;
   if(mine_phase[i])continue;
  }
  if(retry_wait[i])--retry_wait[i];
  if(u->progress) {sf_i=i;sf_rate=kind==4?10:kind==5?4:8;if(sim_fast())continue;}
  }
  if(!u->progress && (requested[i] || u->state!=MOVING)) continue;
  u->phase+=kind==4?10:kind==5?4:8; speed=move_speed[u->phase]; u->phase=move_remainder[u->phase]; /* 80 px/s at 30 Hz */
  while(speed) {
   u8 step;
   if(!u->progress) {
    u8 d;
    if(u->pos>=u->len) { u->state=ARRIVED; break; }
    d=routes[i][u->pos]; u->nx=u->x; u->ny=u->y;
    if(d==0) ++u->nx; else if(d==1) ++u->ny; else if(d==2) --u->nx; else --u->ny;
    if(!free_at(u->nx,u->ny,i)) { nav_retry(i); break; }
   }
   step=16-u->progress;if(step>speed)step=speed;speed-=step;
   if(u->nx>u->x) u->px+=step; else if(u->nx<u->x) u->px-=step;
   else if(u->ny>u->y) u->py+=step; else u->py-=step;
   unit_walk_dist[i]+=step;
   if(unit_walk_dist[i]>=10) {unit_walk_dist[i]-=10;unit_walk_phase[i]=(unit_walk_phase[i]+1)&3;}
   u->progress+=step; sprites_dirty=3;
   if(u->progress==16) {
    u->x=u->nx; u->y=u->ny;fog_traverse(i); u->progress=0; ++u->pos;
    if(requested[i]) { u->state=PLANNING; break; }
    if(u->pos>=u->len) { u->state=ARRIVED; break; }
   }
  }
 }
 if(shot_active)projectile_tick();ai_move();if(!(sim_ticks&1) && (combat_active || !(sim_ticks&7)))combat_tick();navigation();{u8 pf=sim_flags();if(pf&1)economy();if((pf&2) || builder_working)production();if(repair_pending || repair_active)repair_tick();if(pf&4)tower_tick();}
}
