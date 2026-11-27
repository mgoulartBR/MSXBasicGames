#include "../shared.h"
/* Title and main menu (first piece of the game shell). All texts come from tr() (tools/i18n.py). */
#include "../text.h"
#include "../screens.h"
#define ITEM_Y 176
static u8 slen(const c8* s) {u8 n=0;while(s[n])++n;return n;}
static void band_clear(void) {video_fill(0,168,256,40,1);}
static void draw_title(void) {
 band_clear();
 video_text(128-(u8)(slen(tr(TXT_TITLE))*4),176,tr(TXT_TITLE));
 video_text(128-(u8)(slen(tr(TXT_PRESS))*4),192,tr(TXT_PRESS));
}
static void draw_items(u8 state,u8 n,u8 sel) {
 u8 i;
 band_clear();
 for(i=0;i<n;++i) {
  u8 id=(state==1)?(i==0?TXT_NEW_GAME:TXT_LANGUAGE):(u8)(TXT_LANG_PT+i);
  video_text(72,ITEM_Y+i*8,i==sel?"*":" ");
  video_text(88,ITEM_Y+i*8,tr(id));
 }
}
void menu_run(void) __banked {
 u8 state=0,sel=0,prev=0,dirty=1,n;
 u16 t;
 if(menu_auto)return;
 for(;;) {
  u8 k,edge,click,moved;
  if(dirty) {
   if(state==0)draw_title();
   else if(state==1)draw_items(1,2,sel);
   else draw_items(2,LANGS,sel);
   dirty=0;
  }
  t=video_clock();while(video_clock()==t){}
  poll_input();
  moved=(raw_dx||raw_dy);
  {i16 x=(i16)mouse_x+raw_dx,y=(i16)mouse_y+raw_dy;raw_dx=raw_dy=0;
   if(x<0)x=0;if(x>248)x=248;if(y<0)y=0;if(y>204)y=204;mouse_x=(u8)x;mouse_y=(u8)y;}
  click=(raw_latched&2)?1:0;raw_latched=0;
  k=(KP(KEY_UP)?1:0)|(KP(KEY_DOWN)?2:0)|((KP(KEY_RET)||KP(KEY_SPACE))?4:0)|(KP(KEY_ESC)?8:0);
  edge=k&~prev;prev=k;
  if(state==0) {
   if((edge&4)||click){state=1;sel=0;dirty=1;}
   continue;
  }
  n=(state==1)?2:LANGS;
  if(edge&1){sel=sel?sel-1:n-1;dirty=1;}
  if(edge&2){sel=(sel+1>=n)?0:sel+1;dirty=1;}
  if(moved && mouse_y>=ITEM_Y && mouse_y<ITEM_Y+n*8) {u8 h=(u8)((mouse_y-ITEM_Y)>>3);if(h!=sel){sel=h;dirty=1;}}
  if((edge&4)||(click && mouse_y>=ITEM_Y && mouse_y<ITEM_Y+n*8)) {
   if(state==1) {
    if(sel==0)return;
    state=2;sel=lang;dirty=1;
   } else {lang=sel;state=1;sel=1;dirty=1;}
  }
  if((edge&8) && state==2){state=1;sel=1;dirty=1;}
 }
}
/* Victory / defeat screen: the picture of the side being played (side 0 humans, 1 the Unburied), a headline and the restart
 * hint. Enter restarts the game (back to the title); if the game state is cleared meanwhile (tests) the screen is removed. */
void end_screen(void) __banked {
 u8 code=game_over,id,prev=1;
 u16 t;
 id=(code==1)?(side?SCR_VICTORY_UNDEAD:SCR_VICTORY_HUMAN):(side?SCR_DEFEAT_UNDEAD:SCR_DEFEAT_HUMAN);
 end_show(id);
 video_text(128-(u8)(slen(tr(code==1?TXT_END_VICTORY:TXT_END_DEFEAT))*4),176,tr(code==1?TXT_END_VICTORY:TXT_END_DEFEAT));
 video_text(128-(u8)(slen(tr(TXT_AGAIN))*4),192,tr(TXT_AGAIN));
 for(;;) {
  u8 k;
  t=video_clock();while(video_clock()==t){}
  poll_input();
  raw_dx=raw_dy=0;raw_latched=0;
  k=KP(KEY_RET)?1:0;
  if(k && !prev)game_restart();
  prev=k;
  if(game_over!=code) {end_hide();video_hud_band();hud_init();hud_dirty=3;return;}
 }
}
