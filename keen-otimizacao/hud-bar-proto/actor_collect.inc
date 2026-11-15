#include "castle.h"
#include "boobus.h"
#include "swamp.h"
// Included twice to eliminate per-actor mode branches. Shared scratch locals.
#if ACTORS_IMMEDIATE
static void COLLECT_NAME(u8 page)
#else
void COLLECT_NAME(u8 page) __banked
#endif
{
    // Update the destination page records directly, in painter order.
    // Walk pointers and a shifting mask: no per-item index multiplication,
    // variable shift, order selection or temporary structure copy.
    if(active_level>=7){if(active_level==12)CastleStatic(actors_next);else if(active_level==11)BoobusStatic(actors_next);else if(active_level==10)SwampStatic(actors_next);else if(active_level==9)BayStatic(actors_next);else if(active_level==8)GroveStatic(actors_next);else AppleStatic(actors_next);if(ACTORS_IMMEDIATE){for(collect_i=4;collect_i<14;++collect_i)if(actors_next[collect_i].visible)ActorDraw(page,actors_next+collect_i);if(actors_next[1].visible)ActorDraw(page,actors_next+1);if(actors_next[28].visible)ActorDraw(page,actors_next+28);}}else{actors_next[28].visible=0;
    collect_ob=actors_next+4;collect_slot=0;
    {u8 idx[32],pj,pk;ScanArgs sa;
    // Z80 window scan: -24 <= x-cam < 256 and -24 <= y-cam < 192.
    sa.items=(const u8*)pickups;sa.out=idx;sa.ox=camera_px-24;sa.oy=camera_py-24;sa.w=280;sa.h=216;
    sa.stride=sizeof(Pickup);sa.n=LEVEL_PICKUPS>32?32:LEVEL_PICKUPS;sa.count=0;
    pk=ScanWin(&sa);
    for(pj=0;pj<pk && collect_slot<10;++pj){
        collect_i=idx[pj];
        if((pickup_taken>>collect_i)&1)continue;
        collect_pk=pickups+collect_i;
        collect_ob->visible=1;collect_ob->x=collect_pk->x;collect_ob->y=collect_pk->y;collect_ob->frame=collect_pk->frame;if(ACTORS_IMMEDIATE)ActorDraw(page,collect_ob);++collect_ob;++collect_slot;
    }}
    collect_i=BonusCollect(collect_ob,10-collect_slot);
    collect_slot+=collect_i;
    while(collect_i--){if(ACTORS_IMMEDIATE)ActorDraw(page,collect_ob);++collect_ob;}
    while(collect_slot++<10){collect_ob->visible=0;++collect_ob;}
    collect_ob=actors_next+1;
    collect_ob->x=enemy.x>>4;collect_ob->y=enemy.y>>4;collect_ob->frame=enemy.frame;collect_ob->visible=enemy.active && !(enemy_mode&2);
    if(collect_ob->visible){if(ACTORS_IMMEDIATE)ActorDraw(page,collect_ob);}
    }
    if(active_level==1 || active_level==3){
        if(active_level==1)HillCollect(actors_next+15);else BridgeCollect(actors_next+15);
        // Match ActorsDraw's painter order on both full and partial redraws.
        if(ACTORS_IMMEDIATE){
            for(collect_i=20;collect_i<28;++collect_i){collect_ob=actors_next+collect_i;if(collect_ob->visible)ActorDraw(page,collect_ob);}
            for(collect_i=15;collect_i<20;++collect_i){collect_ob=actors_next+collect_i;if(collect_ob->visible)ActorDraw(page,collect_ob);}
        }
    }else{
    if(active_level==12)CastleCollect(actors_next+15);else if(active_level==11)BoobusCollect(actors_next+15);else if(active_level==10)SwampCollect(actors_next+15);else if(active_level==9)BayCollect(actors_next+15);else if(active_level==8)GroveCollect(actors_next+15);else if(active_level==7)AppleCollect(actors_next+15);else if(active_level==6)CityCollect(actors_next+15);else if(active_level==5)PassCollect(actors_next+15);else if(active_level==4)RapidCollect(actors_next+15);else MinesCollect(actors_next+15);
    if(ACTORS_IMMEDIATE){
        for(collect_i=20;collect_i<28;++collect_i){collect_ob=actors_next+collect_i;if(collect_ob->visible)ActorDraw(page,collect_ob);}
        for(collect_i=15;collect_i<20;++collect_i){collect_ob=actors_next+collect_i;if(collect_ob->visible)ActorDraw(page,collect_ob);}
    }
    }
    if(active_level<6){actors_next[26].visible=actors_next[27].visible=0;}
    collect_ob=actors_next;collect_ob->x=player_x>>4;collect_ob->y=player_y>>4;collect_ob->frame=hero_frame;collect_ob->visible=hero_visible && !hw->visible[page];
    if(collect_ob->visible){if(ACTORS_IMMEDIATE)ActorDraw(page,collect_ob);if(ACTORS_IMMEDIATE)ForegroundHero(page);}
    collect_ob+=2;collect_shot=shots;
    for(collect_i=0;collect_i<2;++collect_i,++collect_ob,++collect_shot){
        collect_ob->visible=collect_shot->active && !(collect_shot->state==2 && (collect_shot->timer/5&1));
        collect_ob->x=collect_shot->x>>4;collect_ob->y=collect_shot->y>>4;collect_ob->frame=collect_shot->frame;
        if(collect_ob->visible)if(ACTORS_IMMEDIATE)ActorDraw(page,collect_ob);
    }
    collect_ob=actors_next+14;collect_ob->x=camera_px+168;collect_ob->y=camera_py+4;collect_ob->frame=35;collect_ob->visible=bar_on?0:1;
    if(ACTORS_IMMEDIATE)HudDraw(page);
}

