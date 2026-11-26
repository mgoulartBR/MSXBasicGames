#include "../shared.h"
/* Title and main menu (first piece of the game shell). Texts are uppercase ASCII for now: the 8x8 font has no accents or
 * kana yet (decision pending); the Japanese column is romanised until the kana font exists. */
#define LANGS 4
enum {T_PRESS,T_NEW,T_LANG,T_TITLE,T_SUB,T_COUNT};
static const c8* const txt[LANGS][T_COUNT]={
 {"PRESSIONE ENTER","NOVO JOGO","IDIOMA","REINOS","OS INSEPULTOS"},
 {"PRESS ENTER","NEW GAME","LANGUAGE","KINGDOMS","THE UNBURIED"},
 {"PULSE ENTER","NUEVO JUEGO","IDIOMA","REINOS","LOS INSEPULTOS"},
 {"ENTA O OSHITE","HAJIMERU","GENGO","KOKUOU","SHIBARERU MONO"}
};
static const c8* const lang_name[LANGS]={"PORTUGUES","ENGLISH","ESPANOL","NIHONGO"};
#define ITEM_Y 176
static u8 slen(const c8* s) {u8 n=0;while(s[n])++n;return n;}
static void band_clear(void) {video_fill(0,168,256,40,1);}
static void draw_title(void) {
 band_clear();
 video_text(128-(u8)(slen(txt[lang][T_TITLE])*4),176,txt[lang][T_TITLE]);
 video_text(128-(u8)(slen(txt[lang][T_PRESS])*4),192,txt[lang][T_PRESS]);
}
static void draw_items(const c8* const* items,u8 n,u8 sel) {
 u8 i;
 band_clear();
 for(i=0;i<n;++i) {
  video_text(72,ITEM_Y+i*8,i==sel?"*":" ");
  video_text(88,ITEM_Y+i*8,items[i]);
 }
}
void menu_run(void) __banked {
 u8 state=0,sel=0,prev=0,dirty=1,n;
 u16 t;
 const c8* items[LANGS];
 if(menu_auto)return;
 for(;;) {
  u8 k,edge,click,moved;
  if(dirty) {
   if(state==0)draw_title();
   else if(state==1) {items[0]=txt[lang][T_NEW];items[1]=txt[lang][T_LANG];draw_items(items,2,sel);}
   else {u8 i;for(i=0;i<LANGS;++i)items[i]=lang_name[i];draw_items(items,LANGS,sel);}
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
