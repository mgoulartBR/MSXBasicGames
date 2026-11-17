#include "sprite_color_pass.h"
#include "castle.h"
#include "boobus.h"
#include "swamp.h"
#include "bay.h"
#include "grove.h"
#include "apple.h"
#include "city.h"
#include "pass.h"
#include "rapids.h"
// SPDX-License-Identifier: GPL-2.0-or-later
#include "game.h"
#include "combat.h"
#include "world.h"
#include "combat_assets.h"
#include "cart.h"
#include "keydoor.h"
#include "tomato.h"
#include "bridge.h"
#include "hill.h"
#include "mines.h"
#include "hw_tomato.h"
#include "campaign.h"
#include "opaque_sprites.h"
#include "fast_enemy_ids.h"
#include "timp_frames.h"
void FastEnemy(u8 page,DrawObject* ob) __banked;
extern volatile u8 hardware_scroll;
extern u8 bar_ok;
extern volatile u16 camera_px,camera_py;
DrawObject actors_old[2][ACTOR_COUNT];
DrawObject* actors_next;
u8 actors_partial;

void HudDamage(u8 page) __banked;
u8 HudDirty(u8 page) __banked;
extern volatile u8 front_page,drawn_frame;
extern volatile u16 drawn_x,drawn_y,drawn_camera_px,drawn_camera_py;
void HudDraw(u8 page) __banked;
void ForegroundHero(u8 page) __banked;
void HeroBitmap(u8 page,DrawObject* ob) __banked;
// Level load: turn each selected mask+colour pair into a single TIMP image, in place.
// mask' = (~mask & 9) overlaid by the colour image: 0 transparent, colour index, or 9 (black).
void TimpConvert(void) __banked
{
    const u8* list=timp_lists[LEVEL_INDEX];
    const SpriteDef* sp0=sprites[LEVEL_INDEX];const SpriteDef* sp;u8 f;u16 my;
    while((f=*list++)!=255){
        sp=sp0+f;my=SPR_MY(sp);
        g_VDP_Command.SX=sp->mx;g_VDP_Command.SY=my;g_VDP_Command.DX=sp->mx;g_VDP_Command.DY=my;
        g_VDP_Command.NX=sp->w;g_VDP_Command.NY=sp->h;g_VDP_Command.ARG=0;
        g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_NOT;VDP_CommandSetupR32();
        g_VDP_Command.CLR=9;g_VDP_Command.CMD=VDP_CMD_LMMV+VDP_OP_AND;VDP_CommandSetupR32();
        g_VDP_Command.SX=sp->cx;g_VDP_Command.SY=SPR_CY(sp);
        g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_TIMP;VDP_CommandSetupR32();
    }
    VDP_CommandWait();
}
void ActorDraw(u8 page,DrawObject* ob) __banked
{
    static i16 x,y;
    static u16 ax,ay,cx,cy,base;
    static u8 sx,sy,w,h,dx,dy,px,py,cw,ch,opaque;
    static const SpriteDef* sp;
    if(active_level==12 && ob->frame>=116 && ob->frame<124){CastleSquashBitmap(page,ob);return;}
    if(active_level==11 && ob->frame>=108 && ob->frame<117){BoobusBitmap(page,ob);return;}
    if(active_level==7 && ((ob->frame>=108 && ob->frame<120) || (ob->frame>=136 && ob->frame<154))){AppleBitmap(page,ob);return;}
    if(active_level==6 && ob->frame>=16 && ob->frame<26){CityCarrotBitmap(page,ob);return;}
    if((active_level==6 || active_level==12) && ob->frame>=152 && ob->frame<164){CityFrenchBitmap(page,ob);return;}
    if((active_level==2 || (active_level>=4 && active_level!=11)) && (ob->frame==128 || ob->frame==129 || ob->frame==134 || ob->frame==135)){MinesTaterBitmap(page,ob);return;}
    if(((active_level==2 || (active_level>=4 && active_level!=11)) && ob->frame>=108 && ob->frame<116) || ((active_level==2 || active_level==6) && ob->frame>=136 && ob->frame<140)){MinesBitmap(page,ob);return;}
    if((ob->frame<16 && (active_level!=3 || (ob->frame!=0 && ob->frame!=8))) || ob->frame==30 || ob->frame==31 || (ob->frame>=36 && ob->frame<46 && (active_level!=3 || (active_level>=2 && ob->frame!=36 && ob->frame!=43)))){HeroBitmap(page,ob);return;}
    if((ob->frame>=CART_FRAME && ob->frame<TOMATO_FRAME && !(active_level>=4 && ob->frame==89) && active_level!=6) || (active_level>=4 && ob->frame==105)){CartDraw(page,ob);return;}
    if((active_level==5 || active_level==8 || active_level==9 || active_level==10) && ob->frame>=136 && ob->frame<152){BroccoBitmap(page,ob);return;}
    sp=&sprites[LEVEL_INDEX][ob->frame];x=ob->x+SPR_OX(sp)-camera_px;y=ob->y+SPR_OY(sp)-camera_py;
    if(x<=-(i16)sp->w || x>=256 || y<=-(i16)sp->h || y>=(hardware_scroll?192:176)){ob->visible=0;return;}
    sx=sy=0;w=sp->w;h=sp->h;
    if(x<0){sx=-x;w-=sx;x=0;}if(y<0){sy=-y;h-=sy;y=0;}
    if(x+w>256)w=256-x;if(y+h>(hardware_scroll?192:176))h=(hardware_scroll?192:176)-y;
    dx=hardware_scroll?(x+camera_px)&255:x;dy=hardware_scroll?(y+camera_py)&255:y+16;
    opaque=opaque_sprites[LEVEL_INDEX][ob->frame];
    if(!opaque && active_level<=2 && fast_ids[LEVEL_INDEX][ob->frame]!=255){FastEnemy(page,ob);return;}
    ax=sp->mx+sx;ay=SPR_MY(sp)+sy;cx=sp->cx+sx;cy=SPR_CY(sp)+sy;base=(u16)page*256;
    for(py=0;py<h;py+=ch)
    {
        ch=h-py;if((u16)dy+ch>256)ch=256-dy;
        for(px=0;px<w;px+=cw)
        {
            cw=w-px;if((u16)dx+cw>256)cw=256-dx;
            // Reuse destination and size for the color pass. Avoid rebuilding
            // the same command through the multi-argument inline wrappers.
            g_VDP_Command.DX=dx;g_VDP_Command.DY=base+dy;
            g_VDP_Command.NX=cw;g_VDP_Command.NY=ch;g_VDP_Command.ARG=0;
            if(opaque){
                // A zero mask over the entire crop needs only the color copy.
                // HMMM needs byte-aligned X/width; IMP preserves odd edge pixels.
                g_VDP_Command.SX=cx+px;g_VDP_Command.SY=cy+py;
                g_VDP_Command.CMD=((cx+px|dx|cw)&1)?VDP_CMD_LMMM+VDP_OP_IMP:VDP_CMD_HMMM;
                VDP_CommandSetupR32();
            }else if(timp_bits[LEVEL_INDEX][ob->frame>>3]&(1<<(ob->frame&7))){
                // Mask area was rewritten at load into one image: 0 = transparent, 9 = black.
                g_VDP_Command.SX=ax+px;g_VDP_Command.SY=ay+py;
                g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_TIMP;
                VDP_CommandSetupR32();
            }else{
                g_VDP_Command.SX=ax+px;g_VDP_Command.SY=ay+py;
                g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_AND;
                VDP_CommandSetupR32();
                g_VDP_Command.SX=cx+px;g_VDP_Command.SY=cy+py;
                g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_OR;
                SpriteColorPass();
            }
            dx=0;
        }
        dx=hardware_scroll?(x+camera_px)&255:x;dy=0;
    }
}
    static u8 collect_i;
    static u8 collect_slot;
    static DrawObject* collect_ob;
    static const Pickup* collect_pk;
    static CombatObject* collect_shot;

