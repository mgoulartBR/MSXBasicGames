#pragma once

#include "msxgl.h"
#include "mouse.h"
#include "keyboard.h"
#include "bios_hook.h"
#include "v9990.h"
#define N 8
#define RN 10
#define PS 16
extern u8 unit_count;
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
extern u8 unit_kind[RN],unit_build[N],deposit_id[N];
#define WORKER(i) ((i)<unit_count && unit_hp[i] && unit_kind[i]==0)
extern u8 worker_task[N],node_x[N],node_y[N],worker_kind[N];
extern u8 cargo[N],cargo_kind[N],work_timer[N],work_tries[N];
extern u8 mine_phase[N],mine_step[N];
extern i16 unit_draw_x,unit_draw_y;
extern u8 stock[1024];
extern u16 wood,gold,pickups,deposits;
extern u16 game_seconds;
extern u8 second_phase;
extern volatile u8 hud_live,hud_ready,hud_pending;
extern u8 hud_draw,hud_dirty;
extern u8 hud_cache[2][4][8];
extern u16 hud_values[2][4];
extern u16 hud_char_writes,hud_erase_count;
extern volatile u16 hud_swaps;
extern u8 video_enabled;

typedef struct {
    u16 px,py;
    u8 x,y,tx,ty,nx,ny,owner,len,pos,progress,phase,state;
} Unit;
extern Unit units[RN];
extern u8 terrain[1024],art_map[1024];

extern u8 routes[N][ROUTE_MAX];
extern u8 requested[N], retry_wait[N], retry_left[N];
extern u8 selection_mask, selection_count, nav_cursor;
extern u16 group_orders, replans;
extern u8 nav_targets[128],nav_work_mode,mine_turn;
extern u16 economy_retries,alternate_approaches;

extern u8 parent[1024];
extern u16 queue[1024],qhead,qtail,job_goal;
extern u8 job_unit;
extern u16 cam_x,cam_y;
extern u8 mouse_x,mouse_y,mode,selected,last_keys,cam_keys;
extern u8 hud_mode,hud_selection,hud_state,sprites_dirty;
extern u16 sim_ticks,loops,last_time,accumulator;
extern u16 terrain_copies,scroll_copies,full_redraws,nav_expansions;
extern u16 camera_pixels,camera_phase;
extern u16 blit_ticks,edge_ticks;

extern u8 max_camera_step;
extern Mouse_State mouse;
extern u8 mouse_present,mouse_port,mouse_active,edge_keys,edge_hold;
extern u16 mouse_probe_tick,mouse_connections;extern u8 mouse_probe_port,mouse_warmup,mouse_rescan_down;
#define BMAX 12

typedef struct {u8 type,state,x,y,worker;u16 remaining;u8 training;u16 time;u8 rallyx,rallyy;} Building;
extern Building buildings[BMAX];
extern u8 building_count,building_selected,build_kind,placement_mode,place_worker,place_x,place_y,place_valid;
extern u8 builder_working,research_level,hud_context[2][30],hud_action_cache[2][30];
extern u8 hud_markers[2][2],hud_progress[2][2];
extern const u8 site_tiles[];
extern const u8 build_wood[];
extern const u8 build_gold[];
extern const u8 recruit_wood[];
extern const u8 recruit_gold[];
extern const c8* build_names[];
extern const c8* recruit_names[];
extern u8 sprite_colors[16];

extern const u8 cursor[];


extern u8 unit_hp[RN],combat_target[RN],attack_cooldown[RN],attack_flash[RN],hurt_flash[RN],stance[N];
extern u8 building_hp[BMAX],building_owner[BMAX],enemy_selected,command_last,combat_active;
extern u16 combat_hits,combat_kills,combat_buildings;

u8 building_size(u8 type);
u8 art_index(u8 x,u8 y) __banked;
u8 art_tile(u8 x,u8 y);
u8 unit_art_frame(u8 i) __banked;
u8 unit_position(u8 i);
u8 tile(u8 x,u8 y);
u8 passable(u8 x,u8 y);
u8 free_at(u8 x,u8 y,u8 who);
void init_world(void) __banked;
void selection_update(void) __banked;
void select_at(u16 wx,u16 wy,u8 additive) __banked;
void order_one(u8 id,u8 x,u8 y);
u8 goal_free(u8 x,u8 y,u8 id,u8 assigned) __banked;
void order_at(u8 x,u8 y) __banked;
void work_stop(u8 id) __banked;
u8 economy_order(u8 x,u8 y) __banked;
u8 object_size(u8 x,u8 y) __banked;
void deposit_choose(u8 id) __banked;
u8 work_route(u8 id,u8 x,u8 y) __banked;
u8 work_adjacent(u8 id,u8 x,u8 y) __banked;
void resource_empty(u8 x,u8 y) __banked;
void economy(void) __banked;
void nav_retry(u8 id) __banked;
void nav_begin(u8 id) __banked;
void nav_finish(u16 cell) __banked;
void navigation(void) __banked;
void simulate(void) __banked;
void input(void) __banked;
void camera_update(u16 dt) __banked;
u8 hud_field(u8 field,u8 x,u16 y,const c8* value) __banked;
void format_amount(c8* out,u16 value) __banked;
u8 hud_render(void) __banked;
void hud_init(void) __banked;
void hud(void) __banked;
u8 has_building(u8 type) __banked;
u8 population_limit(void) __banked;
u8 population_free(void) __banked;
u8 build_unlocked(u8 kind) __banked;
void building_refresh(u8 id) __banked;
u8 placement_valid(u8 x,u8 y) __banked;
void build_assign(u8 id,u8 who) __banked;
u8 building_order(u8 x,u8 y) __banked;
void placement_choose(u8 kind) __banked;
void placement_toggle(void) __banked;
void placement_confirm(void) __banked;
u8 recruit_allowed(u8 id,u8 kind) __banked;
void train_kind(u8 kind) __banked;
void train_start(void) __banked;
void production_cancel(void) __banked;
void production(void) __banked;
void hud_context_text(const c8* text) __banked;
void hud_actions(void) __banked;
void video_sprites(void);
u8 video_camera_ready(u16 y);
void video_camera(void);
void video_stream(void);
void video_tile(u8 x,u8 y);
void video_init(void);
void video_enable(void);
u16 video_clock(void);
void video_wait(void);
void video_char(u8 x,u16 y,u8 c);
void video_text(u8 x,u16 y,const c8* s);
void video_fill(u16 x,u16 y,u16 w,u16 h,u8 c);
void video_hud_begin(void);
void video_hud_end(void);
void video_hud_flush(void);
void video_cursor(void);
void video_present(u8 p);

