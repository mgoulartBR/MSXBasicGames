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
u8 fog_merge(void) __banked __naked {
#ifdef __SDCC
 __asm
 push ix
 push iy
 ld hl,#_fog_seen
 ld de,#_fog_trail
 ld ix,#_fog_delta
 ld b,#128
kaw012_trail:
 ld a,(de)
 ld c,a
 ld a,(hl)
 cpl
 and c
 ld 0(ix),a
 ld a,(hl)
 or c
 ld (hl),a
 xor a
 ld (de),a
 inc hl
 inc de
 inc ix
 djnz kaw012_trail
 ld hl,#_fog_visible
 ld de,#_fog_next
 ld ix,#_fog_need
 ld iy,#_fog_delta
 ld b,#128
kaw012_visible:
 ld a,(de)
 ld c,a
 ld a,(hl)
 cpl
 and c
 or 0(iy)
 ld 0(ix),a
 ld a,(hl)
 xor c
 or 0(iy)
 ld 0(iy),a
 ld (hl),c
 inc hl
 inc de
 inc ix
 inc iy
 djnz kaw012_visible
 ld hl,#_fog_seen
 ld de,#_fog_next
 ld b,#128
kaw012_seen:
 ld a,(de)
 or (hl)
 ld (hl),a
 inc hl
 inc de
 djnz kaw012_seen
 ld hl,#_fog_changed
 ld de,#_fog_changed+128
 ld ix,#_fog_delta
 ld bc,#32768
kaw012_damage:
 ld a,0(ix)
 or c
 ld c,a
 ld a,0(ix)
 or (hl)
 ld (hl),a
 ld a,(de)
 or 0(ix)
 ld (de),a
 inc hl
 inc de
 inc ix
 djnz kaw012_damage
 ld a,c
 pop iy
 pop ix
 ret
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
 for(i=0;i<128;++i) {
  u8 need=fog_need[i];
  if(need)for(j=0;j<8;++j)if(need&(1<<j)) {
   c=((u16)i<<3)+j;
   if(fog_memory[c]!=art_map[c]) {fog_memory[c]=art_map[c];if(video_enabled)video_known_tile(c&31,c>>5);}
  }
 }
 if(changed) {
  ++fog_epoch;sprites_dirty=3;
  if(enemy_selected!=NONE && !fog_target(enemy_selected)){enemy_selected=NONE;hud_dirty=3;}
  /* A health marker above a now hidden keep must not survive on a visible row. */
  if(video_enabled)for(i=0;i<building_count;++i)if(building_owner[i] && buildings[i].y){video_tile(buildings[i].x+1,buildings[i].y-1);video_tile(buildings[i].x+2,buildings[i].y-1);}
 }
}
