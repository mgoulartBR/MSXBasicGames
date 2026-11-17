// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from Keen Dreams: kd_keen.c, kd_act1.c and kd_play.c.
// Coordinates/speeds use the original 1/16 pixel units. One tick = 1/70 s.
#include "game.h"
#include "world.h"
#include "combat.h"
#include "keydoor.h"
u16 player_x, player_y;
volatile u16 simulation_ticks;
i16 player_vx, player_vy;
u8 grounded, facing_left, player_frame, floor_type, jump_hold;
volatile u16 wall_hits, ceiling_hits, slope_hits, respawns;
static u8 jump_previous, walk_time;
volatile u16 stable_x,stable_y,stable_ticks;
volatile u8 stable_grounded,stable_frame;
static void Publish(void)
{
    stable_x=player_x;stable_y=player_y;stable_grounded=grounded;
    stable_frame=player_frame;stable_ticks=simulation_ticks;
}

static const u8 walk_frame[24]={1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,4};
static const i8 slope_speed[8]={0,0,4,4,8,-4,-4,-8};
#ifndef GAME_MAP
#define GAME_MAP world_map
#define GAME_COLLISION world_collision
#endif
static u8 Wall(u8 x,u8 y,u8 side)
{
    u16 tile;
    if(x>=MAP_WIDTH || y>=MAP_HEIGHT) return 1;
    tile=MapTile((u16)y*MAP_WIDTH+x);
    return GAME_COLLISION[352+GAME_COLLISION[tile]*4+side];
}
// Exact wallclip heights from kd_play.c, including half-pixel slopes.
static u16 Height(u8 shape,u8 x)
{
    switch(shape&7)
    {
        case 0: return 256;
        case 1: return 0;
        case 2: return (u16)x*8;
        case 3: return 128+(u16)x*8;
        case 4: return (u16)x*16;
        case 5: return 120-(u16)x*8;
        case 6: return 248-(u16)x*8;
        default:return 240-(u16)x*16;
    }
}
u8 level_done;
#include "exit_zones.h"
static u8 ExitBoundary(u8 right)
{
 u8 i;const u8* zone;
 if(active_level<1 || active_level>12)return 0;
 for(i=0;i<3;++i){
  zone=exit_zones[active_level-1][i];
  if(zone[0]==right && player_y+48<(u16)(zone[1]+2)*256 && player_y+496>(u16)zone[1]*256)return 1;
 }
 return 0;
}
// KeyDoorTick does nothing in levels 1, 3, 9, 10 and 11: skip the bank call.
#define KEYDOOR_TICK() do{ if(!(active_level==1||active_level==3||active_level==9||active_level==10||active_level==11))KeyDoorTick(); }while(0)
#include "poles.c"
#include "cart.c"
// Actor side push is a displacement; it must not advance the simulation.
void GamePushSide(i16 dx)
{
 u16 ox=player_x,mid,surface;u8 mx,wall;i8 row,top,bottom;
 player_x+=dx;top=player_y>>8;bottom=(player_y+496)>>8;
 mx=(player_x+(dx<0?64:304))>>8;
 for(row=top;row<=bottom;++row)if(Wall(mx,row,dx<0?1:3)){
  player_x=dx<0?((u16)mx+1)*256-64:(u16)mx*256-305;break;
 }
 if(grounded){
  mid=player_x+184;mx=mid>>8;grounded=0;
  for(row=((player_y+496)>>8)-1;row<=(i8)(((player_y+496+144)>>8));++row){
   wall=Wall(mx,row,0);if(!wall)continue;
   surface=(u16)row*256+Height(wall,(mid>>4)&15)-1;
   if((i16)(surface-player_y-496)>=-144 && (i16)(surface-player_y-496)<=144){player_y=surface-496;grounded=1;floor_type=wall;break;}
  }
 }
 player_vx=0;
 if(climbing){climbing=0;pole_cooldown=19;grounded=0;player_vy=0;jump_hold=0;player_frame=facing_left?15:7;}
 if(player_x!=ox)combat_dirty=1;
 Publish();
}
void Game_Reset(void) __banked
{
    climbing=pole_cooldown=pole_anim=level_done=0;CartReset();KeyDoorReset();
    player_x=LEVEL_SPAWN_X*16;player_y=LEVEL_SPAWN_Y*16;
    player_vx=player_vy=0;grounded=0;facing_left=LEVEL_LEFT;
    player_frame=facing_left?8:0;floor_type=0;jump_hold=0;jump_previous=0;walk_time=0;
}
void Game_Tick(u8 input) __banked
{
    static i8 dir,y,top,bottom;
    static u8 jump,wall,headwall,mx,localx,shape;
    static i16 dx,dy,move,maxmove;
    static u16 oldy,oldbottom,newbottom,oldtop,newtop,mid,surface;
    static u8 topoffset;
    dir=0;jump=(input&16)!=0;headwall=0;
    ++simulation_ticks;
    if(input&32) { Publish();return; }
    if((input&3)==1) dir=-1;
    if((input&3)==2) dir=1;
    if(dir) facing_left=dir<0;
    CartStep();
    if(PoleTick(input,dir,jump)){KEYDOOR_TICK();Publish();return;}
    if(grounded)
    {
        player_vx=dir*24;
        if(jump && !jump_previous)
        {
            player_vx=dir*16;player_vy=-40;jump_hold=16;grounded=0;SoundPlay(SFX_JUMP);
        }
    }
    else if(simulation_ticks&1)
    {
        if(dir) player_vx+=dir*2;
        else if(player_vx>0) --player_vx;
        else if(player_vx<0) ++player_vx;
        if(player_vx>24) player_vx=24;
        if(player_vx< -24) player_vx=-24;
    }
    jump_previous=jump;
    dx=player_vx;
    if(grounded)
    {
        if(dir) dx+=slope_speed[floor_type&7];
        dy=(dx<0?-dx:dx)+16; // original pushtofloor follows down-slopes
    }
    else
    {
        if(jump_hold)
        {
            --jump_hold;
            if(!jump) jump_hold=0;
        }
        else if(simulation_ticks&1)
        {
            if(player_vy<0 && player_vy>=-3) player_vy=0;
            else player_vy+=3;
            if(player_vy>80) player_vy=80;
        }
        dy=player_vy;
    }
    if(grounded && dir) { if(++walk_time>=24)walk_time=0; shape=walk_frame[walk_time]; }
    else { walk_time=0;shape=grounded?0:(jump_hold?5:(player_vy<=0?6:7)); }
    player_frame=shape+(facing_left?8:0);
    topoffset=(shape==2 || shape==4)?48:0;
    oldy=player_y;oldbottom=oldy+496;oldtop=oldy+topoffset;
    player_x+=dx;
    if(player_x+64<512 || player_x+304>(MAP_WIDTH-2U)*256){
        u8 right=player_x+304>(MAP_WIDTH-2U)*256;
        if(ExitBoundary(right)){level_done=1;player_vx=player_vy=0;Publish();return;}
        player_x=right?((MAP_WIDTH-2U)*256-304):448;player_vx=0;
        // Keep vertical physics running when the side is a closed boundary.
    }
    player_y+=dy;
    newbottom=player_y+496;newtop=player_y+topoffset;
    mid=player_x+184;mx=mid>>8;localx=(mid>>4)&15;
    grounded=0;floor_type=0;
    maxmove=-(dx<0?-dx:dx)-dy-16;
    for(y=(oldbottom>>8)-1; y<=(i8)(newbottom>>8); ++y)
    {
        wall=Wall(mx,y,0);
        if(!wall)continue;
        surface=(u16)y*256+Height(wall,localx)-1;
        move=(i16)(surface-newbottom);
        if(move<0 && move>=maxmove)
        {
            player_y+=move;grounded=1;floor_type=wall;
            player_vy=0;jump_hold=0;
            if((wall&7)>1)++slope_hits;
            break;
        }
    }
    if(!grounded)
    {
        maxmove=(dx<0?-dx:dx)-dy+16;
        for(y=(oldtop>>8)+1;y>=(i8)(newtop>>8);--y)
        {
            wall=Wall(mx,y,2);
            if(!wall || wall==17)continue;
            surface=((u16)y+1)*256-Height(wall,localx);
            move=(i16)(surface-newtop);
            if(move>0 && move<=maxmove)
            {
                player_y+=move;headwall=wall;player_vy=0;jump_hold=0;++ceiling_hits;
                break;
            }
        }
    }
    top=(player_y+topoffset)>>8;bottom=(player_y+496)>>8;
    if(headwall>1)++top;
    if(floor_type>1)--bottom;
    if(dx<=0)
    {
        mx=(player_x+64)>>8;
        for(y=top;y<=bottom;++y)if(Wall(mx,y,1))
        {player_x=((u16)mx+1)*256-64;player_vx=0;++wall_hits;break;}
    }
    if(dx>=0)
    {
        mx=(player_x+304)>>8;
        for(y=top;y<=bottom;++y)if(Wall(mx,y,3))
        {player_x=(u16)mx*256-305;player_vx=0;++wall_hits;break;}
    }
    CartContact(oldbottom);
    KEYDOOR_TICK();
    if(player_y>=(MAP_HEIGHT-2U)*256) {++respawns;death_ticks=70;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}
    Publish();
}


