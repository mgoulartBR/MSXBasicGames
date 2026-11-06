#include "../shared.h"
/* Octagonal sight on a 32x32 grid. Only sources changing cell/state rebuild it. */
static void sight(u8 x,u8 y,u8 r) {fog_stamp(x,y,r);}
void fog_traverse(u8 i) __banked {
 if(i>=unit_count || !unit_hp[i])return;
 fog_refresh=1;fog_dest=fog_trail;sight(units[i].x,units[i].y,unit_kind[i]==4?6:5);
}

u8 fog_target(u8 t) __banked {
 u8 x,y,n,dx,dy;
 if(t==NONE)return 0;
 if(t&128) {u8 id=t&127;if(id>=building_count || !buildings[id].state || !building_hp[id])return 0;x=buildings[id].x;y=buildings[id].y;n=building_size(buildings[id].type);}
 else {if(!combat_alive(t))return 0;x=units[t].x;y=units[t].y;n=1;}
 for(dy=0;dy<n;++dy)for(dx=0;dx<n;++dx)if(fog_at(x+dx,y+dy)==2)return 1;
 return 0;
}
void fog_init(void) __banked {
 u16 i;fog_dest=fog_next;fog_epoch=0;fog_pending[0]=fog_pending[1]=0;fog_checked_epoch[0]=fog_checked_epoch[1]=65535;fog_checked_x[0]=fog_checked_x[1]=fog_checked_y[0]=fog_checked_y[1]=65535;fog_refresh=1;fog_build_dirty=1;fog_updates=0;repair_active=repair_pending=0;tower_shots=repair_spent=repair_healed=0;
 for(i=0;i<128;++i)fog_trail[i]=fog_seen[i]=fog_visible[i]=fog_changed[0][i]=fog_changed[1][i]=0;
 for(i=0;i<1024;++i)fog_memory[i]=0;
 for(i=0;i<N;++i){fog_sources[i]=65535;repair_target[i]=NONE;repair_timer[i]=0;}
 for(i=0;i<BMAX;++i)tower_cooldown[i]=0;
 fog_update();
 for(i=0;i<128;++i)fog_changed[0][i]=fog_changed[1][i]=0;
}
static u8 fog_merge_acc,fog_work_idx[128],fog_work_n;
/* Single pass with a fast skip: a byte whose next/visible/trail are all zero changes nothing
 * (need/delta are cleared up front), which is the common case on a mostly unexplored map. */
