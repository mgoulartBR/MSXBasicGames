#include "../shared.h"
void init_world(void) __banked
{
 u8 x,y,i;
 nav_base_dirty=1;unit_count=4;building_count=1;building_selected=placement_mode=builder_working=research_level=0;build_kind=1;edge_hold=edge_keys=mouse_active=0;
 for(i=0;i<BMAX;++i)buildings[i].state=0;
 buildings[0].type=0;buildings[0].state=2;buildings[0].x=BASE_X;buildings[0].y=BASE_Y;buildings[0].worker=NONE;buildings[0].training=0;
 buildings[0].rallyx=6;buildings[0].rallyy=10;
 for(y=0;y<32;++y) for(x=0;x<32;++x) {
  u8 t=0;
  if(((((x>>1)*11+(y>>1)*7)%17)<3)) t=1;
  if(x==12 || x==13 || (y>13 && y<17 && x>13)) t=2;

  if(y<=5 && x<=11) t=0;
  terrain[((u16)y<<5)+x]=t;
 }
 for(y=0;y<24;++y)for(x=0;x<12;++x)terrain[((u16)y<<5)+x]=0;
 for(y=13;y<=23;y+=2) {terrain[((u16)y<<5)]=1;terrain[((u16)y<<5)+10]=1;}
 for(y=BASE_Y;y<BASE_Y+4;++y)for(x=BASE_X;x<BASE_X+4;++x)terrain[((u16)y<<5)+x]=5;
 for(y=7;y<9;++y)for(x=9;x<11;++x)terrain[((u16)y<<5)+x]=4;
 terrain[(7<<5)+6]=terrain[(8<<5)+6]=1;
 for(y=0;y<32;++y)for(x=0;x<32;++x)if(terrain[((u16)y<<5)+x]==1) {
  if(x==31 || y==0)terrain[((u16)y<<5)+x]=0;
  else {
   u8 a=terrain[((u16)y<<5)+x+1],b=terrain[((u16)(y-1)<<5)+x],c=terrain[((u16)(y-1)<<5)+x+1];
   if(a>=2 || b>=2 || c>=2)terrain[((u16)y<<5)+x]=0;
  }
 }
 for(x=0;x<N;++x) {unit_kind[x]=x<2?0:1;unit_build[x]=NONE;deposit_id[x]=0;mine_phase[x]=mine_step[x]=0;worker_task[x]=cargo[x]=cargo_kind[x]=work_timer[x]=worker_kind[x]=0;work_tries[x]=0;}
 for(i=0;i<unit_count;++i) {
  Unit* u=&units[i];
  u->x=u->tx=u->nx=i<2?3+i*3:8+(i-2)*2; u->y=u->ty=u->ny=i<2?3:4;
  u->px=(u16)u->x*16; u->py=(u16)u->y*16;
  unit_facing[i]=unit_walk_dist[i]=unit_walk_phase[i]=0;unit_frame[i]=1;
  u->owner=0; u->len=u->pos=u->progress=u->phase=u->state=0;
 }
 {u16 cell;for(cell=0;cell<1024;++cell)stock[cell]=terrain[cell]==1?32:cell==233?240:0;}

 combat_init();
 {u16 cell;for(cell=0;cell<1024;++cell)art_map[cell]=art_index(cell&31,cell>>5);}
 wood=gold=pickups=deposits=0;
 for(i=0;i<unit_count;++i) requested[i]=0;
}

u8 art_index(u8 x,u8 y) __banked {
 u8 t=terrain[((u16)y<<5)+x],v=(x*13+y*7)&7,mask=0;
 if(t>=10) {
  Building* b=&buildings[t-10];
  return b->state==1?185+(y-b->y)*4+x-b->x:build_base[b->type]+(y-b->y)*building_size(b->type)+x-b->x;
 }
 if(buildings[0].state && x>=BASE_X && x<BASE_X+4 && y>=BASE_Y && y<BASE_Y+4)return build_base[0]+(y-BASE_Y)*4+x-BASE_X;
 if(terrain[(7<<5)+9]==4 && x>=9 && x<11 && y>=7 && y<9)return 25+(y-7)*2+x-9;
 if(t==2) {
  if(x && terrain[((u16)y<<5)+x-1]!=2)return 10;
  if(x<31 && terrain[((u16)y<<5)+x+1]!=2)return 11;
  if(y && terrain[((u16)(y-1)<<5)+x]!=2)return 12;
  if(y<31 && terrain[((u16)(y+1)<<5)+x]!=2)return 13;
  return 2;
 }
 if(t==3)return 3;
 if(x && terrain[((u16)y<<5)+x-1]==1)mask|=1;
 if(t==1)mask|=2;
 if(x && y<31 && terrain[((u16)(y+1)<<5)+x-1]==1)mask|=4;
 if(y<31 && terrain[((u16)(y+1)<<5)+x]==1)mask|=8;
 if(mask)return 29+((x+y)&1)*15+mask-1;
 return v<4?0:v==4?6:v==5?7:v==6?14:15;
}

