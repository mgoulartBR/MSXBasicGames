/* Kingdoms at War v0.8 -- BigFive Studios, 2026.
 * 256 KiB ASCII8. V9990 bitmap / V9968 circular field renderer.
 * Approved 32/48-pixel units and 48/64-pixel buildings, eight allies.
 */
#include "msxgl.h"
#include "mouse.h"
#include "keyboard.h"
#include "bios_hook.h"
#include "v9990.h"
#define N 8
#define RN 10
#define PS 16
u8 unit_count;
#define NONE 255
#define IDLE 0
#define PLANNING 1
#define MOVING 2
#define ARRIVED 3
#define BLOCKED 4
#define ROUTE_MAX 128
#define BASE_X 2
#define BASE_Y 6
#define TO_NODE 1
#define HARVEST 2
#define TO_BASE 3
#define WAIT_NODE 4
#define WAIT_BASE 5
u8 unit_kind[RN],unit_build[N],deposit_id[N];
#define WORKER(i) ((i)<unit_count && unit_hp[i] && unit_kind[i]==0)
u8 worker_task[N],node_x[N],node_y[N],worker_kind[N];
u8 cargo[N],cargo_kind[N],work_timer[N],work_tries[N];
u8 mine_phase[N],mine_step[N];
i16 unit_draw_x,unit_draw_y;
u8 stock[1024];
u16 wood,gold,pickups,deposits;
u16 game_seconds;
u8 second_phase;
volatile u8 hud_live,hud_ready,hud_pending;
u8 hud_draw,hud_dirty;
u8 hud_cache[2][4][8];
u16 hud_values[2][4];
u16 hud_char_writes,hud_erase_count;
volatile u16 hud_swaps;
u8 video_enabled;
typedef struct {u8 x,y,add;i16 dx,dy;u16 cx,cy;} DragEdge;
DragEdge left_edges[2];u8 left_seen,left_x,left_y;u16 left_wx,left_wy;

u8 raw_command_down,raw_command_latched;
u8 raw_down,raw_latched;u16 raw_tick; i16 raw_dx,raw_dy;
void poll_input(void);
/* Exactly 16 bytes. px/py are world pixel coordinates, x/y last reached cell.
 * nx/ny reserve the next cell while the current segment is in progress.
 */
typedef struct {
    u16 px,py;
    u8 x,y,tx,ty,nx,ny,owner,len,pos,progress,phase,state;
} Unit;
Unit units[RN];
u8 terrain[1024],art_map[1024];

u8 routes[N][ROUTE_MAX];
u8 requested[N], retry_wait[N], retry_left[N];
u8 selection_mask, selection_count, nav_cursor;
u16 group_orders, replans;
u8 nav_targets[128],nav_work_mode,mine_turn;
u16 economy_retries,alternate_approaches;
/* parent: 255 unseen, 254 root, 253 blocked, 0..3 incoming direction. */
u8 parent[1024];u8 nav_base_dirty;
u16 queue[1024],qhead,qtail,job_goal;
u8 job_unit;
u16 cam_x,cam_y;
u8 mouse_x,mouse_y,mode,selected,last_keys,cam_keys;
u8 hud_mode,hud_selection,hud_state,sprites_dirty;
u16 sim_ticks,loops,last_time,accumulator,fog_last;
#define FOG_GAP 20
u16 terrain_copies,scroll_copies,full_redraws,nav_expansions;
u16 camera_pixels,camera_phase;
u16 blit_ticks,edge_ticks;

