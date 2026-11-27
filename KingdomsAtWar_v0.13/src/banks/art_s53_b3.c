#include "../shared.h"
#include "../text.h"
#include "../screens.h"
#include "../mission_table.h"
/* Campaign: mission parameters (tools/campaign.py), a new game state for each mission without reloading the assets, and
 * the briefing screen. */
void mission_load(u8 m) __banked {
 u8 i;const u8* src;u8* dst=(u8*)&mis;
 if(m>=MISSIONS)m=MISSIONS-1;
 src=mission_table+(u16)m*16;
 for(i=0;i<16;++i)dst[i]=src[i];
 mission_total=MISSIONS;
}
/* Everything the game state needs to start (again): variables, world, fog, resources. The V9990 side is video_game_reset(). */
void game_reset(u8 m) __banked {
 u8 i;
 mission_n=m;mission_load(m);
 left_seen=0;drag_down=drag_moved=0;end_t0=0;
 raw_command_down=raw_command_latched=0;raw_down=raw_latched=0;raw_dx=raw_dy=0;
 cam_x=cam_y=sim_ticks=accumulator=0;mine_turn=0;economy_retries=alternate_approaches=0;
 mode=last_keys=cam_keys=0;
 job_unit=selected=hud_mode=hud_state=NONE;hud_selection=254;
 mouse_x=mouse_y=56;
 selection_mask=selection_count=nav_cursor=0;group_orders=replans=0;
 for(i=0;i<N;++i)retry_wait[i]=retry_left[i]=0;
 game_seconds=second_phase=hud_dirty=0;hud_live=hud_ready=hud_pending=hud_draw=0;
 {u8 p;for(p=0;p<2;++p)for(i=0;i<30;++i)hud_context[p][i]=hud_action_cache[p][i]=0;} /* the band is redrawn from scratch */
 init_world();fog_init();
 wood=mis.wood;gold=mis.gold;
 {u16 mx=(u16)map_w*16-256,my=(u16)map_h*16-160,cx=(u16)(pb_x>2?pb_x-2:0)*16,cy=(u16)(pb_y>6?pb_y-6:0)*16;cam_x=cx>mx?mx:cx;cam_y=cy>my?my:cy;} /* the camera starts at the castle */
}
/* Briefing: the parchment picture, the mission title and four text lines in the HUD band; Enter or a click starts. */
static u8 tr_len(const c8* s) {u8 n=0;while(s[n])++n;return n;}
void briefing(u8 m) __banked {
 u8 prev=1,i;u16 t;
 if(menu_auto)return;
 mission_n=m;mission_load(m);
 end_show(SCR_MENU);
 for(i=0;i<5;++i)video_text(8,168+i*8,tr(mis.text+i));
 for(;;) {
  u8 k,click;
  t=video_clock();while(video_clock()==t){}
  poll_input();
  raw_dx=raw_dy=0;click=(raw_latched&2)?1:0;raw_latched=0;
  k=(KP(KEY_RET)||KP(KEY_SPACE))?1:0;
  if((k&&!prev)||click)break;
  prev=k;
 }
 /* the picture stays while the mission is prepared; the band says so */
 video_fill(0,168,256,40,1);
 video_text(128-(u8)(4*(u8)tr_len(tr(TXT_LOADING))),184,tr(TXT_LOADING));
}