void camera_update(u16 dt) __banked {
 i16 x=cam_x,y=cam_y;
 u16 d=dt*2;
 if(!cam_keys) return;
 if(d>8)d=8;
 if(cam_keys&1)x-=d; if(cam_keys&2)x+=d;
 if(cam_keys&4)y-=d; if(cam_keys&8)y+=d;
 if(x<0)x=0; if(x>256)x=256; if(y<0)y=0; if(y>352)y=352;
 if(x==cam_x && y==cam_y)return;
 if(!video_camera_ready(y))return;
 cam_x=x;cam_y=y;
 if(placement_mode) {place_x=(cam_x+mouse_x)>>4;place_y=(cam_y+mouse_y)>>4;place_valid=mouse_y<160 && placement_valid(place_x,place_y);}
 video_camera();++scroll_copies;camera_pixels+=d;
 if(d>max_camera_step)max_camera_step=d;
 sprites_dirty|=1;
}

u8 object_size(u8 x,u8 y) __banked {
 u8 t=tile(x,y);return t==5?4:t>=10?building_size(buildings[t-10].type):t==4?2:1;
}

/* Keep native fixed ROM below the ASCII8 switched window. */
#ifdef KAW_P1
/* One banked call for the P1 renderer: same rules as unit_art_frame + unit_position, with the worker tests
 * evaluated once. Returns 1 when the actor is drawn; the frame goes to unit_prep_f. */
u8 unit_prep_f;
static const u8 bit8[8]={1,2,4,8,16,32,64,128};
u8 unit_prep(u8 i) __banked {
 Unit* u=&units[i];u8 kind=unit_kind[i],d=unit_facing[i],f,mp=0,harvest=0,hmask=0,fl=attack_flash[i],w=kind==0 && i<unit_count && unit_hp[i];
 if(w) {
  mp=mine_phase[i];
  if(i<8)hmask=(builder_working|repair_active)&bit8[i];
  harvest=worker_task[i]==HARVEST || hmask;
 }
 if(mp)d=mp==3?0:2;
 else if(u->progress) {if(u->nx>u->x)d=1;else if(u->nx<u->x)d=3;else if(u->ny>u->y)d=0;else d=2;}
 if(harvest) {
  u8 nx,ny;
  if(hmask) {const Building* b=&buildings[(repair_active&bit8[i])?repair_target[i]:unit_build[i]];nx=b->x;ny=b->y;}
  else {nx=node_x[i];ny=node_y[i];}
  if(nx>u->x)d=1;else if(nx<u->x)d=3;else if(ny>u->y)d=0;else d=2;
 }
 unit_facing[i]=d;
 f=d*4+((u->progress || u->state==MOVING)?unit_walk_phase[i]:1);
 if(mp)f=d*4+((mine_step[i]>>1)&3);
 else if(harvest)f=((sim_ticks&8)?16:20)+d;
 if(fl)f=d*4+(fl>4?3:1);
 unit_frame[i]=f;unit_prep_f=f;
 if(!combat_alive(i) || (u->owner && fog_at(u->x,u->y)!=2))return 0;
 {u8 sz=kind_sizes[kind];
  unit_draw_x=(i16)u->px+8-cam_x-(sz>>1);unit_draw_y=(i16)u->py+16-cam_y-sz;}
 if(fl>4) {if(d==0)unit_draw_y+=2;else if(d==2)unit_draw_y-=2;else unit_draw_x+=d==1?2:-2;}
 if(mp) {
  if(mp==2)return 0;
  unit_draw_x+=u->x==10?-(i16)mine_step[i]:mine_step[i];
  unit_draw_y-=(u16)mine_step[i]*2;
 }
 return 1;
}
#endif
u8 unit_art_frame(u8 i) __banked {
 Unit* u=&units[i];u8 d=unit_facing[i],f;
 if(WORKER(i) && mine_phase[i])d=mine_phase[i]==3?0:2;
 else if(u->progress) {if(u->nx>u->x)d=1;else if(u->nx<u->x)d=3;else if(u->ny>u->y)d=0;else d=2;}
 if(WORKER(i) && (worker_task[i]==HARVEST || ((builder_working|repair_active)&(1<<i)))) {
  u8 nx=((builder_working|repair_active)&(1<<i))?buildings[repair_active&(1<<i)?repair_target[i]:unit_build[i]].x:node_x[i],ny=((builder_working|repair_active)&(1<<i))?buildings[repair_active&(1<<i)?repair_target[i]:unit_build[i]].y:node_y[i];
  if(nx>u->x)d=1;else if(nx<u->x)d=3;else if(ny>u->y)d=0;else d=2;
 }
 unit_facing[i]=d;
 f=d*4+((u->progress || u->state==MOVING)?unit_walk_phase[i]:1);
 if(WORKER(i) && mine_phase[i])f=d*4+((mine_step[i]>>1)&3);
 else if(WORKER(i) && (worker_task[i]==HARVEST || ((builder_working|repair_active)&(1<<i))))f=((sim_ticks&8)?16:20)+d;
 if(attack_flash[i])f=d*4+(attack_flash[i]>4?3:1);
 unit_frame[i]=f;return f;
}


