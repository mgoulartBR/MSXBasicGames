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
static u8 fog_merge_ready;
void fog_init(void) __banked {
 u16 i;fog_merge_ready=0;fog_dest=fog_next;fog_epoch=0;fog_refresh=1;fog_build_dirty=1;fog_updates=0;repair_active=repair_pending=0;tower_shots=repair_spent=repair_healed=0;
 for(i=0;i<128;++i)fog_trail[i]=fog_seen[i]=fog_visible[i]=0;
 for(i=0;i<1024;++i)fog_memory[i]=0;
 for(i=0;i<N;++i){fog_sources[i]=65535;repair_target[i]=NONE;repair_timer[i]=0;}
 for(i=0;i<BMAX;++i)tower_cooldown[i]=0;
 if(fog_update())fog_apply();
}
static u8 fog_merge_acc,fog_work_idx[128],fog_work_n;
/* Single pass with a fast skip: a byte with next==visible and no trail changes nothing (need/delta are
 * cleared up front; seen already contains every visible bit), the common case. */
u8 fog_merge(void) __banked __naked {
#ifdef __SDCC
 __asm
 push ix
 push iy
 ld a,(_fog_merge_ready)
 or a,a
 jr z,fm_full
 ld a,(_fog_work_n)
 or a,a
 jr z,fm_cleared
 ld b,a
 ld ix,#_fog_work_idx
fm_lz:
 ld e,0(ix)
 ld d,#0
 ld hl,#_fog_need
 add hl,de
 ld (hl),#0
 ld hl,#_fog_delta
 add hl,de
 ld (hl),#0
 inc ix
 djnz fm_lz
 jr fm_cleared
fm_full:
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
 ld a,#1
 ld (_fog_merge_ready),a
fm_cleared:
 xor a
 ld (_fog_merge_acc),a
 ld (_fog_work_n),a
 ld hl,#_fog_next
 ld de,#_fog_visible
 ld ix,#_fog_trail
 ld iy,#_fog_seen
 ld b,#128
fm_loop:
 ld a,(de)
 xor a,(hl)
 jr nz,fm_work
 ld a,0(ix)
 or a,a
 jr z,fm_step
 ld a,0(iy)
 cpl
 and a,0(ix)
 jr nz,fm_work
 ld 0(ix),a
fm_step:
 inc hl
 inc de
 inc ix
 inc iy
 djnz fm_loop
 ld a,(_fog_merge_acc)
 pop iy
 pop ix
 ret
fm_work:
 push bc
 ld a,#128
 sub a,b
 ld c,a
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
 pop bc
 jr fm_step
 __endasm;
#endif
}

static u8 fb_mode,fb_k,fb_v,fb_i,fb_j;static const u8* fb_arr;
void fog_act(u16 c) {
 if(fb_mode==0){if(fog_memory[c]!=art_map[c]){fog_memory[c]=art_map[c];if(video_enabled)video_known_tile(c&31,c>>5);}}
 else video_world_tile(c&31,c>>5);
}
/* Z80: visit every set bit of arr[] over the touched bytes in fog_work_idx[]; cell = byte*8+bit. */
static void fog_bits(const u8* arr) __naked {
 __asm
 ld (_fb_arr),hl
 xor a,a
 ld (_fb_k),a
fb_lp:
 ld a,(_fb_k)
 ld hl,#_fog_work_n
 cp a,(hl)
 jp nc,fb_end
 ld e,a
 ld d,#0
 ld hl,#_fog_work_idx
 add hl,de
 ld a,(hl)
 ld (_fb_i),a
 ld e,a
 ld hl,(_fb_arr)
 add hl,de
 ld a,(hl)
 or a,a
 jp z,fb_nx
 ld (_fb_v),a
 xor a,a
 ld (_fb_j),a
fb_bit:
 ld a,(_fb_v)
 or a,a
 jp z,fb_nx
 srl a
 ld (_fb_v),a
 jp nc,fb_skip
 ld a,(_fb_i)
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 ld a,(_fb_j)
 ld e,a
 ld d,#0
 add hl,de
 call _fog_act
fb_skip:
 ld hl,#_fb_j
 inc (hl)
 jp fb_bit
fb_nx:
 ld hl,#_fb_k
 inc (hl)
 jp fb_lp
fb_end:
 ret
 __endasm;
}
static u8 fog_chg;
/* Phase 1 of a sight update: recompute and merge the visibility (returns 1 when fog_apply must follow). */
u8 fog_update(void) __banked {
 u8 i;
 for(i=0;i<N;++i) {
  u16 source=(i<unit_count && unit_hp[i] && !(WORKER(i) && mine_phase[i]==2))?((u16)units[i].y<<5)+units[i].x:65535;
  if(source!=fog_sources[i]){fog_sources[i]=source;fog_refresh=1;}
 }
 if(!fog_refresh)return 0;fog_refresh=0;++fog_updates;
 if(fog_build_dirty) {
 fog_build_dirty=0;fog_dest=fog_static;__builtin_memset(fog_static,0,128);
 for(i=0;i<building_count;++i)if(!building_owner[i] && buildings[i].state) {
  Building* b=&buildings[i];u8 n=building_size(b->type);
  sight(b->x+n/2,b->y+n/2,b->state==1?3:b->type==6?7:b->type==0?6:4);
 }
 }
 __builtin_memcpy(fog_next,fog_static,128);fog_dest=fog_next;
 for(i=0;i<unit_count;++i)if(fog_sources[i]!=65535)sight(units[i].x,units[i].y,unit_kind[i]==4?6:5);
 fog_chg=fog_merge();
 return 1;
}
/* Phase 2, one frame later: remembered tiles, tile names and the epoch bump for what phase 1 changed. */
void fog_apply(void) __banked {
 u8 i,changed=fog_chg;
 fb_mode=0;fog_bits(fog_need);
 /* P1: a changed cell is just a new name in the tile map (newly visible, remembered or newly seen). */
 if(changed && video_enabled){fb_mode=1;fog_bits(fog_delta);}
 if(changed) {
  ++fog_epoch;sprites_dirty=3;
  if(enemy_selected!=NONE && !fog_target(enemy_selected)){enemy_selected=NONE;hud_dirty=3;}
  /* A health marker above a now hidden keep must not survive on a visible row. */
  if(video_enabled)for(i=0;i<building_count;++i)if(building_owner[i] && buildings[i].y){video_tile(buildings[i].x+1,buildings[i].y-1);video_tile(buildings[i].x+2,buildings[i].y-1);}
 }
}
