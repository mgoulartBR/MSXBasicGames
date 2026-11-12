#include "../shared.h"
static u16 cursor_tick;
void input(void) __banked {
 i16 x=mouse_x,y=mouse_y;
 u8 keys,commands;
 poll_input();commands=raw_command_down|raw_command_latched;command_last&=~raw_command_latched;raw_command_latched=0;
 if(commands&~command_last)combat_stop((commands&2)?1:0);command_last=commands;
 poll_input();keys=raw_down|raw_latched;last_keys&=~raw_latched;raw_latched=0;
 x+=raw_dx;y+=raw_dy;raw_dx=raw_dy=0;
 cam_keys=0;
 /* Keyboard cursor: one 3-pixel step per video frame, not per loop pass (the loop is free-running now). */
 {u16 ct=video_clock();if(ct!=cursor_tick) {cursor_tick=ct;
 if(KP(KEY_J)) x-=3;
 if(KP(KEY_L)) x+=3;
 if(KP(KEY_I)) y-=3;
 if(KP(KEY_K)) y+=3;
 }}
 if(KP(KEY_I) || KP(KEY_J) || KP(KEY_K) || KP(KEY_L))mouse_active=0;
 if(x<0)x=0; if(x>248)x=248; if(y<0)y=0; if(y>204)y=204;
 mouse_x=x; mouse_y=y;
 if(KP(KEY_LEFT)) cam_keys|=1;
 if(KP(KEY_RIGHT)) cam_keys|=2;
 if(KP(KEY_UP)) cam_keys|=4;
 if(KP(KEY_DOWN)) cam_keys|=8;
 edge_keys=0;
 if(mouse_present && mouse_active && mouse_y<160) {
  if(mouse_x<3)edge_keys|=1;if(mouse_x>245)edge_keys|=2;
  if(mouse_y<3)edge_keys|=4;if(mouse_y>156)edge_keys|=8;
 }
 if(edge_hold>=6)cam_keys|=edge_keys;
 if((keys&1) && !(last_keys&1)) mode^=1;
 if((keys&32) && !(last_keys&32))placement_toggle();
 if((keys&64) && !(last_keys&64))train_start();
 if((keys&128) && !(last_keys&128))production_cancel();
 if(placement_mode) {
  u8 ox=place_x,oy=place_y,ov=place_valid;
  place_x=(cam_x+mouse_x)>>4;place_y=(cam_y+mouse_y)>>4;
  place_valid=mouse_y<160 && placement_valid(place_x,place_y);
  if(ox!=place_x || oy!=place_y || ov!=place_valid)sprites_dirty|=1;
 }
 if((keys&2) && !(last_keys&2)) {
  left_prepare();
  if(left_y>=200 && left_y<208) {
   if(military_selected())combat_stop(left_x>=80);
   else if(WORKER(selected) && left_x>=8 && left_x<232)placement_choose(1+(left_x-8)/32);
   else if(building_selected && buildings[building_selected-1].type==2)train_kind(1+(left_x<80?0:left_x<152?1:2));
   else train_start();
  }
  else if(left_y<160) {if(placement_mode){place_x=left_wx>>4;place_y=left_wy>>4;placement_confirm();}else drag_begin();}
 }
 if((keys&4) && !(last_keys&4)) {
  if(drag_down){drag_down=0;sprites_dirty=3;}
  if(placement_mode) {placement_mode=0;hud_dirty=3;sprites_dirty=3;}
  else if(mouse_y>=200 && mouse_y<208)production_cancel();
  else if(mouse_y<160) {
   u8 tx=(cam_x+mouse_x)>>4,ty=(cam_y+mouse_y)>>4;
   if(building_selected && buildings[building_selected-1].state==2 && passable(tx,ty)) {buildings[building_selected-1].rallyx=tx;buildings[building_selected-1].rallyy=ty;}
   else if(!combat_click(cam_x+mouse_x,cam_y+mouse_y))order_at(tx,ty);
  }
 }
 if((keys&8) && !(last_keys&8)) {drag_down=0;building_selected=0;enemy_selected=NONE;selection_mask=(1<<unit_count)-1;selection_update();}
 if((keys&16) && !(last_keys&16)) {
  drag_down=0;
  if(placement_mode)placement_mode=0;else {building_selected=0;enemy_selected=NONE;selection_mask=0;}
  selection_update();
 }
 if(drag_down)drag_tick();
 video_cursor();
 last_keys=keys;
}
u8 hud_field(u8 field,u8 x,u16 y,const c8* value) __banked {
 u8 i,n=0;
 for(i=0;value[i];++i)if(hud_cache[hud_draw][field][i]!=value[i]) {
  video_char(x+i*8,y,value[i]);hud_cache[hud_draw][field][i]=value[i];++hud_char_writes;++n;
 }
 return n;
}
void format_amount(c8* out,u16 value) __banked {
 i8 i;
 for(i=4;i>=0;--i) {out[i]='0'+value%10;value/=10;}
 out[5]=0;for(i=0;i<4 && out[i]=='0';++i)out[i]=' ';
}
u8 hud_render(void) __banked {
 c8 buf[9];u8 n=0,available,p=hud_draw,h,m,sec;
 video_hud_begin();
 if(hud_values[p][0]!=wood) {format_amount(buf,wood);n+=hud_field(0,64,168,buf);hud_values[p][0]=wood;}
 if(hud_values[p][1]!=gold) {format_amount(buf,gold);n+=hud_field(1,192,168,buf);hud_values[p][1]=gold;}
 if(hud_values[p][2]!=game_seconds) {
  h=game_seconds/3600;m=(game_seconds/60)%60;sec=game_seconds%60;
  buf[0]='0'+h/10;buf[1]='0'+h%10;buf[2]=':';
  buf[3]='0'+m/10;buf[4]='0'+m%10;buf[5]=':';buf[6]='0'+sec/10;buf[7]='0'+sec%10;buf[8]=0;
  n+=hud_field(2,64,184,buf);hud_values[p][2]=game_seconds;
 }
 available=population_free();
 if(hud_values[p][3]!=available) {
  buf[0]='0'+available;buf[1]='/';buf[2]='0'+population_limit();buf[3]=0;
  n+=hud_field(3,192,184,buf);hud_values[p][3]=available;
 }
 hud_actions();video_hud_end();
 return n;
}
void hud_init(void) __banked {
 u8 p,f,i;
 for(p=0;p<2;++p)for(f=0;f<2;++f) {hud_markers[p][f]=0;hud_progress[p][f]=255;}
 for(p=0;p<2;++p)for(f=0;f<4;++f) {hud_values[p][f]=65535;for(i=0;i<8;++i)hud_cache[p][f][i]=0;}
 for(p=0;p<2;++p) {hud_draw=p;hud_render();}
 video_hud_flush();hud_live=hud_ready=hud_pending=hud_dirty=0;hud_draw=0;
}
void hud(void) __banked {
 u8 bit;
 if(!hud_dirty || hud_pending)return;
 hud_draw=hud_live^1;bit=hud_draw?2:1;
 if(!(hud_dirty&bit))return;
 hud_render();video_hud_flush();hud_dirty&=~bit;
 hud_ready=hud_draw;hud_pending=1;
}
void selection_update(void) __banked {
 u8 i;
 selected=NONE; selection_count=0;
 for(i=0;i<unit_count;++i) if((selection_mask&(1<<i)) && unit_hp[i]) {
  if(selected==NONE)selected=i;
  ++selection_count;
 }
 sprites_dirty=3;hud_dirty=3;
}
void select_at(u16 wx,u16 wy,u8 additive) __banked {
 u8 i,hit=NONE,bid=NONE;
 building_selected=0;enemy_selected=combat_hit_test(wx,wy);
 if(enemy_selected!=NONE) {selection_mask=0;selection_update();return;}
 for(i=0;i<building_count;++i) {
  Building* bl=&buildings[i];u8 n=building_size(bl->type);
  if(!building_owner[i] && bl->state && wx/16>=bl->x && wx/16<bl->x+n && wy/16>=bl->y && wy/16<bl->y+n)bid=i;
 }
 for(i=0;i<unit_count;++i)if(unit_position(i)) {
  const u8* bb=pose_bounds+(u16)POSE_KEY(i)*4;
  i16 ux=unit_draw_x+cam_x,uy=unit_draw_y+cam_y;
  if((i16)wx>=ux+bb[0] && (i16)wx<ux+bb[0]+bb[2] && (i16)wy>=uy+bb[1] && (i16)wy<uy+bb[1]+bb[3] && (bid==NONE || units[i].py+16>(u16)(buildings[bid].y+building_size(buildings[bid].type))*16)) {
   if(hit==NONE || units[i].py>=units[hit].py)hit=i;
  }
 }
 if(hit==NONE && bid!=NONE) {selection_mask=0;building_selected=bid+1;selection_update();return;}
 if(!additive)selection_mask=0;if(hit!=NONE)selection_mask^=1<<hit;selection_update();
}
u8 goal_free(u8 x,u8 y,u8 id,u8 assigned) __banked {
 u8 j;
 if(!free_at(x,y,id))return 0;
 for(j=0;j<unit_count;++j)if(j!=id && ((assigned&(1<<j)) || !(selection_mask&(1<<j)))) {
  if(assigned&(1<<j)) {
   u8 gap=(kind_sizes[unit_kind[id]]+kind_sizes[unit_kind[j]]+31)/32;
   i16 dx=(i16)units[j].tx-x,dy=(i16)units[j].ty-y;if(dx<0)dx=-dx;if(dy<0)dy=-dy;if(dx<gap && dy<gap)return 0;
  } else if((units[j].state==PLANNING || units[j].state==MOVING) && units[j].tx==x && units[j].ty==y)return 0;
 }
 return 1;
}
void order_at(u8 x,u8 y) __banked {
 u8 i,assigned=0,r,found;
 i8 dx,dy;
 i16 gx,gy;
 if(!selection_mask)return;
 enemy_selected=NONE;for(i=0;i<unit_count;++i)if(unit_hp[i] && (selection_mask&(1<<i))) {combat_target[i]=NONE;stance[i]=0;auto_hx[i]=255;if(WORKER(i)){repair_target[i]=NONE;repair_timer[i]=0;repair_active&=~(1<<i);}}
 if((fog_at(x,y)==2 && building_order(x,y)) || economy_order(x,y))return;
 ++group_orders;
 for(i=0;i<unit_count;++i)if(unit_hp[i] && (selection_mask&(1<<i))) {
  poll_input();found=0;
  for(r=0;r<=4 && !found;++r)for(dy=-(i8)r;dy<=(i8)r && !found;++dy)for(dx=-(i8)r;dx<=(i8)r;++dx) {
   if(dx!=-(i8)r && dx!=(i8)r && dy!=-(i8)r && dy!=(i8)r)continue;
   if(!dx)poll_input();gx=(i16)x+dx;gy=(i16)y+dy;
   if(gx<0 || gx>=32 || gy<0 || gy>=32)continue;
   if(!goal_free(gx,gy,i,assigned))continue;
   if(unit_build[i]!=NONE) {buildings[unit_build[i]].worker=NONE;unit_build[i]=NONE;builder_working&=~(1<<i);}
   if(WORKER(i))worker_task[i]=0;
   order_one(i,gx,gy);assigned|=1<<i;found=1;break;
  }
  /* Preserve a previous in-flight order if no safe destination exists. */
  if(!found && !units[i].progress && units[i].state!=MOVING && units[i].state!=PLANNING)units[i].state=BLOCKED;
 }
}
u8 economy_order(u8 x,u8 y) __banked {
 u8 t=tile(x,y),i,a=art_tile(x,y);
 if(t==0 && a>=29 && a<=58) {
  a=1+(a-29)%15;
  if(a&8)++y;
  else if(a&4) {--x;++y;}
  else if(!(a&2))--x;
  t=tile(x,y);
 }
 if(!fog_at(x,y) || (t!=1 && t!=4 && t!=5))return 0;
 if(t==4) {x=9;y=7;}
 ++group_orders;
 for(i=0;i<unit_count;++i)if(WORKER(i) && (selection_mask&(1<<i))) {
  if(t==5 && !cargo[i])continue;
  if(unit_build[i]!=NONE) {buildings[unit_build[i]].worker=NONE;unit_build[i]=NONE;builder_working&=~(1<<i);}
  work_stop(i);work_timer[i]=0;work_tries[i]=0;
  if(t==5) {worker_kind[i]=0;worker_task[i]=WAIT_BASE;}
  else {
   node_x[i]=x;node_y[i]=y;worker_kind[i]=t==1?1:2;
   worker_task[i]=cargo[i]?WAIT_BASE:WAIT_NODE;
  }
 }
 return 1;
}