static u8 box_overlap(const Rect* a,u16 l,u16 t,u16 r,u16 b) {
 return a->w && a->x<=(i16)r && a->x+a->w>(i16)l && a->y<=(i16)b && a->y+a->h>(i16)t &&
 !(a->x>(i16)l && a->x+a->w<=(i16)r && a->y>(i16)t && a->y+a->h<=(i16)b);
}

/* Dirty actors expand the redraw set; only initial actors are erased. */
/* Active rectangles are cached once as edges; the O(n^2) closure then touches only visible actors. */
/* Damage marking in Z80. dm[] = edges of the visible actors; queries (strips, fog box, changed tiles, old
 * rectangles) OR into the mask the actors they touch, then the closure spreads it over overlapping actors. */
DMEnt dm[RN];u8 dm_n;
static i16 dq_l[4],dq_t[4],dq_r[4],dq_b[4];u8 dq_n;
static const u8* dq_cx;static const u8* dq_cy;static u8 dq_cn;static const Rect* dq_old;static u16 dq_restore;
static i16 qq_l,qq_t,qq_r,qq_b;static u16 qq_mask,qq_before;static u16 dmb;static u8 dbw,dbh;
static void dm_build(void) __naked {
 __asm
 push ix
 ld ix,#_dm
 ld hl,#_rect_now
 ld de,#1
 ld (_dmb),de
 ld b,#10
 xor a,a
 ld (_dm_n),a
db_loop:
 push bc
 ld e,(hl)
 inc hl
 ld d,(hl)
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 inc hl
 ld a,(hl)
 inc hl
 ld (_dbw),a
 ld a,(hl)
 inc hl
 ld (_dbh),a
 ld a,(_dbw)
 or a,a
 jr z,db_skip
 push hl
 ld 0(ix),e
 ld 1(ix),d
 ld 2(ix),c
 ld 3(ix),b
 ld a,(_dbw)
 ld l,a
 ld h,#0
 add hl,de
 ld 4(ix),l
 ld 5(ix),h
 ld a,(_dbh)
 ld l,a
 ld h,#0
 add hl,bc
 ld 6(ix),l
 ld 7(ix),h
 ld hl,(_dmb)
 ld 8(ix),l
 ld 9(ix),h
 ld de,#10
 add ix,de
 ld a,(_dm_n)
 inc a
 ld (_dm_n),a
 pop hl
db_skip:
 push hl
 ld hl,(_dmb)
 add hl,hl
 ld (_dmb),hl
 pop hl
 pop bc
 djnz db_loop
 pop ix
 ret
 __endasm;
}
/* mark: qq_* rectangle vs every actor not yet in qq_mask. */
static void dm_mark(void) __naked {
 __asm
 push ix
 ld ix,#_dm
 ld a,(_dm_n)
 or a,a
 jr z,dmm_end
 ld b,a
dmm_k:
 ld hl,(_qq_mask)
 ld e,8(ix)
 ld d,9(ix)
 ld a,l
 and a,e
 jr nz,dmm_next
 ld a,h
 and a,d
 jr nz,dmm_next
 ld l,0(ix)
 ld h,1(ix)
 ld de,(_qq_r)
 or a,a
 sbc hl,de
 bit 7,h
 jr z,dmm_next
 ld hl,(_qq_l)
 ld e,4(ix)
 ld d,5(ix)
 or a,a
 sbc hl,de
 bit 7,h
 jr z,dmm_next
 ld l,2(ix)
 ld h,3(ix)
 ld de,(_qq_b)
 or a,a
 sbc hl,de
 bit 7,h
 jr z,dmm_next
 ld hl,(_qq_t)
 ld e,6(ix)
 ld d,7(ix)
 or a,a
 sbc hl,de
 bit 7,h
 jr z,dmm_next
 ld hl,(_qq_mask)
 ld e,8(ix)
 ld d,9(ix)
 ld a,l
 or a,e
 ld l,a
 ld a,h
 or a,d
 ld h,a
 ld (_qq_mask),hl
dmm_next:
 ld de,#10
 add ix,de
 djnz dmm_k
dmm_end:
 pop ix
 ret
 __endasm;
}
/* All queries: dq[] list, changed tiles (x*16,y*16,16x16), old rectangles of the actors in dq_restore. */
static void dm_queries(void) __naked {
 __asm
 ; ---- dq list
 ld a,(_dq_n)
 or a,a
 jr z,dq_tiles
 ld b,a
 ld hl,#_dq_l
dq_li:
 push bc
 push hl
 ; entries are stored as 4 parallel arrays of 4: l[i], t[i], r[i], b[i]
 ld de,#0
 ld a,(_dq_n)
 sub a,b
 ld e,a                ; index i = n - b
 ld hl,#_dq_l
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld (_qq_l),bc
 ld hl,#_dq_t
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld (_qq_t),bc
 ld hl,#_dq_r
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld (_qq_r),bc
 ld hl,#_dq_b
 add hl,de
 add hl,de
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld (_qq_b),bc
 call _dm_mark
 pop hl
 pop bc
 djnz dq_li
dq_tiles:
 ; ---- changed tiles
 ld a,(_dq_cn)
 or a,a
 jr z,dq_old_
 ld b,a
 ld hl,(_dq_cx)
 ld de,(_dq_cy)
dq_ti:
 push bc
 push hl
 push de
 ld a,(hl)             ; x
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld (_qq_l),hl
 ld bc,#16
 add hl,bc
 ld (_qq_r),hl
 pop de
 push de
 ld a,(de)             ; y
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld (_qq_t),hl
 ld bc,#16
 add hl,bc
 ld (_qq_b),hl
 call _dm_mark
 pop de
 pop hl
 pop bc
 inc hl
 inc de
 djnz dq_ti
dq_old_:
 ; ---- old rectangles of restored actors
 ld hl,(_dq_restore)
 ld a,h
 or a,l
 ret z
 ld hl,(_dq_old)
 ld de,#1
 ld (_dmb),de
 ld b,#10
do_loop:
 push bc
 push hl
 ld de,(_dmb)
 ld hl,(_dq_restore)
 ld a,l
 and a,e
 ld c,a
 ld a,h
 and a,d
 or a,c
 pop hl
 jr z,do_next
 push hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 inc hl
 ld a,(hl)             ; w
 inc hl
 or a,a
 jr z,do_skip
 ld (_dbw),a
 ld a,(hl)             ; h
 ld (_dbh),a
 ld (_qq_l),de
 ld (_qq_t),bc
 ld a,(_dbw)
 ld l,a
 ld h,#0
 add hl,de
 ld (_qq_r),hl
 ld a,(_dbh)
 ld l,a
 ld h,#0
 add hl,bc
 ld (_qq_b),hl
 call _dm_mark
do_skip:
 pop hl
do_next:
 ld de,#6
 add hl,de
 push hl
 ld hl,(_dmb)
 add hl,hl
 ld (_dmb),hl
 pop hl
 pop bc
 djnz do_loop
 ret
 __endasm;
}
/* Closure: every marked actor marks the actors it overlaps, until nothing changes. */
static void dm_closure(void) __naked {
 __asm
dc_again:
 ld hl,(_qq_mask)
 ld (_qq_before),hl
 push ix
 ld ix,#_dm
 ld a,(_dm_n)
 or a,a
 jr z,dc_done
 ld b,a
dc_k:
 push bc
 ld hl,(_qq_mask)
 ld e,8(ix)
 ld d,9(ix)
 ld a,l
 and a,e
 ld c,a
 ld a,h
 and a,d
 or a,c
 jr z,dc_next
 ld l,0(ix)
 ld h,1(ix)
 ld (_qq_l),hl
 ld l,2(ix)
 ld h,3(ix)
 ld (_qq_t),hl
 ld l,4(ix)
 ld h,5(ix)
 ld (_qq_r),hl
 ld l,6(ix)
 ld h,7(ix)
 ld (_qq_b),hl
 push ix
 call _dm_mark
 pop ix
dc_next:
 pop bc
 ld de,#10
 add ix,de
 djnz dc_k
dc_done:
 pop ix
 ld hl,(_qq_mask)
 ld de,(_qq_before)
 or a,a
 sbc hl,de
 jr nz,dc_again
 ret
 __endasm;
}
u16 damage_mask(u8 p,u16 mask,u8 flags) __banked {
 u8 i;u16 restore=mask;
 dm_build();
 mask=arrow_damage(p,mask);
 dq_n=0;
 for(i=0;i<strip_n;++i) {dq_l[dq_n]=strip_l[i];dq_t[dq_n]=strip_t[i];dq_r[dq_n]=strip_r[i];dq_b[dq_n]=strip_b[i];++dq_n;}
#ifdef KAW_V9990
 if((flags&16) && dm_n)dq_n+=fog_query(p,dq_l+dq_n,dq_t+dq_n,dq_r+dq_n,dq_b+dq_n);
#else
 if((flags&16) && dm_n)mask=fog_damage(p,mask);
#endif
 dq_cx=changed_x[p];dq_cy=changed_y[p];dq_cn=changed_count[p];dq_old=rect_old[p];dq_restore=restore;
 qq_mask=mask;dm_queries();mask=qq_mask;
  if(flags&2)for(i=0;i<RN;++i)if(
   (box_old[p] && box_overlap(rect_now+i,box_l[p],box_t[p],box_r[p],box_b[p])) ||
   ((flags&1) && box_overlap(rect_now+i,drag_l,drag_t,drag_r,drag_b)))mask|=render_bit[i];
  if(flags&8)for(i=0;i<RN;++i)if(
   (ghost_old[p] && box_overlap(rect_now+i,ghost_x[p],ghost_y[p],ghost_x[p]+ghost_size[p]-1,ghost_y[p]+ghost_size[p]-1)) ||
   ((flags&4) && box_overlap(rect_now+i,place_x*16,place_y*16,place_x*16+building_size(build_kind)*16-1,place_y*16+building_size(build_kind)*16-1)))mask|=render_bit[i];
 qq_mask=mask;dm_closure();
 return qq_mask;
}

/* Render entry/exit at the cave mouth; logical doorway stays reserved. */
u8 unit_position(u8 i) __banked {
 if(!combat_alive(i) || (units[i].owner && fog_at(units[i].x,units[i].y)!=2))return 0;
 unit_draw_x=(i16)units[i].px+8-cam_x-(kind_sizes[unit_kind[i]]>>1);unit_draw_y=(i16)units[i].py+16-cam_y-kind_sizes[unit_kind[i]];
 if(attack_flash[i]>4) {if(unit_facing[i]==0)unit_draw_y+=2;else if(unit_facing[i]==2)unit_draw_y-=2;else unit_draw_x+=unit_facing[i]==1?2:-2;}
 if(WORKER(i) && mine_phase[i]) {
  if(mine_phase[i]==2)return 0;
  unit_draw_x+=units[i].x==10?-(i16)mine_step[i]:mine_step[i];
  unit_draw_y-=(u16)mine_step[i]*2;
 }
 return 1;
}

