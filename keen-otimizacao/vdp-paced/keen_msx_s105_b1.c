#include "render_fronts.h"
// Mines-only exact hybrid path. Hardware candidates never overlap another actor.
#include "game.h"
#include "combat.h"
#include "combat_assets.h"
#include "tomato.h"
#include "hw_tomato.h"
#include "hw_compact_hero.h"
#include "hw_tomato_data.h"
#include "hw_sprite_rows.h"
void VP(const u8* src,u16 dst,u8 page,u16 n) __banked;
u8 hw_tomato_second[2],hw_tomato_second_cached[2];
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
void HeroCompact0(u8 f,u16 base,u16 patterns) __banked;
void HeroCompact1(u8 f,u16 base,u16 patterns) __banked;
void HeroCompact2(u8 f,u16 base,u16 patterns) __banked;
void HeroCompact3(u8 f,u16 base,u16 patterns) __banked;
void HeroCompact4(u8 f,u16 base,u16 patterns) __banked;

#if MULTI_HW_TOMATO
void HWTomatoPrepare(u8 page,DrawObject* next) __banked
{
 u8 i,j,k,count=hw->visible[page]?compact_count[hw->frame]:0;
 DrawObject* a;DrawObject* b;Tomato* t;const SpriteDef* sp;const SpriteDef* other;
 i16 x,y,bx,by,sx,sy;
 hw_tomato[page]=hw_tomato_second[page]=255;
 if(!hardware_scroll || (enemy_mode&2))return;
 for(i=15;i<26;++i){
  a=next+i;if(!a->visible || a->frame<90 || a->frame>93)continue;
  k=a->frame-90;if(count+hw_count[k]>32)continue;
  sp=&sprites[1][a->frame];x=a->x;y=a->y;
  sx=x-camera_px;sy=y-camera_py;
  if(sx<0 || sx>232 || sy<20 || sy>160)continue;
  // The hero can consume all eight slots on each occupied scanline.
  if(hw->visible[page] && y<(i16)(player_y>>4)+33 && (i16)(player_y>>4)<y+32)continue;
  if(hw_tomato[page]!=255){
   Tomato* first=tomatoes+hw_tomato[page];
   i16 first_y=first->y>>4;
   if(y<first_y+32 && first_y<y+32)continue;
  }
  for(j=0;j<ACTOR_COUNT;++j){
   if(j==i)continue;b=next+j;if(!b->visible)continue;
   other=&sprites[1][b->frame];bx=b->x+SPR_OX(other);by=b->y+SPR_OY(other);
   if(x<bx+other->w && bx<x+24 && y<by+other->h && by<y+32)break;
  }
  if(j<ACTOR_COUNT)continue;
  for(j=0,t=tomatoes;j<TOMATO_COUNT;++j,++t)
   if(t->active && t->frame==a->frame && (t->x>>4)==a->x && (t->y>>4)==a->y)break;
  if(j==TOMATO_COUNT)continue;
  a->visible=0;count+=hw_count[k];
  if(hw_tomato[page]==255)hw_tomato[page]=j;
  else{hw_tomato_second[page]=j;return;}
 }
}

void HWMinesUpload(u8 page) __banked
{
 u8 i,f=hw->frame,count=0,k,id=hw_tomato[page];
 u8 table=(hw->bitmap_recent>>6)&1;
 u16 base=table?0x1400:0x0c00,patterns=(u16)table*2048;
 i16 x,y;Tomato* t;
 if(hw->visible[page]){
  count=compact_count[f];
  if(hw->cached[table]!=f){
   VDP_CommandWait();
   if(f<12)HeroCompact0(f,base,patterns);
   else if(f<24)HeroCompact1(f-12,base,patterns);
   else if(f<36)HeroCompact2(f-24,base,patterns);
   else if(f<48)HeroCompact3(f-36,base,patterns);
   else HeroCompact4(f-48,base,patterns);
   hw->cached[table]=f;
   // Hero color upload includes unused slots, so reload appended colors.
   hw_tomato_cached[table]=hw_tomato_second_cached[table]=255;
  }
  for(i=0;i<count;++i){
   hw_attributes[i*4]=hw->y+compact_xy[f][i*2+1]-1;
   hw_attributes[i*4+1]=hw->x+compact_xy[f][i*2];
   hw_attributes[i*4+2]=i*4;hw_attributes[i*4+3]=0;
  }
 }
 {
  u8 which,tag;
  for(which=0;which<2;++which){
   if(!MULTI_HW_TOMATO && which)break;
   id=which?hw_tomato_second[page]:hw_tomato[page];if(id==255)continue;
   t=tomatoes+id;k=t->frame-90;tag=k|(count<<2);
   if((which?hw_tomato_second_cached[table]:hw_tomato_cached[table])!=tag){
    VDP_CommandWait();
    VP(hw_patterns[k],patterns+768+(u16)which*448,1,448);
    VP(hw_colors[k],base+1024+(u16)count*16,1,(u16)hw_count[k]*16);
    if(which)hw_tomato_second_cached[table]=tag;else hw_tomato_cached[table]=tag;
    if(!hw->visible[page])hw->cached[table]=255;
   }
   x=(t->x>>4)-camera_px;y=t->y>>4;
   for(i=0;i<hw_count[k];++i){
    hw_attributes[(count+i)*4]=y+hw_xy[k][i*2+1]-1;
    hw_attributes[(count+i)*4+1]=x+hw_xy[k][i*2];
    hw_attributes[(count+i)*4+2]=96+which*56+i*4;hw_attributes[(count+i)*4+3]=0;
   }
   count+=hw_count[k];
  }
 }
 hw_attributes[count*4]=216;
 VDP_CommandWait();
 VP(hw_attributes,base+0x600,1,(u16)count*4+1);
}

