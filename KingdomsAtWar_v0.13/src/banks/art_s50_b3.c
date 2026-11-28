#include "../shared.h"
/* Title and main menu (first piece of the game shell). All texts come from tr() (tools/i18n.py). */
#include "../text.h"
#include "../screens.h"
#define ITEM_Y 176
extern u8 music_cur;
static u8 slen(const c8* s) {u8 n=0;while(s[n])++n;return n;}
static void band_clear(void) {video_fill(0,168,256,40,1);}
static void draw_title(void) {
 band_clear();
 video_text(128-(u8)(slen(tr(TXT_TITLE))*4),176,tr(TXT_TITLE));
 video_text(128-(u8)(slen(tr(TXT_PRESS))*4),192,tr(TXT_PRESS));
}
/* Option lines are built from label + value; without an MSX-MUSIC chip only a notice and "back" are offered. */
static void opt_line(c8* o,u8 i) {
 const c8* l;u8 k=0;
 o[0]=0;
 if(i==3) {l=tr(TXT_OPT_BACK);while(*l && k<22)o[k++]=*l++;o[k]=0;return;}
 if(!fm_found) {if(i==0){l=tr(TXT_OPT_NOFM);while(*l && k<22)o[k++]=*l++;o[k]=0;}return;}
 l=tr(i==0?TXT_OPT_MUSIC:i==1?TXT_OPT_SFX:TXT_OPT_VOLUME);while(*l && k<12)o[k++]=*l++;
 while(k<11)o[k++]=' '; /* values in one column */
 if(i==2) {o[k++]='0'+opt_vol;}
 else {l=tr((i==0?opt_music:opt_sfx)?TXT_OPT_ON:TXT_OPT_OFF);while(*l && k<22)o[k++]=*l++;}
 o[k]=0;
}
/* state 1 main, 2 language, 3 options */
static void draw_items(u8 state,u8 n,u8 sel) {
 u8 i;c8 line[24];
 band_clear();
 for(i=0;i<n;++i) {
  video_text(72,ITEM_Y+i*8,i==sel?"*":" ");
  if(state==3) {opt_line(line,i);video_text(88,ITEM_Y+i*8,line);}
  else video_text(88,ITEM_Y+i*8,tr(state==1?(i==0?TXT_NEW_GAME:i==1?TXT_OPTIONS:TXT_LANGUAGE):(u8)(TXT_LANG_PT+i)));
 }
}
static void sound_changed(u8 what) { /* 0 music switch, 1 volume */
 if(what==0) {if(opt_music) {music_cur=255;music_play(SONG_TITLE);} else music_stop();}
 else if(music_cur!=255) {u8 c=music_cur;music_cur=255;music_play(c);}
}
static void opt_change(u8 item,i8 dir) { /* dir: +1/-1 from the arrows, 0 = activate */
 if(!fm_found && item<3)return;
 if(item==0) {opt_music^=1;sound_changed(0);}
 else if(item==1) {opt_sfx^=1;if(opt_sfx)sfx_play(SFX_CONFIRM);}
 else if(item==2) {
  if(dir>=0)opt_vol=opt_vol>=4?1:opt_vol+1;else opt_vol=opt_vol<=1?4:opt_vol-1;
  sound_changed(1);sfx_play(SFX_CLICK);
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
   else if(state==1)draw_items(1,3,sel);
   else if(state==3)draw_items(3,4,sel);
   else draw_items(2,LANGS,sel);
   dirty=0;
  }
  t=video_clock();while(video_clock()==t){}
  poll_input();
  moved=(raw_dx||raw_dy);
  {i16 x=(i16)mouse_x+raw_dx,y=(i16)mouse_y+raw_dy;raw_dx=raw_dy=0;
   if(x<0)x=0;if(x>248)x=248;if(y<0)y=0;if(y>204)y=204;mouse_x=(u8)x;mouse_y=(u8)y;}
  click=(raw_latched&2)?1:0;raw_latched=0;
  k=(KP(KEY_UP)?1:0)|(KP(KEY_DOWN)?2:0)|((KP(KEY_RET)||KP(KEY_SPACE))?4:0)|(KP(KEY_ESC)?8:0)|(KP(KEY_LEFT)?16:0)|(KP(KEY_RIGHT)?32:0);
  edge=k&~prev;prev=k;
  if(state==0) {
   if((edge&4)||click){state=1;sel=0;dirty=1;}
   continue;
  }
  n=(state==1)?3:(state==3)?4:LANGS;
  if(edge&1){sel=sel?sel-1:n-1;dirty=1;}
  if(edge&2){sel=(sel+1>=n)?0:sel+1;dirty=1;}
  if(moved && mouse_y>=ITEM_Y && mouse_y<ITEM_Y+n*8) {u8 h=(u8)((mouse_y-ITEM_Y)>>3);if(h!=sel){sel=h;dirty=1;}}
  if(state==3 && (edge&48) && sel<3) {opt_change(sel,(edge&16)?-1:1);dirty=1;}
  if((edge&4)||(click && mouse_y>=ITEM_Y && mouse_y<ITEM_Y+n*8)) {
   if(state==1) {
    if(sel==0)return;
    if(sel==1){state=3;sel=0;}
    else {state=2;sel=lang;}
    dirty=1;
   } else if(state==3) {
    if(sel==3){state=1;sel=1;}
    else opt_change(sel,0);
    dirty=1;
   } else {lang=sel;state=1;sel=2;dirty=1;}
  }
  if((edge&8) && state>=2){sel=(state==3)?1:2;state=1;dirty=1;}
 }
}
/* Victory / defeat screen: the picture of the side being played (side 0 humans, 1 the Unburied), a headline and the next step.
 * Returns 1 next mission, 2 retry, 3 back to the title (after the last mission the end-of-campaign picture comes first);
 * 0 if the game state was cleared meanwhile (tests): the screen is removed and play continues. */