u8 max_camera_step;
Mouse_State mouse;
u8 mouse_present,mouse_port,mouse_active,edge_keys,edge_hold;
u16 mouse_probe_tick,mouse_connections;u8 mouse_probe_port,mouse_warmup,mouse_rescan_down;
#define BMAX 12
/* type,state,x,y,worker,remaining[2],training,time[2],rallyx,rallyy. */
typedef struct {u8 type,state,x,y,worker;u16 remaining;u8 training;u16 time;u8 rallyx,rallyy;} Building;
Building buildings[BMAX];
u8 building_count,building_selected,build_kind,placement_mode,place_worker,place_x,place_y,place_valid;
u8 builder_working,research_level,hud_context[2][30],hud_action_cache[2][30];
u8 hud_markers[2][2],hud_progress[2][2];
const u8 site_tiles[]={1,4,5,8};
const u8 build_wood[]={0,16,24,24,24,32,24,40};
const u8 build_gold[]={0,0,16,8,16,24,8,24};
const u8 recruit_wood[]={0,8,8,8,16,24};
const u8 recruit_gold[]={8,8,8,12,24,16};
const c8* build_names[]={"CASTLE","HOUSE","BARRACKS","SAWMILL","FORGE","STABLE","TOWER","SIEGE"};
const c8* recruit_names[]={"PEASANT","SWORD","SPEAR","ARCHER","KNIGHT","RAM"};
u8 sprite_colors[16];
/* MSX 16x16: left 8 columns top->bottom, then right 8 columns. */
const u8 cursor[] = {128,192,224,240,248,252,254,248,216,140,12,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#include "banked_calls.h"
#include "art07.h"
#include "rom_mapper.h"
u8 graphics_slot,bios_slot,asset_bank;
#include "asset_codec.h"
/* Sample edges while the VDP is busy so short clicks survive a slow frame. */
/* One matrix scan per video frame; every key test below is a constant-mask lookup. */
u8 kbrow[9]={255,255,255,255,255,255,255,255,255};
#define KP(k) ((kbrow[KEY_ROW(k)]&(u8)(1<<KEY_IDX(k)))==0)
void poll_input(void) {
 u8 keys=0,commands=0;u16 tick;
 if(!video_enabled)return;
 tick=video_clock();if(tick==raw_tick)return;raw_tick=tick;
 {u8 r;for(r=0;r<9;++r)kbrow[r]=Keyboard_Read(r);}
 {
  u8 rescan=KP(KEY_M);
  if(rescan && !mouse_rescan_down) {
   mouse_present=mouse_active=0;mouse_probe_tick=tick-30;mouse_probe_port=0;
   raw_dx=raw_dy=0;raw_latched&=~6;last_keys&=~6;
  }
  mouse_rescan_down=rescan;
 }
 /* Web emulators can enable the peripheral only after the ROM has booted. */
 if(!mouse_present && (u16)(tick-mouse_probe_tick)>=30) {
  u8 port=mouse_probe_port?MOUSE_PORT_2:MOUSE_PORT_1;
  mouse_probe_tick=tick;mouse_probe_port^=1;
  if(Input_Detect(port)==INPUT_TYPE_MOUSE) {
   mouse_port=port;mouse_present=1;mouse_warmup=2;++mouse_connections;
   mouse.Buttons=mouse.PrevButtons=255;raw_dx=raw_dy=0;
   raw_down&=~6;raw_latched&=~6;last_keys&=~6;
  }
 }
 if(mouse_present) {
  Mouse_Read(mouse_port,&mouse);
  if(mouse_warmup) --mouse_warmup;
  else {
   raw_dx+=Mouse_GetOffsetX(&mouse);raw_dy+=Mouse_GetOffsetY(&mouse);
   if(Mouse_IsButtonPress(&mouse,MOUSE_BOUTON_LEFT))keys|=2;
   if(Mouse_IsButtonPress(&mouse,MOUSE_BOUTON_RIGHT))keys|=4;
   if(raw_dx||raw_dy||(keys&6))mouse_active=1;
  }
 }
 if(KP(KEY_SPACE))keys|=1;
 if(KP(KEY_Z))keys|=2;
 if(KP(KEY_X))keys|=4;
 if(KP(KEY_A))keys|=8;
 if(KP(KEY_ESC))keys|=16;
 if(KP(KEY_Q))keys|=32;
 if(KP(KEY_R))keys|=64;
 if(KP(KEY_C))keys|=128;
 if(KP(KEY_S))commands|=1;if(KP(KEY_H))commands|=2;
 raw_command_latched|=commands&~raw_command_down;raw_command_down=commands;
 if((keys^raw_down)&2) {
  u8 edge=(keys&2)?0:1;
  if(edge || !(raw_latched&2)) {
   DragEdge* e=left_edges+edge;
   e->x=mouse_x;e->y=mouse_y;e->dx=raw_dx;e->dy=raw_dy;e->cx=cam_x;e->cy=cam_y;e->add=KP(KEY_SHIFT);
   if(edge)left_seen|=2;else left_seen=1;
  }
 }
 raw_latched|=keys&~raw_down;raw_down=keys;
}
void bank_support(void) __naked {
 __asm
 .globl set_bank
 .globl get_bank
 .globl _g_Bank3Segment
set_bank::
 ld (#0x7800),a
 ld (_g_Bank3Segment),a
 ret
get_bank::
 ld a,(_g_Bank3Segment)
 ret
 __endasm;
}


u8 drag_down,drag_moved,drag_add,drag_prior;
u16 drag_anchor_x,drag_anchor_y,drag_l,drag_t,drag_r,drag_b;

u8 unit_hp[RN],combat_target[RN],attack_cooldown[RN],attack_flash[RN],hurt_flash[RN],stance[N];
u8 building_hp[BMAX],building_owner[BMAX],enemy_selected,command_last,combat_active;
u16 combat_hits,combat_kills,combat_buildings;

const u8 unit_max_hp[]={32,48,48,32,72,96,48};
u8 unit_facing[RN],unit_frame[RN],unit_walk_dist[RN],unit_walk_phase[RN];
u8 tile(u8 x,u8 y);
u8 building_size(u8 type) {return build_sizes[type];}

/* Visual composition is cached once per cell; camera work remains a lookup. */

u8 art_tile(u8 x,u8 y) {return art_map[((u16)y<<5)+x];}
/* S,E,N,W. Kept outside Unit so route/debug ABI stays 16 bytes. */

u8 fog_seen[128],fog_visible[128],fog_next[128],fog_memory[1024],fog_refresh;
u8 fog_static[128],fog_build_dirty,fog_need[128],fog_delta[128];
u16 fog_sources[N];
u8 fog_trail[128];u8* fog_dest=fog_next;u16 fog_epoch;

u8 repair_target[N],repair_timer[N],repair_active,repair_pending,tower_cooldown[BMAX];
u16 tower_shots,repair_spent,repair_healed,fog_updates;

u16 shot_x[PS],shot_y[PS],shot_active,combat_shots;
i8 shot_dx[PS],shot_dy[PS];
u8 shot_time[PS],shot_target[PS],shot_damage[PS],shot_owner[PS];
const u8 unit_range[]={0,1,2,4,1,1,1},unit_damage[]={0,8,7,5,12,36,4},unit_cooldown[]={0,24,30,36,24,60,30};


/* 0 unknown, 1 remembered, 2 visible. Z80: A=x, L=y, result in A (sdcccall(1)). */
u8 fog_at(u8 x,u8 y) __naked {
 x;y;
 __asm
 cp #32
 jr nc,fog_at_zero
 ld c,a
 ld a,l
 cp #32
 jr nc,fog_at_zero
 add a,a
 add a,a
 ld b,a
 ld a,c
 rrca
 rrca
 rrca
 and #3
 add a,b
 ld e,a
 ld d,#0
 ld a,c
 and #7
 ld c,a
 ld b,#0
 ld hl,#_fog_bit_table
 add hl,bc
 ld c,(hl)
 ld hl,#_fog_visible
 add hl,de
 ld a,(hl)
 and a,c
 jr z,fog_at_notvis
 ld a,#2
 ret
fog_at_notvis:
 ld hl,#_fog_seen
 add hl,de
 ld a,(hl)
 and a,c
 ret z
 ld a,#1
 ret
fog_at_zero:
 xor a,a
 ret
 __endasm;
}
const u8 fog_bit_table[8]={1,2,4,8,16,32,64,128};
/* Art of a cell: live while visible, otherwise the player's memory. A=x, L=y. */
u8 fog_art(u8 x,u8 y) __naked {
 x;y;
 __asm
 ld c,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld b,#0
 add hl,bc
 ex de,hl
 ld a,c
 and #7
 ld c,a
 ld hl,#_fog_bit_table
 add hl,bc
 ld c,(hl)
 push de
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 ld hl,#_fog_visible
 add hl,de
 ld a,(hl)
 pop de
 and a,c
 ld hl,#_art_map
 jr nz,fog_art_pick
 ld hl,#_fog_memory
fog_art_pick:
 add hl,de
 ld a,(hl)
 ret
 __endasm;
}
#include "video.h"
u8 tile(u8 x,u8 y) { return terrain[((u16)y<<5)+x]; }
u8 passable(u8 x,u8 y)
{
 u8 t;
 if(x>=32 || y>=32) return 0;
 t=tile(x,y); return t==0 || t==3;
}
u8 combat_alive(u8 i) {return i<RN && unit_hp[i] && (i<unit_count || i>=N);}
/* Z80 scan of the unit table: is a living unit standing on, or about to step onto, (fa_x,fa_y)? */
u8 fa_x,fa_y,fa_who;
static u8 occ_scan(void) __naked {
 __asm
 push ix
 ld ix,#_units
 ld b,#0
 ld c,#0
 ld a,(_unit_count)
 ld d,a
oc_loop:
 ld a,(_fa_who)
 cp a,c
 jr z,oc_next
 ld hl,#_unit_hp
 add hl,bc
 ld a,(hl)
 or a,a
 jr z,oc_next
 ld a,c
 cp a,d
 jr c,oc_alive
 cp a,#8
 jr c,oc_next
oc_alive:
 ld a,(_fa_x)
 cp a,4(ix)
 jr nz,oc_prog
 ld a,(_fa_y)
 cp a,5(ix)
 jr z,oc_hit
oc_prog:
 ld a,13(ix)
 or a,a
 jr z,oc_next
 ld a,(_fa_x)
 cp a,8(ix)
 jr nz,oc_next
 ld a,(_fa_y)
 cp a,9(ix)
 jr z,oc_hit
oc_next:
 push bc
 ld bc,#16
 add ix,bc
 pop bc
 inc c
 ld a,c
 cp a,#10
 jr nz,oc_loop
 xor a,a
 pop ix
 ret
oc_hit:
 ld a,#1
 pop ix
 ret
 __endasm;
}
u8 free_at(u8 x,u8 y,u8 who)
{
 if(!passable(x,y)) return 0;
 fa_x=x;fa_y=y;fa_who=who;
 return !occ_scan();
}



void order_one(u8 id,u8 x,u8 y)
{
 Unit* u=&units[id];sprites_dirty=3;
 if(WORKER(id) && mine_phase[id]) {mine_phase[id]=3;fog_refresh=1;sprites_dirty=3;}
 if(job_unit==id)job_unit=NONE;
 u->tx=x;u->ty=y;u->len=u->pos=0;u->state=PLANNING;
 requested[id]=1;retry_wait[id]=0;retry_left[id]=8;
}
/* Distinct nearby goals, bounded to a four-cell radius. A tile occupied by
 * another unit or reserved as a destination is never assigned twice.
 */

u8 economy_order(u8 x,u8 y) __banked;
u8 building_order(u8 x,u8 y) __banked;

/* Cancel the old plan, but finish any occupied segment before choosing
 * a new approach cell. Carried material is never destroyed by cancellation.
 */


/* Four cardinal approach cells; all moving destinations are reserved,
 * including workers already assigned by an earlier economy tick.
 */










/* Bounded FIFO breadth-first search: uniform grid, shortest cardinal route.
 * A single job is processed incrementally; four units can retain their routes.
 */




/* Numeric glyphs go to the inactive panel. No visible clear/draw cycle. */





void main(void) {
 __asm
  ld a,#0
  ld (0xF3DB),a
 __endasm;
 left_seen=0;drag_down=drag_moved=0;
 raw_command_down=raw_command_latched=0;
 raw_down=raw_latched=0;raw_tick=65535;raw_dx=raw_dy=0;
 mouse_probe_tick=mouse_connections=mouse_rescan_down=mouse_probe_port=0;mouse_warmup=2;
 cam_x=cam_y=sim_ticks=loops=accumulator=terrain_copies=scroll_copies=full_redraws=nav_expansions=camera_pixels=camera_phase=0;
 mine_turn=0;economy_retries=alternate_approaches=0;
 blit_ticks=edge_ticks=0;mode=last_keys=cam_keys=max_camera_step=0;
 job_unit=selected=hud_mode=hud_state=NONE;hud_selection=254;
 mouse_x=mouse_y=56;mouse.Buttons=mouse.PrevButtons=255;
 selection_mask=selection_count=nav_cursor=0;group_orders=replans=0;
 for(mode=0;mode<N;++mode)retry_wait[mode]=retry_left[mode]=0;mode=0;
 game_seconds=second_phase=hud_dirty=0;hud_live=hud_ready=hud_pending=hud_draw=video_enabled=0;hud_char_writes=hud_swaps=hud_erase_count=0;
 init_world();fog_init(); graphics_slot=Sys_GetPageSlot(1);bios_slot=Sys_GetPageSlot(0);
 video_init(); ++full_redraws;
 mouse_port=MOUSE_PORT_1;mouse_present=(Input_Detect(INPUT_PORT1)==INPUT_TYPE_MOUSE);
 if(!mouse_present && Input_Detect(INPUT_PORT2)==INPUT_TYPE_MOUSE) {mouse_port=MOUSE_PORT_2;mouse_present=1;}
 video_sprites();hud_init();video_enable();last_time=video_clock();fog_last=last_time-FOG_GAP;
 for(;;) {
  u16 now,dt;
  /* With the retrace interrupt doing the page flip, only idle (nothing to draw or flip) waits. */
  video_hud_swap();
  if(!sprites_dirty)video_wait();
  ++loops;input();now=video_clock();dt=now-last_time;last_time=now;
  if(dt>60)dt=60;
  accumulator+=dt;
  while(accumulator>=2) {simulate();accumulator-=2;}
  /* Sight is recomputed at most every FOG_GAP video ticks; the reveal lags by a fraction of a second. */
  if(fog_refresh && (u16)(now-fog_last)>=FOG_GAP){fog_last=now;fog_update();}
  video_stream();camera_update(dt);
  if(sprites_dirty) {video_sprites();sprites_dirty=0;}
  hud();
 }
}
