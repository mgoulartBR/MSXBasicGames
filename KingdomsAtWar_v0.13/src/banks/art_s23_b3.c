#include "../shared.h"
/* Enemy kingdom AI (the dead): abstract economy, a fixed build order inside the enemy base, training of the two
 * enemy fighter slots (8 and 9) and attack waves. Movement is a greedy stepper that walks one cell at a time, idling
 * between cells so that the regular combat code (combat_tick) can pick targets for stationary enemy units. */
/* Sites keep the corridor x=4..7 (y=19..22) free: soldiers cross it at the gate (5,21). */
static const u8 ord_type[8]={2,3,1,6,5,7,1,6};
static const u8 ord_x[8]={7,0,0,8,7,0,0,8},ord_y[8]={24,19,24,19,28,15,28,15};
static const u8 wave_kind[12]={1,1, 2,3, 4,1, 4,3, 4,4, 5,4};
static const i8 ddx[4]={1,0,-1,0},ddy[4]={0,1,0,-1};
static const u8 face_of[4]={1,0,3,2},muster_x[2]={3,8};
static u8 tg_x,tg_y,tg_n;
void ai_init(void) __banked {
 u8 i;
 ai_enabled=1;ai_wood=60;ai_gold=40;ai_launch=200;ai_step=ai_wave=0;ai_site=NONE;ai_left=0;
 for(i=0;i<2;++i)ai_state[i]=ai_timer[i]=ai_stuck[i]=ai_dir[i]=ai_kind[i]=0;
 /* Trees at the edge of the enemy base (x=0 and x=10, y=15..19) are cleared so that the 4x4 sites fit. */
 for(i=15;i<20;i+=2)terrain[((u16)i<<5)+0]=terrain[((u16)i<<5)+10]=stock[((u16)i<<5)+0]=stock[((u16)i<<5)+10]=0;
}
static u8 ai_has(u8 type) {
 u8 i;for(i=0;i<building_count;++i)if(building_owner[i] && buildings[i].state==2 && buildings[i].type==type)return 1;return 0;
}
static u8 site_free(u8 x,u8 y,u8 n) {
 u8 dx,dy,i;
 for(dy=y;dy<y+n;++dy)for(dx=x;dx<x+n;++dx) {
  if(tile(dx,dy)!=0)return 0;
  for(i=0;i<RN;++i)if(combat_alive(i) && ((units[i].x==dx && units[i].y==dy) || (units[i].progress && units[i].nx==dx && units[i].ny==dy)))return 0;
 }
 return 1;
}
static void ai_build(void) {
 u8 type,x,y,n,id;Building* b;
 if(ai_step>=8)return;
 type=ord_type[ai_step];
 if(ai_wood<build_wood[type] || ai_gold<build_gold[type])return;
 x=ord_x[ai_step];y=ord_y[ai_step];n=building_size(type);++ai_step;
 if(!site_free(x,y,n))return;
 for(id=BMAX-1;id>=2;--id)if(!buildings[id].state)break;
 if(id<2)return;
 b=&buildings[id];b->type=type;b->state=1;b->x=x;b->y=y;b->worker=NONE;b->remaining=15;b->training=0;b->time=0;b->rallyx=x;b->rallyy=y+n;
 building_owner[id]=1;building_hp[id]=96;tower_cooldown[id]=0;
 if(id>=building_count)building_count=id+1;
 ai_wood-=build_wood[type];ai_gold-=build_gold[type];ai_site=id;ai_left=15;
 building_refresh(id);sprites_dirty=3;
}
static u8 need_of(u8 kind) {return kind==5?7:kind==4?5:2;}
static u8 spawn(u8 s) {
 u8 i=8+s,x=muster_x[s],k=ai_kind[s];Unit* u=&units[i];
 if(!free_at(x,23,i))return 0;
 u->x=u->tx=u->nx=x;u->y=u->ty=u->ny=23;u->px=(u16)x<<4;u->py=23<<4;u->owner=1;u->state=ARRIVED;u->len=u->pos=u->progress=u->phase=0;
 unit_kind[i]=k;unit_hp[i]=unit_max_hp[k];unit_facing[i]=2;unit_frame[i]=9;unit_walk_dist[i]=unit_walk_phase[i]=0;
 combat_target[i]=NONE;attack_cooldown[i]=attack_flash[i]=hurt_flash[i]=0;
 sprites_dirty=3;units_prep_reset();return 1;
}
void ai_second(void) __banked {
 u8 s,i,k;
 if(!ai_enabled)return;
 ai_wood+=1+ai_has(3);if(game_seconds&1)++ai_gold;
 if(ai_wood>400)ai_wood=400;if(ai_gold>400)ai_gold=400;
 if(ai_site!=NONE) {
  Building* b=&buildings[ai_site];
  if(b->state!=1 || !building_owner[ai_site])ai_site=NONE;
  else if(b->remaining)--b->remaining;
  if(ai_site!=NONE && !b->remaining) {b->state=2;building_refresh(ai_site);ai_site=NONE;hud_dirty=3;}
 } else ai_build();
 for(s=0;s<2;++s) {
  i=8+s;
  if(ai_state[s]<2 && !unit_hp[i])ai_state[s]=2;
  if(ai_state[s]==2) {
   k=ai_wave?wave_kind[(ai_wave>6?5:ai_wave-1)*2+s]:wave_kind[s];
   while(k>1 && !ai_has(need_of(k)))k=k==5?4:k==4?2:k==3?1:1;
   if(!ai_has(2))continue;
   if(ai_wood>=recruit_wood[k] && ai_gold>=recruit_gold[k]) {
    ai_wood-=recruit_wood[k];ai_gold-=recruit_gold[k];ai_kind[s]=k;ai_timer[s]=k>=4?30:20;ai_state[s]=3;
   }
  } else if(ai_state[s]==3) {
   if(ai_timer[s])--ai_timer[s];
   else if(spawn(s))ai_state[s]=0;
  }
 }
 if(!ai_state[0] && !ai_state[1] && unit_hp[8] && unit_hp[9] && game_seconds>=ai_launch) {
  ai_state[0]=ai_state[1]=1;++ai_wave;ai_launch=game_seconds+60;ai_stuck[0]=ai_stuck[1]=0;
 }
}
/* Once per second: the game ends when a keep falls (defeat wins a tie). The simulation then stops (simulate). */
void end_check(void) __banked {
 u8 i;
 if(game_over || !end_enabled)return;
 if(!buildings[0].state || !building_hp[0])game_over=2;
 else if(!buildings[1].state || !building_hp[1])game_over=1;
 if(game_over) {for(i=0;i<RN;++i)attack_flash[i]=hurt_flash[i]=0;combat_active=0;hud_dirty=3;sprites_dirty=3;}
}
static u8 bdist(u8 ux,u8 uy,u8 x,u8 y,u8 n) {
 return (ux<x?x-ux:ux>=x+n?ux-x-n+1:0)+(uy<y?y-uy:uy>=y+n?uy-y-n+1:0);
}
static u8 find_target(u8 i) {
 u8 j,d,best=255,ux=units[i].x,uy=units[i].y;
 if(unit_kind[i]!=5)for(j=0;j<unit_count;++j)if(unit_hp[j] && mine_phase[j]!=2) {
  d=bdist(ux,uy,units[j].x,units[j].y,1);if(d<best){best=d;tg_x=units[j].x;tg_y=units[j].y;tg_n=1;}
 }
 for(j=0;j<building_count;++j)if(!building_owner[j] && buildings[j].state && building_hp[j]) {
  u8 n=building_size(buildings[j].type);
  d=bdist(ux,uy,buildings[j].x,buildings[j].y,n);if(d<best){best=d;tg_x=buildings[j].x;tg_y=buildings[j].y;tg_n=n;}
 }
 return best!=255;
}
static void ai_unit(u8 i,u8 s) {
 Unit* u=&units[i];u8 step;
 if(!unit_hp[i])return;
 if(u->state!=MOVING) {
  u8 d,best=255,bd=0,sc,nx,ny,range;
  if(combat_target[i]!=NONE || (sim_ticks&7)!=1 || !find_target(i))return;
  range=unit_range[unit_kind[i]];
  /* Crossing between the base (south of y=23) and the field (north of y=18): go through the gate first. */
  if((u->y>=23 && tg_y<=18) || (u->y<=18 && tg_y>=23)) {tg_x=5;tg_y=21;tg_n=1;range=1;}
  if(bdist(u->x,u->y,tg_x,tg_y,tg_n)<=range && (ai_stuck[s]<3 || range==1)) {if(ai_stuck[s]<250)++ai_stuck[s];return;}
  for(d=0;d<4;++d) {
   nx=u->x+ddx[d];ny=u->y+ddy[d];
   if(!free_at(nx,ny,i))continue;
   sc=bdist(nx,ny,tg_x,tg_y,tg_n)*2+(d==((ai_dir[s]+2)&3));
   if(sc<best){best=sc;bd=d;}
  }
  if(best==255){if(ai_stuck[s]<250)++ai_stuck[s];return;}
  ai_stuck[s]=0;ai_dir[s]=bd;u->nx=u->x+ddx[bd];u->ny=u->y+ddy[bd];unit_facing[i]=face_of[bd];u->state=MOVING;
 }
 step=(sim_ticks&1)?2:3;if(u->progress+step>16)step=16-u->progress;
 if(u->nx>u->x)u->px+=step;else if(u->nx<u->x)u->px-=step;else if(u->ny>u->y)u->py+=step;else u->py-=step;
 unit_walk_dist[i]+=step;if(unit_walk_dist[i]>=10){unit_walk_dist[i]-=10;unit_walk_phase[i]=(unit_walk_phase[i]+1)&3;}
 u->progress+=step;sprites_dirty=3;
 if(u->progress==16){u->x=u->nx;u->y=u->ny;u->progress=0;u->state=ARRIVED;}
}
void ai_move(void) __banked {
 if(!ai_enabled)return;
 if(ai_state[0]==1)ai_unit(8,0);
 if(ai_state[1]==1)ai_unit(9,1);
}
