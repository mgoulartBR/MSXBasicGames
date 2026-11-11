#include "../shared.h"
/* Military roles share the budgeted multi-goal navigator. */
static u8 target_box(u8 t,u8* x,u8* y,u8* n) {
 if(t==NONE)return 0;
 if(t&128) {u8 id=t&127;if(id>=building_count || !buildings[id].state || !building_hp[id])return 0;*x=buildings[id].x;*y=buildings[id].y;*n=building_size(buildings[id].type);}
 else {if(!combat_alive(t))return 0;*x=units[t].x;*y=units[t].y;*n=1;}
 return 1;
}
static u8 distance(u8 i,u8 x,u8 y,u8 n) {
 u8 ux=units[i].x,uy=units[i].y;
 return (ux<x?x-ux:ux>=x+n?ux-x-n+1:0)+(uy<y?y-uy:uy>=y+n?uy-y-n+1:0);
}
/* Manhattan range to nearest footprint cell and Bresenham obstruction test. */
static u8 cp_core(u8 i,u8 ux,u8 uy,u8 x,u8 y,u8 n) {
 u8 range=unit_range[unit_kind[i]],tx,ty;i16 dx,dy;
 tx=ux<x?x:ux>=x+n?x+n-1:ux;ty=uy<y?y:uy>=y+n?y+n-1:uy;
 dx=(i16)tx-ux;dy=(i16)ty-uy;
 if((dx<0?-dx:dx)+(dy<0?-dy:dy)>range || (!dx && !dy))return 0;
 if(range==1)return 1;
 return line_clear(ux,uy,tx,ty,unit_kind[i]==3,255);
}
u8 combat_position(u8 i,u8 ux,u8 uy,u8 t) __banked {
 u8 x,y,n,range=unit_range[unit_kind[i]];
 if(!range || (unit_kind[i]==5 && !(t&128)) || !target_box(t,&x,&y,&n))return 0;
 return cp_core(i,ux,uy,x,y,n);
}
u8 military_selected(void) __banked {
 u8 i;for(i=0;i<unit_count;++i)if(unit_hp[i] && unit_kind[i]>=1 && unit_kind[i]<=5 && (selection_mask&(1<<i)))return 1;return 0;
}
static void face(u8 i,u8 x,u8 y) {
 i16 dx=(i16)x-units[i].x,dy=(i16)y-units[i].y;
 if(dx>0)unit_facing[i]=1;else if(dx<0)unit_facing[i]=3;else unit_facing[i]=dy<0?2:0;
}
static void die(u8 i) {
 unit_hp[i]=0;fog_refresh=1;attack_flash[i]=0;combat_target[i]=NONE;
 if(i<N) {repair_target[i]=NONE;repair_active&=~(1<<i);work_stop(i);requested[i]=retry_wait[i]=0;worker_task[i]=0;
  if(unit_build[i]!=NONE) {buildings[unit_build[i]].worker=NONE;unit_build[i]=NONE;builder_working&=~(1<<i);}
  /* Carried resources are lost with the worker, not credited to the bank. */
  cargo[i]=cargo_kind[i]=0;mine_phase[i]=0;selection_mask&=~(1<<i);selection_update();
 }
 units[i].progress=units[i].len=units[i].pos=0;units[i].state=IDLE;
 if(enemy_selected==i)enemy_selected=NONE;
 ++combat_kills;hud_dirty=3;sprites_dirty=3;
}
void combat_impact(u8 t,u8 damage,u8 owner) __banked {
 if(t==NONE)return;
 if(t&128) {
  u8 id=t&127;if(id>=building_count || !buildings[id].state || !building_hp[id] || building_owner[id]==owner)return;
  if(building_hp[id]>damage)building_hp[id]-=damage;
  else {building_hp[id]=0;buildings[id].state=0;buildings[id].training=0;building_refresh(id);video_tile(buildings[id].x+1,buildings[id].y-1);video_tile(buildings[id].x+2,buildings[id].y-1);++combat_buildings;if(enemy_selected==t)enemy_selected=NONE;}
 } else {
  if(!combat_alive(t) || units[t].owner==owner)return;
  if(!units[t].owner && research_level && unit_kind[t]>=1 && unit_kind[t]<=5 && damage>1)--damage;
  hurt_flash[t]=8;if(unit_hp[t]>damage)unit_hp[t]-=damage;else die(t);
 }
 if(t&128)structure_dirty(t&127);
 ++combat_hits;hud_dirty=3;sprites_dirty=3;
}
static void strike(u8 i,u8 t) {
 u8 kind=unit_kind[i],damage=military_damage(kind,units[i].owner);
 combat_active=1;attack_flash[i]=8;attack_cooldown[i]=unit_cooldown[kind];
 if(kind==2 && !(t&128) && unit_kind[t]==4)damage+=damage;
 if(kind==3) {
  u8 x,y,n;target_box(t,&x,&y,&n);
  if(units[i].x>=x+n)x+=n-1;else if(units[i].x>=x)x=units[i].x;
  if(units[i].y>=y+n)y+=n-1;else if(units[i].y>=y)y=units[i].y;
  shot_x[i]=units[i].px+8;shot_y[i]=units[i].py+4;
  shot_dx[i]=(i16)x-units[i].x;shot_dy[i]=(i16)y-units[i].y;
  shot_target[i]=t;shot_owner[i]=units[i].owner;shot_damage[i]=damage;shot_time[i]=16;shot_active|=((u16)1<<i);++combat_shots;
 } else combat_impact(t,damage,units[i].owner);
 hud_dirty=3;sprites_dirty=3;
}
void combat_init(void) __banked {
 u8 i,x,y;
 for(i=0;i<PS;++i)shot_time[i]=0;
 combat_shots=shot_active=0;combat_active=0;combat_hits=combat_kills=combat_buildings=0;enemy_selected=NONE;command_last=0;
 for(i=0;i<RN;++i) {shot_time[i]=0;unit_hp[i]=i<unit_count?unit_max_hp[unit_kind[i]]:0;combat_target[i]=NONE;attack_cooldown[i]=attack_flash[i]=hurt_flash[i]=0;}
 for(i=0;i<N;++i)stance[i]=0;
 for(i=0;i<BMAX;++i) {building_owner[i]=0;building_hp[i]=i?0:240;}
 nav_base_dirty=1;
 for(y=20;y<32;++y)for(x=0;x<12;++x) {u16 c=((u16)y<<5)+x;terrain[c]=0;stock[c]=0;}
 building_count=2;buildings[1].type=0;buildings[1].state=2;buildings[1].x=3;buildings[1].y=27;buildings[1].worker=NONE;buildings[1].training=0;building_owner[1]=1;building_hp[1]=180;
 for(y=27;y<31;++y)for(x=3;x<7;++x)terrain[((u16)y<<5)+x]=11;
 for(i=N;i<RN;++i) {Unit* u=&units[i];u->x=u->tx=u->nx=i==N?3:8;u->y=u->ty=u->ny=23;u->px=(u16)u->x*16;u->py=(u16)u->y*16;u->owner=1;u->state=ARRIVED;u->len=u->pos=u->progress=u->phase=0;unit_kind[i]=6;unit_hp[i]=48;unit_facing[i]=2;unit_frame[i]=9;unit_walk_dist[i]=unit_walk_phase[i]=0;}
 ai_init();
}
u8 combat_hit_test(u16 wx,u16 wy) __banked {
 u8 i;
 for(i=N;i<RN;++i)if(unit_position(i)) {const u8* b=pose_bounds+(u16)POSE_KEY(i)*4;i16 x=unit_draw_x+cam_x,y=unit_draw_y+cam_y;
  if((i16)wx>=x+b[0] && (i16)wx<x+b[0]+b[2] && (i16)wy>=y+b[1] && (i16)wy<y+b[1]+b[3])return i;
 }
 if(wx<512 && wy<512) {i=tile(wx>>4,wy>>4);if(i>=10 && i-10<building_count && building_owner[i-10] && buildings[i-10].state && fog_target(128+i-10))return 128+i-10;}
 return NONE;
}
u8 combat_click(u16 wx,u16 wy) __banked {
 u8 i,t=combat_hit_test(wx,wy);
 if(t==NONE || !fog_target(t))return 0;combat_active=1;
 for(i=0;i<unit_count;++i)if(unit_hp[i] && unit_kind[i]>=1 && unit_kind[i]<=5 && (selection_mask&(1<<i))) {if(unit_kind[i]==5 && !(t&128))continue;work_stop(i);combat_target[i]=t;stance[i]=0;}
 sprites_dirty=3;hud_dirty=3;return 1;
}
void combat_stop(u8 hold) __banked {
 u8 i;for(i=0;i<unit_count;++i)if(unit_hp[i] && unit_kind[i]>=1 && unit_kind[i]<=5 && (selection_mask&(1<<i))) {work_stop(i);combat_target[i]=NONE;stance[i]=hold;attack_flash[i]=0;}
 sprites_dirty=3;hud_dirty=3;
}
/* Cheap reject before the banked range test: Manhattan distance from (ux,uy) to the n x n box at (bx,by). */
static u8 near_box(u8 ux,u8 uy,u8 bx,u8 by,u8 n,u8 range) {
 u8 dx=ux<bx?bx-ux:ux>=bx+n?ux-(bx+n-1):0,dy=uy<by?by-uy:uy>=by+n?uy-(by+n-1):0;
 return (u8)(dx+dy)<=range;
}
/* Z80: first unit from cs_j on that is a cheap candidate target for the unit at (cs_xi,cs_yi): alive, within
 * cs_rng on both axes, other owner, standing still, not hidden inside a mine. 255 when none. */
