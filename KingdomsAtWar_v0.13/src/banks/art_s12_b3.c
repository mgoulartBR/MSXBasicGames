#include "../shared.h"
/* Fog of war v2 (maps up to 128x128): the explored flag of every cell lives in VRAM (MAP_FOGX); whether a cell is visible is
 * computed on demand from the list of sight sources (vs_*: player units and buildings, see fog_at in main.c). A sight update
 * only touches the ring of cells that entered or left a disc: they are marked explored, their remembered art is stored and
 * their layer B name is redrawn (p1_cell skips cells outside the rendered window). */
#define FS (N+BMAX)
static u8 fs_on[FS],fs_x[FS],fs_y[FS],fs_r[FS];   /* sources as computed by the last fog_update */
static u8 fo_on[FS],fo_x[FS],fo_y[FS],fo_r[FS];   /* sources as applied to the screen (fog_apply) */
static u8 fchg[FS],fog_cur;
static u8 in_disc(u8 on,u8 sx,u8 sy,u8 r,u8 x,u8 y) {
 u8 dx,dy;
 if(!on)return 0;
 dx=x>sx?x-sx:sx-x;if(dx>r)return 0;
 dy=y>sy?y-sy:sy-y;if(dy>r)return 0;
 return dx+dy<=r+(r>>1);
}
void fog_traverse(u8 i) __banked {i;fog_refresh=1;}
u8 fog_target(u8 t) __banked {
 u8 x,y,n,dx,dy;
 if(t==NONE)return 0;
 if(t&128) {u8 id=t&127;if(id>=building_count || !buildings[id].state || !building_hp[id])return 0;x=buildings[id].x;y=buildings[id].y;n=building_size(buildings[id].type);}
 else {if(!combat_alive(t))return 0;x=units[t].x;y=units[t].y;n=1;}
 for(dy=0;dy<n;++dy)for(dx=0;dx<n;++dx)if(fog_at(x+dx,y+dy)==2)return 1;
 return 0;
}
static void sources_build(void) {
 u8 i;
 for(i=0;i<N;++i) {
  u8 on=(i<unit_count && unit_hp[i] && !(WORKER(i) && mine_phase[i]==2));
  fs_on[i]=on;if(on){fs_x[i]=units[i].x;fs_y[i]=units[i].y;fs_r[i]=unit_kind[i]==4?6:5;}
 }
 for(i=0;i<BMAX;++i) {
  u8 s=N+i,on=(i<building_count && !building_owner[i] && buildings[i].state);
  fs_on[s]=on;
  if(on) {Building* b=&buildings[i];u8 n=building_size(b->type);fs_x[s]=b->x+n/2;fs_y[s]=b->y+n/2;fs_r[s]=b->state==1?3:b->type==6?7:b->type==0?6:4;}
 }
}
void fog_init(void) __banked {
 u8 i;
 fog_epoch=0;fog_refresh=1;fog_build_dirty=1;fog_updates=0;repair_active=repair_pending=0;tower_shots=repair_spent=repair_healed=0;
 map_clear_fog();
 for(i=0;i<FS;++i){fs_on[i]=fo_on[i]=fchg[i]=0;}
 vs_n=0;
 for(i=0;i<N;++i){repair_target[i]=NONE;repair_timer[i]=0;}
 for(i=0;i<BMAX;++i)tower_cooldown[i]=0;
 if(fog_update()) {u8 g=0;while(fog_apply())if(++g>40)break;}
}
/* Phase 1 of a sight update: recompute the sources and list what changed (returns 1 when fog_apply must follow). */
u8 fog_update(void) __banked {
 u8 i,any=0;
 fog_refresh=0;++fog_updates;
 sources_build();
 for(i=0;i<FS;++i)
  if(fs_on[i]!=fo_on[i] || (fs_on[i] && (fs_x[i]!=fo_x[i] || fs_y[i]!=fo_y[i] || fs_r[i]!=fo_r[i]))) {fchg[i]=1;any=1;} else fchg[i]=0;
 if(!any)return 0;
 /* the visible list used by fog_at */
 vs_n=0;
 for(i=0;i<FS;++i)if(fs_on[i]) {vs_x[vs_n]=fs_x[i];vs_y[vs_n]=fs_y[i];vs_r[vs_n]=fs_r[i];++vs_n;}
 fog_cur=0;
 return 1;
}
/* One changed source: its old and new discs. Returns when all rows of the union box were visited. */
static void apply_slot(u8 s) {
 u8 x,y,x0,x1,y0,y1,r0=fo_r[s],r1=fs_r[s];
 u8 on0=fo_on[s],on1=fs_on[s];
 /* bounding box of both discs, clipped to the map */
 x0=255;y0=255;x1=0;y1=0;
 if(on0){x0=fo_x[s]>r0?fo_x[s]-r0:0;x1=fo_x[s]+r0;y0=fo_y[s]>r0?fo_y[s]-r0:0;y1=fo_y[s]+r0;}
 if(on1){u8 a=fs_x[s]>r1?fs_x[s]-r1:0,b=fs_x[s]+r1,c=fs_y[s]>r1?fs_y[s]-r1:0,d=fs_y[s]+r1;
  if(a<x0)x0=a;if(b>x1)x1=b;if(c<y0)y0=c;if(d>y1)y1=d;}
 if(x1>=map_w)x1=map_w-1;if(y1>=map_h)y1=map_h-1;
 for(y=y0;y<=y1;++y)for(x=x0;x<=x1;++x) {
  u8 a=in_disc(on0,fo_x[s],fo_y[s],r0,x,y),b=in_disc(on1,fs_x[s],fs_y[s],r1,x,y);
  if(a==b)continue;
  if(b){fogx_set(x,y,1);fmem_set(x,y,art_tile(x,y));}
  p1_cell(x,y);
 }
 fo_on[s]=fs_on[s];fo_x[s]=fs_x[s];fo_y[s]=fs_y[s];fo_r[s]=fs_r[s];
}
/* Phase 2 (one frame later, repeated while it returns 1): the changed sources, two per call to bound the frame time. */
u8 fog_apply(void) __banked {
 u8 n=0,i;
 while(fog_cur<FS && n<2) {
  if(fchg[fog_cur]){apply_slot(fog_cur);fchg[fog_cur]=0;++n;}
  ++fog_cur;
 }
 if(fog_cur<FS)return 1;
 ++fog_epoch;sprites_dirty=3;
 if(enemy_selected!=NONE && !fog_target(enemy_selected)){enemy_selected=NONE;hud_dirty=3;}
 /* A health marker above a now hidden keep must not survive on a visible row. */
 if(video_enabled)for(i=0;i<building_count;++i)if(building_owner[i] && buildings[i].y){video_tile(buildings[i].x+1,buildings[i].y-1);video_tile(buildings[i].x+2,buildings[i].y-1);}
 return 0;
}