#ifdef KEEN_STEP2
// 35 Hz step: advances Keen two simulation ticks with one physics pass (0.9.65).
// Exact for the plain case (not climbing, no cart, no pole start): the original applies gravity and
// horizontal acceleration on odd ticks only, so two ticks are +3 / +-2 once, then displacement 2*v.
// Anything special falls back to two ordinary ticks.
void Game_Tick2(u8 input) __banked
{
    i8 dir=0,y,top,bottom;
    u8 jump=(input&16)!=0,wall,headwall=0,mx,localx,shape,topoffset;
    i16 dx,dy,move,maxmove;u8 wasground,hit=0;
    u16 s_x,s_y,s_ticks,s_wall,s_ceil,s_slope;i16 s_vx,s_vy;u8 s_g,s_ft,s_jh,s_fl,s_pf,s_jp,s_wt;
    u16 oldy,oldbottom,newbottom,oldtop,newtop,mid,surface;
    if((input&32) || climbing || pole_cooldown || (input&12)==4 || (input&12)==8 || active_level==2 || active_level==7)
    {Game_Tick(input);Game_Tick(input);return;}
    // Snapshot: a wall/ceiling clamp inside a doubled step may embed Keen in a corner the two-tick
    // original would have hit one tick earlier, so such steps are replayed as two ordinary ticks.
    s_x=player_x;s_y=player_y;s_vx=player_vx;s_vy=player_vy;s_g=grounded;s_ft=floor_type;s_jh=jump_hold;
    s_fl=facing_left;s_pf=player_frame;s_jp=jump_previous;s_wt=walk_time;s_ticks=simulation_ticks;
    s_wall=wall_hits;s_ceil=ceiling_hits;s_slope=slope_hits;
    simulation_ticks+=2;
    if((input&3)==1) dir=-1;
    if((input&3)==2) dir=1;
    if(dir) facing_left=dir<0;
    CartStep();
    if(grounded)
    {
        player_vx=dir*24;
        if(jump && !jump_previous)
        {
            player_vx=dir*16;player_vy=-40;jump_hold=16;grounded=0;SoundPlay(SFX_JUMP);
        }
    }
    else
    {
        if(dir) player_vx+=dir*2;
        else if(player_vx>0) --player_vx;
        else if(player_vx<0) ++player_vx;
        if(player_vx>24) player_vx=24;
        if(player_vx< -24) player_vx=-24;
    }
    jump_previous=jump;wasground=grounded;
    dx=player_vx*2;
    if(grounded)
    {
        if(dir) dx+=slope_speed[floor_type&7]*2;
        dy=(dx<0?-dx:dx)+16;
    }
    else
    {
        if(jump_hold)
        {
            jump_hold=jump_hold>2?jump_hold-2:0;
            if(!jump) jump_hold=0;
        }
        else
        {
            if(player_vy<0 && player_vy>=-3) player_vy=0;
            else player_vy+=3;
            if(player_vy>80) player_vy=80;
        }
        dy=player_vy*2;
    }
    if(grounded && dir) { walk_time+=2;if(walk_time>=24)walk_time-=24; shape=walk_frame[walk_time]; }
    else { walk_time=0;shape=grounded?0:(jump_hold?5:(player_vy<=0?6:7)); }
    player_frame=shape+(facing_left?8:0);
    topoffset=(shape==2 || shape==4)?48:0;
    oldy=player_y;oldbottom=oldy+496;oldtop=oldy+topoffset;
    player_x+=dx;
    if(player_x+64<512 || player_x+304>(MAP_WIDTH-2U)*256){
        u8 right=player_x+304>(MAP_WIDTH-2U)*256;
        if(ExitBoundary(right)){level_done=1;player_vx=player_vy=0;Publish();return;}
        player_x=right?((MAP_WIDTH-2U)*256-304):448;player_vx=0;
    }
    player_y+=dy;
    newbottom=player_y+496;newtop=player_y+topoffset;
    mid=player_x+184;mx=mid>>8;localx=(mid>>4)&15;
    grounded=0;floor_type=0;
    maxmove=-(dx<0?-dx:dx)-dy-16;
    for(y=(oldbottom>>8)-1; y<=(i8)(newbottom>>8); ++y)
    {
        wall=Wall(mx,y,0);
        if(!wall)continue;
        surface=(u16)y*256+Height(wall,localx)-1;
        move=(i16)(surface-newbottom);
        if(move<0 && move>=maxmove)
        {
            player_y+=move;grounded=1;floor_type=wall;
            player_vy=0;jump_hold=0;
            if((wall&7)>1)++slope_hits;
            break;
        }
    }
    if(!grounded && wasground)
    {
        // Ledge: the original drops Keen by one tick's push-to-floor (|dx|/2+16), then he is airborne.
        dy=(dx<0?-dx:dx)/2+16;player_y=oldy+dy;newtop=player_y+topoffset;
    }
    if(!grounded)
    {
        maxmove=(dx<0?-dx:dx)-dy+16;
        for(y=(oldtop>>8)+1;y>=(i8)(newtop>>8);--y)
        {
            wall=Wall(mx,y,2);
            if(!wall || wall==17)continue;
            surface=((u16)y+1)*256-Height(wall,localx);
            move=(i16)(surface-newtop);
            if(move>0 && move<=maxmove)
            {
                player_y+=move;headwall=wall;player_vy=0;jump_hold=0;++ceiling_hits;hit=1;
                break;
            }
        }
    }
    top=(player_y+topoffset)>>8;bottom=(player_y+496)>>8;
    if(headwall>1)++top;
    if(floor_type>1)--bottom;
    if(dx<=0)
    {
        mx=(player_x+64)>>8;
        for(y=top;y<=bottom;++y)if(Wall(mx,y,1))
        {player_x=((u16)mx+1)*256-64;player_vx=0;++wall_hits;hit=1;break;}
    }
    if(dx>=0)
    {
        mx=(player_x+304)>>8;
        for(y=top;y<=bottom;++y)if(Wall(mx,y,3))
        {player_x=(u16)mx*256-305;player_vx=0;++wall_hits;hit=1;break;}
    }
    if(hit)
    {
        player_x=s_x;player_y=s_y;player_vx=s_vx;player_vy=s_vy;grounded=s_g;floor_type=s_ft;jump_hold=s_jh;
        facing_left=s_fl;player_frame=s_pf;jump_previous=s_jp;walk_time=s_wt;simulation_ticks=s_ticks;
        wall_hits=s_wall;ceiling_hits=s_ceil;slope_hits=s_slope;
        Game_Tick(input);Game_Tick(input);return;
    }
    CartContact(oldbottom);
    KEYDOOR_TICK();KEYDOOR_TICK();
    if(player_y>=(MAP_HEIGHT-2U)*256) {++respawns;death_ticks=70;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}
    Publish();
}
#endif
void RapidPush(i16 dx) __banked{GamePushSide(dx);}
