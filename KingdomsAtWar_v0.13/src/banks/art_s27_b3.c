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
/* One banked call for the P1 renderer: same rules as unit_art_frame + unit_position, with the worker tests
 * evaluated once. Returns 1 when the actor is drawn; the frame goes to unit_prep_f. */
u8 unit_prep_f;
static const u8 bit8[8]={1,2,4,8,16,32,64,128};
static u8 up_i,up_d,up_f,up_fl,up_sz;
/* Z80: unit_prep for everything that is not a worker (no mining/harvest/build poses). */
static u8 up_plain(u8 i) __naked {
 i;
 __asm
 ld (_up_i),a
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
 ld a,(_up_i)
 ld e,a
 ld d,#0
 ld hl,#_unit_facing
 add hl,de
 ld a,(hl)
 ld (_up_d),a
 ld a,13(ix)
 or a,a
 jp z,up_nop
 ld a,8(ix)
 cp a,4(ix)
 jp z,up_ny
 jp c,up_west
 ld a,#1
 jp up_setd
up_west:
 ld a,#3
 jp up_setd
up_ny:
 ld a,9(ix)
 cp a,5(ix)
 jp z,up_two
 jp c,up_two
 xor a,a
 jp up_setd
up_two:
 ld a,#2
up_setd:
 ld (_up_d),a
up_nop:
 ld hl,#_unit_facing
 add hl,de
 ld a,(_up_d)
 ld (hl),a
 add a,a
 add a,a
 ld (_up_f),a
 ld a,13(ix)
 or a,a
 jp nz,up_mv
 ld a,15(ix)
 cp a,#2
 jp nz,up_still
up_mv:
 ld hl,#_unit_walk_phase
 add hl,de
 ld a,(hl)
 jp up_addf
up_still:
 ld a,#1
up_addf:
 ld hl,#_up_f
 add a,(hl)
 ld (hl),a
 ld hl,#_attack_flash
 add hl,de
 ld a,(hl)
 ld (_up_fl),a
 or a,a
 jp z,up_nofl
 ld a,(_up_d)
 add a,a
 add a,a
 ld b,a
 ld a,(_up_fl)
 cp a,#5
 ld a,#1
 jp c,up_fa
 ld a,#3
up_fa:
 add a,b
 ld (_up_f),a
up_nofl:
 ld a,(_up_f)
 ld hl,#_unit_frame
 add hl,de
 ld (hl),a
 ld (_unit_prep_f),a
 ld hl,#_unit_hp
 add hl,de
 ld a,(hl)
 or a,a
 jp z,up_ret0
 ld a,(_up_i)
 cp a,#8
 jp nc,up_alive
 ld hl,#_unit_count
 cp a,(hl)
 jp nc,up_ret0
up_alive:
 ld a,10(ix)
 or a,a
 jp z,up_draw
 ld a,4(ix)
 ld l,5(ix)
 call _fog_at
 cp a,#2
 jp nz,up_ret0
up_draw:
 ld a,(_up_i)
 ld e,a
 ld d,#0
 ld hl,#_unit_kind
 add hl,de
 ld e,(hl)
 ld hl,#_kind_sizes
 add hl,de
 ld a,(hl)
 ld (_up_sz),a
 ld l,0(ix)
 ld h,1(ix)
 ld de,#8
 add hl,de
 ld de,(_cam_x)
 or a,a
 sbc hl,de
 ld a,(_up_sz)
 srl a
 ld e,a
 ld d,#0
 or a,a
 sbc hl,de
 ld (_unit_draw_x),hl
 ld l,2(ix)
 ld h,3(ix)
 ld de,#16
 add hl,de
 ld de,(_cam_y)
 or a,a
 sbc hl,de
 ld a,(_up_sz)
 ld e,a
 ld d,#0
 or a,a
 sbc hl,de
 ld (_unit_draw_y),hl
 ld a,(_up_fl)
 cp a,#5
 jp c,up_ret1
 ld a,(_up_d)
 or a,a
 jp z,up_rd0
 cp a,#2
 jp z,up_rd2
 ld hl,(_unit_draw_x)
 cp a,#1
 jp z,up_rx1
 dec hl
 dec hl
 jp up_rxs
up_rx1:
 inc hl
 inc hl
up_rxs:
 ld (_unit_draw_x),hl
 jp up_ret1
up_rd0:
 ld hl,(_unit_draw_y)
 inc hl
 inc hl
 ld (_unit_draw_y),hl
 jp up_ret1
up_rd2:
 ld hl,(_unit_draw_y)
 dec hl
 dec hl
 ld (_unit_draw_y),hl
up_ret1:
 ld a,#1
 jp up_end
up_ret0:
 xor a,a
up_end:
 pop ix
 ret
 __endasm;
}
u8 unit_prep(u8 i) __banked {
 Unit* u=&units[i];u8 kind=unit_kind[i],d=unit_facing[i],f,mp=0,harvest=0,hmask=0,fl=attack_flash[i],w=kind==0 && i<unit_count && unit_hp[i];
 if(!w)return up_plain(i);
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