u8 end_screen(void) __banked {
 u8 code=game_over,id,prev=1,last=(u8)(mission_n+1>=mission_total),hint;
 u16 t;
 id=(code==1)?(side?SCR_VICTORY_UNDEAD:SCR_VICTORY_HUMAN):(side?SCR_DEFEAT_UNDEAD:SCR_DEFEAT_HUMAN);
 hint=(code==1)?(last?TXT_END_MENU:TXT_END_NEXT):TXT_END_RETRY;
 end_show(id);
 video_text(128-(u8)(slen(tr(code==1?TXT_END_VICTORY:TXT_END_DEFEAT))*4),176,tr(code==1?TXT_END_VICTORY:TXT_END_DEFEAT));
 video_text(128-(u8)(slen(tr(hint))*4),192,tr(hint));
 for(;;) {
  u8 k;
  t=video_clock();while(video_clock()==t){}
  poll_input();
  raw_dx=raw_dy=0;raw_latched=0;
  k=KP(KEY_RET)?1:0;
  if(KP(KEY_ESC))return 3;
  if(k && !prev) {
   if(code==2)return 2;
   if(!last)return 1;
   /* the campaign is complete: the closing picture, then the title */
   end_show(side?SCR_END_UNDEAD:SCR_END_HUMAN);
   video_text(128-(u8)(slen(tr(TXT_END_CAMPAIGN))*4),176,tr(TXT_END_CAMPAIGN));
   video_text(128-(u8)(slen(tr(TXT_END_MENU))*4),192,tr(TXT_END_MENU));
   prev=1;
   for(;;) {
    t=video_clock();while(video_clock()==t){}
    poll_input();raw_dx=raw_dy=0;raw_latched=0;
    k=KP(KEY_RET)?1:0;
    if(k && !prev)return 3;
    prev=k;
   }
  }
  prev=k;
  if(game_over!=code) {end_hide();video_hud_band();hud_init();hud_dirty=3;return 0;}
 }
}