#else
void HWTomatoPrepare(u8 page,DrawObject* next) __banked
{
 u8 i,j,k,count=hw->visible[page]?compact_count[hw->frame]:0;
 DrawObject* a;DrawObject* b;Tomato* t;const SpriteDef* sp;const SpriteDef* other;
 i16 x,y,bx,by,sx,sy;
 hw_tomato[page]=255;
 if(!hardware_scroll || (enemy_mode&2))return;
 for(i=15;i<26;++i){
  a=next+i;if(!a->visible || a->frame<90 || a->frame>93)continue;
  k=a->frame-90;if(count+hw_count[k]>32)continue;
  sp=&sprites[1][a->frame];x=a->x;y=a->y;
  sx=x-camera_px;sy=y-camera_py;
  if(sx<0 || sx>232 || sy<20 || sy>160)continue;
  // The hero can consume all eight slots on each occupied scanline.
  if(hw->visible[page] && y<(i16)(player_y>>4)+33 && (i16)(player_y>>4)<y+32)continue;
  for(j=0;j<ACTOR_COUNT;++j){
   if(j==i)continue;b=next+j;if(!b->visible)continue;
   other=&sprites[1][b->frame];bx=b->x+SPR_OX(other);by=b->y+SPR_OY(other);
   if(x<bx+other->w && bx<x+24 && y<by+other->h && by<y+32)break;
  }
  if(j<ACTOR_COUNT)continue;
  for(j=0,t=tomatoes;j<TOMATO_COUNT;++j,++t)
   if(t->active && t->frame==a->frame && (t->x>>4)==a->x && (t->y>>4)==a->y)break;
  if(j==TOMATO_COUNT)continue;
  hw_tomato[page]=j;a->visible=0;return;
 }
}

void HWMinesUpload(u8 page) __banked
{
 u8 i,f=hw->frame,count=0,k,id=hw_tomato[page];
 u8 table=(hw->bitmap_recent>>6)&1;
 u16 base=table?0x1400:0x0c00,patterns=(u16)table*2048;
 i16 x,y;Tomato* t;
 if(hw->visible[page]){
  count=compact_count[f];
  if(hw->cached[table]!=f){
   VDP_CommandWait();
   if(f<12)HeroCompact0(f,base,patterns);
   else if(f<24)HeroCompact1(f-12,base,patterns);
   else if(f<36)HeroCompact2(f-24,base,patterns);
   else if(f<48)HeroCompact3(f-36,base,patterns);
   else HeroCompact4(f-48,base,patterns);
   hw->cached[table]=f;
   // Hero color upload includes unused slots, so reload appended colors.
   hw_tomato_cached[table]=255;
  }
  for(i=0;i<count;++i){
   hw_attributes[i*4]=hw->y+compact_xy[f][i*2+1]-1;
   hw_attributes[i*4+1]=hw->x+compact_xy[f][i*2];
   hw_attributes[i*4+2]=i*4;hw_attributes[i*4+3]=0;
  }
 }
 if(id!=255){
  t=tomatoes+id;k=t->frame-90;
  if(hw_tomato_cached[table]!=(k|(count<<2))){
   VDP_CommandWait();
   VP(hw_patterns[k],patterns+768,1,448);
   VP(hw_colors[k],base+1024+(u16)count*16,1,(u16)hw_count[k]*16);
   hw_tomato_cached[table]=k|(count<<2);
   if(!hw->visible[page])hw->cached[table]=255;
  }
  x=(t->x>>4)-camera_px;
  y=t->y>>4;
  for(i=0;i<hw_count[k];++i){
   hw_attributes[(count+i)*4]=y+hw_xy[k][i*2+1]-1;
   hw_attributes[(count+i)*4+1]=x+hw_xy[k][i*2];
   hw_attributes[(count+i)*4+2]=96+i*4;
   hw_attributes[(count+i)*4+3]=0;
  }
  count+=hw_count[k];
 }
 hw_attributes[count*4]=216;
 VDP_CommandWait();
 VP(hw_attributes,base+0x600,1,(u16)count*4+1);
}

#endif