u8 fog_merge(void) __banked __naked {
#ifdef __SDCC
 __asm
 push ix
 push iy
 ld hl,#_fog_need
 ld (hl),#0
 ld de,#_fog_need+1
 ld bc,#127
 ldir
 ld hl,#_fog_delta
 ld (hl),#0
 ld de,#_fog_delta+1
 ld bc,#127
 ldir
 xor a
 ld (_fog_merge_acc),a
 ld (_fog_work_n),a
 ld hl,#_fog_next
 ld de,#_fog_visible
 ld ix,#_fog_trail
 ld iy,#_fog_seen
 ld c,#0
fm_loop:
 ld a,(hl)
 or 0(ix)
 ld b,a
 ld a,(de)
 or b
 jr nz,fm_work
fm_step:
 inc hl
 inc de
 inc ix
 inc iy
 inc c
 ld a,c
 cp #128
 jr nz,fm_loop
 ld a,(_fog_merge_acc)
 pop iy
 pop ix
 ret
fm_work:
 push hl
 push de
 ld b,(hl)
 ld a,(de)
 ld l,a
 ld a,0(iy)
 cpl
 and 0(ix)
 ld h,a
 ld a,l
 xor b
 or h
 ld d,a
 ld a,l
 cpl
 and b
 or h
 ld e,a
 ld a,b
 or 0(ix)
 or 0(iy)
 ld 0(iy),a
 xor a
 ld 0(ix),a
 ld l,b
 ld b,#0
 push hl
 ld hl,#_fog_need
 add hl,bc
 ld (hl),e
 ld hl,#_fog_delta
 add hl,bc
 ld (hl),d
 ld hl,#_fog_changed
 add hl,bc
 ld a,(hl)
 or d
 ld (hl),a
 ld hl,#_fog_changed+128
 add hl,bc
 ld a,(hl)
 or d
 ld (hl),a
 ld a,(_fog_merge_acc)
 or d
 ld (_fog_merge_acc),a
 ld a,(_fog_work_n)
 ld hl,#_fog_work_idx
 ld e,a
 ld d,#0
 add hl,de
 ld (hl),c
 inc a
 ld (_fog_work_n),a
 pop hl
 pop de
 ld a,l
 ld (de),a
 pop hl
 jr fm_step
 __endasm;
#else
 u8 i,changed=0;
 for(i=0;i<128;++i) {
  u8 v=fog_next[i],fresh=fog_trail[i]&~fog_seen[i],d=(fog_visible[i]^v)|fresh;
  fog_need[i]=(v&~fog_visible[i])|fresh;fog_delta[i]=d;fog_visible[i]=v;fog_seen[i]|=v|fog_trail[i];fog_trail[i]=0;
  fog_changed[0][i]|=d;fog_changed[1][i]|=d;changed|=d;
 }
 return changed;
#endif
}
void fog_update(void) __banked {
 u16 c;u8 i,j,changed=0;
 for(i=0;i<N;++i) {
  u16 source=(i<unit_count && unit_hp[i] && !(WORKER(i) && mine_phase[i]==2))?((u16)units[i].y<<5)+units[i].x:65535;
  if(source!=fog_sources[i]){fog_sources[i]=source;fog_refresh=1;}
 }
 if(!fog_refresh)return;fog_refresh=0;++fog_updates;
 if(fog_build_dirty) {
 fog_build_dirty=0;fog_dest=fog_static;__builtin_memset(fog_static,0,128);
 for(i=0;i<building_count;++i)if(!building_owner[i] && buildings[i].state) {
  Building* b=&buildings[i];u8 n=building_size(b->type);
  sight(b->x+n/2,b->y+n/2,b->state==1?3:b->type==6?7:b->type==0?6:4);
 }
 }
 __builtin_memcpy(fog_next,fog_static,128);fog_dest=fog_next;
 for(i=0;i<unit_count;++i)if(fog_sources[i]!=65535)sight(units[i].x,units[i].y,unit_kind[i]==4?6:5);
 poll_input();changed=fog_merge();poll_input();
 {u8 k;
 for(k=0;k<fog_work_n;++k) {
  u8 need;i=fog_work_idx[k];need=fog_need[i];
  if(need)for(j=0;j<8;++j)if(need&fog_bit_table[j]) {
   c=((u16)i<<3)+j;
   if(fog_memory[c]!=art_map[c]) {fog_memory[c]=art_map[c];if(video_enabled)video_known_tile(c&31,c>>5);}
  }
 }
 #ifdef KAW_V9990
 /* Bake the new fog look into the resident world. Pass 1: tiles of newly visible / newly seen cells.
  * Pass 2: remembered cells get the dither, one copy per horizontal run (visible->remembered cells
  * already hold the right tile underneath). Only bytes listed by fog_merge are visited. */
 if(changed && video_enabled) {
  u8 r,run=0,start=0,rrow=0,previ=255,kk,m;
  for(k=0;k<fog_work_n;++k) {u8 nd;i=fog_work_idx[k];nd=fog_need[i];if(nd)for(j=0;j<8;++j)if(nd&fog_bit_table[j]){c=((u16)i<<3)+j;video_world_tile(c&31,c>>5);}}
  for(k=0;k<fog_work_n;++k) {
   u8 d;i=fog_work_idx[k];r=i>>2;kk=i&3;d=fog_delta[i];
   if(run && (r!=rrow || i!=(u8)(previ+1))){video_world_dither(start,rrow,run);run=0;}
   previ=i;
   if(!d) {if(run){video_world_dither(start,rrow,run);run=0;}continue;}
   for(j=0;j<8;++j) {
    m=(kk<<3)|j;
    if((d&fog_bit_table[j]) && fog_at(m,r)==1) {if(!run){start=m;rrow=r;}if(++run==15){video_world_dither(start,rrow,run);run=0;}}
    else if(run){video_world_dither(start,rrow,run);run=0;}
   }
  }
  if(run)video_world_dither(start,rrow,run);
 }
 #endif
 }
 if(changed) {
  ++fog_epoch;sprites_dirty=3;
  if(enemy_selected!=NONE && !fog_target(enemy_selected)){enemy_selected=NONE;hud_dirty=3;}
  /* A health marker above a now hidden keep must not survive on a visible row. */
  if(video_enabled)for(i=0;i<building_count;++i)if(building_owner[i] && buildings[i].y){video_tile(buildings[i].x+1,buildings[i].y-1);video_tile(buildings[i].x+2,buildings[i].y-1);}
 }
}
