#include "bay.h"
#include "bay_bonus.h"
#include "combat_assets.h"
#include "campaign.h"
#include "game.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
extern u16 ca_x,ca_r,ca_wx,ca_wy;
u8 BonusList(u8* idx);
static void BayStaticPut(DrawObject* out,u8* n,u16 x,u16 y,u8 f)
{
 DrawObject* d;const SpriteDef* sp=sprites[8]+f;if((i16)(x+SPR_OX(sp)-camera_px)<=-(i16)sp->w || (i16)(x+SPR_OX(sp)-camera_px)>=256 || (i16)(y+SPR_OY(sp)-camera_py)<=-(i16)sp->h || (i16)(y+SPR_OY(sp)-camera_py)>=(hardware_scroll?192:176))return;
 if(*n>=12){bay_overflow=2;return;}d=out+(*n<10?4+*n:*n==10?1:28);++*n;d->x=x;d->y=y;d->frame=f;d->visible=1;
}
void BayStatic(DrawObject* out) __banked
{
 u8 i,n=0;u32 bit=1;const Pickup* p=pickups;
 for(i=4;i<14;++i)out[i].visible=0;out[1].visible=out[28].visible=0;
 {u8 pl[32],k,cnt,m=LEVEL_PICKUPS>32?32:LEVEL_PICKUPS;ca_x=(u16)pickups;ca_r=m;ca_wx=sizeof(Pickup);ca_wy=(u16)&pickup_taken;cnt=BonusList(pl);for(k=0;k<cnt;++k){p=pickups+pl[k];BayStaticPut(out,&n,p->x,p->y,p->frame);}for(i=32,p=pickups+32;i<LEVEL_PICKUPS;++i,++p)BayStaticPut(out,&n,p->x,p->y,p->frame);}
 {u8 list[28],k,cnt;ca_x=(u16)bay_bonus;ca_r=28;ca_wx=5;ca_wy=(u16)&bonus_taken;cnt=BonusList(list);for(k=0;k<cnt;++k){i=list[k];BayStaticPut(out,&n,bay_bonus[i].x,bay_bonus[i].y,94+bay_bonus[i].kind);}}
}