static u8 cs_xi,cs_yi,cs_own,cs_rng,cs_j;
static u8 cand_scan(void) __naked {
 __asm
 push ix
 ld a,(_cs_j)
 ld c,a
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
 ld b,#0
cs_loop:
 ld a,c
 cp a,#10
 jr nc,cs_none
 ld hl,#_unit_hp
 add hl,bc
 ld a,(hl)
 or a,a
 jr z,cs_next
 ld a,4(ix)
 ld hl,#_cs_xi
 sub a,(hl)
 jr nc,cs_dx
 neg
cs_dx:
 ld hl,#_cs_rng
 cp a,(hl)
 jr c,cs_dxok
 jr nz,cs_next
cs_dxok:
 ld a,5(ix)
 ld hl,#_cs_yi
 sub a,(hl)
 jr nc,cs_dy
 neg
cs_dy:
 ld hl,#_cs_rng
 cp a,(hl)
 jr c,cs_dyok
 jr nz,cs_next
cs_dyok:
 ld a,10(ix)
 ld hl,#_cs_own
 cp a,(hl)
 jr z,cs_next
 ld a,13(ix)
 or a,a
 jr nz,cs_next
 ld a,c
 cp a,#8
 jr nc,cs_ok
 ld hl,#_mine_phase
 add hl,bc
 ld a,(hl)
 cp a,#2
 jr z,cs_next
cs_ok:
 ld a,c
 pop ix
 ret
cs_next:
 push bc
 ld bc,#16
 add ix,bc
 pop bc
 inc c
 jr cs_loop
cs_none:
 ld a,#255
 pop ix
 ret
 __endasm;
}
static u8 ct_i;
/* Z80: tick down the flash/cooldown counters of every unit and return the next unit at or after `start` that
 * is alive and can fight (255 = none). */
