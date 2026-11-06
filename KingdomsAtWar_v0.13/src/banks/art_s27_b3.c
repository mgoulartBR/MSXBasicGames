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
i16 dm_l[RN],dm_t[RN],dm_r[RN],dm_b[RN];u16 dm_bit[RN];u8 dm_n;
#define DM_HIT(L,T,R,B,k) ((R)>dm_l[k] && dm_r[k]>(L) && (B)>dm_t[k] && dm_b[k]>(T))
u16 damage_mask(u8 p,u16 mask,u8 flags) __banked {
 u8 i,j,k,n=0;u16 before,restore=mask;
 {const Rect* a=rect_now;for(j=0;j<RN;++j,++a)if(a->w){dm_l[n]=a->x;dm_r[n]=a->x+a->w;dm_t[n]=a->y;dm_b[n]=a->y+a->h;dm_bit[n]=render_bit[j];++n;}}
 dm_n=n;
 mask=arrow_damage(p,mask);if((flags&16) && n)mask=fog_damage(p,mask);
 for(i=0;i<strip_n;++i)for(k=0;k<n;++k)if(!(mask&dm_bit[k]) && DM_HIT(strip_l[i],strip_t[i],strip_r[i],strip_b[i],k))mask|=dm_bit[k];
 for(i=0;i<changed_count[p];++i) {
  i16 l=(i16)changed_x[p][i]*16,t=(i16)changed_y[p][i]*16,r=l+16,b=t+16;
  for(k=0;k<n;++k)if(!(mask&dm_bit[k]) && DM_HIT(l,t,r,b,k))mask|=dm_bit[k];
 }
 if(restore) {const Rect* o=rect_old[p];
  for(i=0;i<RN;++i,++o)if((restore&render_bit[i]) && o->w) {
   i16 l=o->x,t=o->y,r=o->x+o->w,b=o->y+o->h;
   for(k=0;k<n;++k)if(!(mask&dm_bit[k]) && DM_HIT(l,t,r,b,k))mask|=dm_bit[k];
  }
 }
  if(flags&2)for(i=0;i<RN;++i)if(
   (box_old[p] && box_overlap(rect_now+i,box_l[p],box_t[p],box_r[p],box_b[p])) ||
   ((flags&1) && box_overlap(rect_now+i,drag_l,drag_t,drag_r,drag_b)))mask|=render_bit[i];
  if(flags&8)for(i=0;i<RN;++i)if(
   (ghost_old[p] && box_overlap(rect_now+i,ghost_x[p],ghost_y[p],ghost_x[p]+ghost_size[p]-1,ghost_y[p]+ghost_size[p]-1)) ||
   ((flags&4) && box_overlap(rect_now+i,place_x*16,place_y*16,place_x*16+building_size(build_kind)*16-1,place_y*16+building_size(build_kind)*16-1)))mask|=render_bit[i];
  /* Rebuild the active list: box/ghost marking may have added inactive-free bits only. */
  do {before=mask;
   for(i=0;i<n;++i)if(mask&dm_bit[i]) {
    i16 l=dm_l[i],t=dm_t[i],r=dm_r[i],b=dm_b[i];
    for(k=0;k<n;++k)if(!(mask&dm_bit[k]) && DM_HIT(l,t,r,b,k))mask|=dm_bit[k];
   }
  }while(before!=mask);
 return mask;
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