#include "scanwin.inc"
#define COLLECT_NAME ActorsCollect
#define ACTORS_IMMEDIATE 0
#include "actor_collect.inc"
#undef COLLECT_NAME
#undef ACTORS_IMMEDIATE
#define COLLECT_NAME ActorsImmediate
#define ACTORS_IMMEDIATE 1
#include "actor_collect.inc"
#undef COLLECT_NAME
#undef ACTORS_IMMEDIATE


void ActorsDraw(u8 page) __banked
{
    static const u8 order[ACTOR_COUNT]={4,5,6,7,8,9,10,11,12,13,1,28,20,21,22,23,24,25,26,27,15,16,17,18,19,0,2,3,14};
    u8 i,slot;DrawObject* ob;DrawObject* next;
    if(!actors_next){actors_next=actors_old[page];ActorsImmediate(page);return;}
    for(i=0;i<ACTOR_COUNT;++i){
        slot=order[i];next=actors_next+slot;
        if(!(next->visible&2))continue;
        ob=actors_old[page]+slot;
        if(!(next->visible&1)){ob->visible=0;continue;}
        *ob=*next;ob->visible=1|(next->visible&60);
        if(slot==14)HudDraw(page);
        else {ActorDraw(page,ob);if(slot==0)ForegroundHero(page);}
    }
    
}