static u8 ct_scan(u8 start) __naked {
 start;
 __asm
 ld (_ct_i),a
ct_lp:
 ld a,(_ct_i)
 cp a,#10
 jp nc,ct_none
 ld e,a
 ld d,#0
 ld hl,#_attack_flash
 add hl,de
 ld a,(hl)
 or a,a
 jp z,ct_a
 ld b,a
 sub a,#2
 jp nc,ct_a1
 xor a,a
ct_a1:
 ld (hl),a
 ld c,a
 ld a,#1
 ld (_combat_active),a
 ld a,c
 or a,a
 jp z,ct_ad
 cp a,#5
 jp nc,ct_a
 ld a,b
 cp a,#5
 jp c,ct_a
ct_ad:
 ld a,#3
 ld (_sprites_dirty),a
ct_a:
 ld hl,#_hurt_flash
 add hl,de
 ld a,(hl)
 or a,a
 jp z,ct_h
 ld b,a
 sub a,#2
 jp nc,ct_h1
 xor a,a
ct_h1:
 ld (hl),a
 ld c,a
 ld a,#1
 ld (_combat_active),a
 ld a,c
 or a,a
 jp z,ct_hd
 cp a,#5
 jp nc,ct_h
 ld a,b
 cp a,#5
 jp c,ct_h
ct_hd:
 ld a,#3
 ld (_sprites_dirty),a
ct_h:
 ld hl,#_attack_cooldown
 add hl,de
 ld a,(hl)
 or a,a
 jp z,ct_c
 sub a,#2
 jp nc,ct_c1
 xor a,a
ct_c1:
 ld (hl),a
 ld a,#1
 ld (_combat_active),a
ct_c:
 ld hl,#_unit_hp
 add hl,de
 ld a,(hl)
 or a,a
 jp z,ct_next
 ld hl,#_unit_kind
 add hl,de
 ld c,(hl)
 ld b,#0
 ld hl,#_unit_range
 add hl,bc
 ld a,(hl)
 or a,a
 jp z,ct_next
 ld hl,#_combat_target
 add hl,de
 ld a,(hl)
 inc a
 jp nz,ct_ret
 ld a,(_sim_ticks)
 and a,#7
 jp nz,ct_next
ct_ret:
 ld a,(_ct_i)
 ret
ct_next:
 ld hl,#_ct_i
 inc (hl)
 jp ct_lp
ct_none:
 ld a,#255
 ret
 __endasm;
}
void combat_tick(void) __banked {
 u8 i,j,x,y,n,t,tbv;combat_active=0;
 for(i=ct_scan(0);i!=255;i=ct_scan(i+1)) {
  t=combat_target[i];tbv=0;if(t!=NONE)combat_active=1;
  if(t!=NONE) {
   tbv=target_box(t,&x,&y,&n);
   if(!tbv || (i<N && !fog_target(t)) || ((t&128)?building_owner[t&127]==units[i].owner:units[t].owner==units[i].owner)) {
    combat_target[i]=t=NONE;tbv=0;if(i<N)work_stop(i);
   }
  }
  /* Hold/idle allies and stationary defenders retaliate only at melee range. */
  if(t==NONE && !(sim_ticks&7) && !units[i].progress && units[i].state!=MOVING && (i>=N || stance[i])) {
   {u8 rng=unit_range[unit_kind[i]];cs_xi=units[i].x;cs_yi=units[i].y;cs_own=units[i].owner;cs_rng=rng;cs_j=0;
    for(;;) {
     j=cand_scan();if(j==255)break;
     if((i>=N || fog_target(j)) && combat_position(i,units[i].x,units[i].y,j)) {combat_target[i]=t=j;break;}
     cs_j=j+1;
    }}
   if(t==NONE && i>=N)for(j=0;j<building_count;++j)if(buildings[j].state && building_hp[j] && building_owner[j]!=units[i].owner && near_box(units[i].x,units[i].y,buildings[j].x,buildings[j].y,building_size(buildings[j].type),unit_range[unit_kind[i]]) && combat_position(i,units[i].x,units[i].y,128+j)){combat_target[i]=t=128+j;break;}
  }
  if(t==NONE || (!tbv && !target_box(t,&x,&y,&n)) || units[i].progress)continue;
  if(unit_range[unit_kind[i]] && !(unit_kind[i]==5 && !(t&128)) && cp_core(i,units[i].x,units[i].y,x,y,n)) {
   if(i<N && (requested[i] || units[i].state==MOVING || job_unit==i))work_stop(i);
   face(i,x,y);
   if(!attack_cooldown[i] && !shot_time[i])strike(i,t);
  } else if(i<N && !stance[i] && !requested[i] && job_unit!=i && units[i].state!=MOVING && !retry_wait[i]) {
   order_one(i,x,y);retry_wait[i]=15;
  } else if(i>=N)combat_target[i]=NONE;
 }
}
static void number(c8* p,u8 value) {p[0]='0'+value/100;p[1]='0'+value/10%10;p[2]='0'+value%10;p[3]=0;}
u8 combat_context(void) __banked {
 c8 text[30];u8 i=0,hp,max;const c8* name;
 if(enemy_selected!=NONE) {u8 t=enemy_selected;if(t&128) {u8 bt=buildings[t&127].type;hp=building_hp[t&127];max=bt?96:180;name=bt?build_names[bt]:"ENEMY KEEP";}else {hp=unit_hp[t];max=48;name="ENEMY SWORD";}}
 else if(selected!=NONE && unit_kind[selected]>=1 && unit_kind[selected]<=5) {hp=unit_hp[selected];max=unit_max_hp[unit_kind[selected]];name=recruit_names[unit_kind[selected]];}
 else return 0;
 while(*name)text[i++]=*name++;text[i++]=' ';text[i++]='H';text[i++]='P';text[i++]=' ';number(text+i,hp);i+=3;text[i++]='/';number(text+i,max);i+=3;
 if(enemy_selected==NONE && selected!=NONE) {u8 d=military_damage(unit_kind[selected],0);text[i++]=' ';text[i++]='A';text[i++]='0'+d/10;text[i++]='0'+d%10;text[i++]=' ';text[i++]='D';text[i++]=research_level?'1':'0';text[i]=0;}
 hud_context_text(text);return 1;
}