extern u8 unit_facing[RN],unit_frame[RN],unit_walk_dist[RN],unit_walk_phase[RN];

extern const u8 kind_sizes[],pose_bounds[],build_base[];

extern u8 raw_down,raw_latched;extern i16 raw_dx,raw_dy;
void poll_input(void);

void combat_init(void) __banked;
void combat_tick(void) __banked;
u8 combat_click(u16 wx,u16 wy) __banked;
u8 combat_hit_test(u16 wx,u16 wy) __banked;
void combat_stop(u8 hold) __banked;
u8 combat_context(void) __banked;
u8 combat_alive(u8 i);

extern const u8 unit_max_hp[];

extern u8 raw_command_down,raw_command_latched;

void drag_begin(void) __banked;
void drag_tick(void) __banked;
void group_context(void) __banked;

extern u8 drag_down,drag_moved,drag_add,drag_prior;
extern u16 drag_anchor_x,drag_anchor_y,drag_l,drag_t,drag_r,drag_b;

void left_prepare(void) __banked;

typedef struct {u8 x,y,add;i16 dx,dy;u16 cx,cy;} DragEdge;
extern DragEdge left_edges[2];
extern u8 left_seen,left_x,left_y;extern u16 left_wx,left_wy;

u16 damage_mask(u8 p,u16 mask,u8 flags) __banked;
#include "render_types.h"
extern Rect rect_now[RN],rect_old[2][RN];
extern const u16 render_bit[RN];
extern i16 strip_l[2],strip_t[2],strip_r[2],strip_b[2];extern u8 strip_n;
extern u8 box_old[2],changed_x[2][32],changed_y[2][32],changed_count[2],ghost_old[2],ghost_size[2];
extern u16 box_l[2],box_t[2],box_r[2],box_b[2],ghost_x[2],ghost_y[2];
u8 intersect(const Rect* a,const Rect* b);

u8 military_selected(void) __banked;
u8 combat_position(u8 i,u8 x,u8 y,u8 t) __banked;
void combat_impact(u8 t,u8 damage,u8 owner) __banked;
void projectile_tick(void) __banked;
u16 arrow_damage(u8 p,u16 mask) __banked;
void arrow_draw(u8 p) __banked;
void world_fill(i16 x,i16 y,u8 w,u8 h,u8 p,u8 c);
void restore_field(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy);

extern u16 shot_x[PS],shot_y[PS],shot_active,combat_shots,arrow_old[2],arrow_old_x[2][PS],arrow_old_y[2][PS];
extern i8 shot_dx[PS],shot_dy[PS];
extern u8 shot_time[PS],shot_target[PS],shot_damage[PS],shot_owner[PS];
extern const u8 unit_range[],unit_damage[],unit_cooldown[];


u8 fog_at(u8 x,u8 y);
u8 fog_art(u8 x,u8 y);
u8 fog_target(u8 t) __banked;
void fog_init(void) __banked;
void fog_update(void) __banked;
u16 fog_damage(u8 p,u16 mask) __banked;
void fog_apply(u8 p) __banked;
void fog_cover(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy) __banked;
void clip_copy(i16 wx,i16 wy,u16 sx,u16 sy,u8 w,u8 h,u8 p,u16 cx,u16 cy);
u8 line_clear(u8 x,u8 y,u8 tx,u8 ty,u8 water,u8 ignore) __banked;
u8 military_damage(u8 kind,u8 owner) __banked;
u8 structure_max_hp(u8 id) __banked;
void structure_dirty(u8 id) __banked;
void structure_bars(u8 p) __banked;
u8 structure_context(void) __banked;
void tower_tick(void) __banked;
u8 repair_order(u8 id) __banked;
void repair_tick(void) __banked;
u8 repair_context(void) __banked;

extern u8 fog_seen[128],fog_visible[128],fog_next[128],fog_changed[2][128],fog_memory[1024],fog_refresh;
extern u16 fog_sources[N];
extern u8 repair_target[N],repair_timer[N],repair_active,tower_cooldown[BMAX];
extern u16 tower_shots,repair_spent,repair_healed,fog_updates;

u8 fog_page_pending(u8 p) __banked;

void fog_traverse(u8 i) __banked;
void video_known_tile(u8 x,u8 y);
void video_world_cell(u8 x,u8 y);
extern const u8 fog_bit_table[8];
extern u8 fog_trail[128],fog_pending[2];extern u8* fog_dest;extern u16 fog_epoch,fog_checked_epoch[2],fog_checked_x[2],fog_checked_y[2];

extern u8 fog_static[128],fog_build_dirty,repair_pending;
void fog_stamp(u8 x,u8 y,u8 r) __banked;

extern u8 fog_need[128],fog_delta[128];
u8 fog_merge(void) __banked;
