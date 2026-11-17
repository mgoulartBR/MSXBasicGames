#include "ticks.h"
#include "apple.h"
#include "apple_spawns.h"
#include "combat_assets.h"
#include "campaign.h"
#include "cart.h"
#include "game.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static void ApplePut(DrawObject* out,u8* count,u16 x,u16 y,u8 frame)
{
 const SpriteDef* sp=sprites[6]+frame;
 if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))return;
 if(*count>=13){apple_overflow=1;return;}out+=(*count)++;out->x=x;out->y=y;out->frame=frame;out->visible=1;
}
void AppleCollect(DrawObject* out) __banked
{
 u8 i,n=0,bit;CombatObject* a;for(i=0;i<13;++i)out[i].visible=0;apple_overflow=0;
 if(!(enemy_mode&2))for(i=0;i<9;++i){a=i<5?apple_actors+i:apple_peas+i-5;if(a->active)ApplePut(out,&n,(a->x>>4)-(a->state==1?0:24),(a->y>>4)-(a->state==1?0:4),a->frame);}
 ApplePut(out,&n,carts[0].x>>4,carts[0].y>>4,84+TickTenthOdd());
 for(i=0,bit=1;i<6;++i,bit<<=1)if(!(apple_bombs&bit) || apple_rise[i])ApplePut(out,&n,(u16)apple_spawn[6+i][0]*16,(u16)apple_spawn[6+i][1]*16-(apple_rise[i]?(40-apple_rise[i])/2:0),apple_rise[i]?89:102+(simulation_ticks/20)%3);
}
