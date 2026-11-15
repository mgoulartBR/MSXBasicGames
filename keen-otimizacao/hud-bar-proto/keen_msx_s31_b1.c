// SPDX-License-Identifier: GPL-2.0-or-later
#include "msxgl.h"
#include "game.h"
#include "combat.h"
#include "combat_assets.h"
#include "hw_tomato.h"
extern u8 page_cy[2];
extern volatile u8 flip_flag,flip_r2;
extern u16 page_px[2];
extern volatile u8 hardware_scroll,render_commit,front_page,camera_x,camera_y;
extern volatile u8 drawn_frame,drawn_cx,drawn_cy;
extern volatile u16 camera_px,camera_py,full_redraws,scroll_copies,render_count,drawn_x,drawn_y,drawn_camera_px,drawn_camera_py;
u8 RingStable(u8 page) __banked;
void RingScene(u8 page) __banked;
// Paint a clipped rectangle from the ROM tile map. Horizontal coordinates
// are even, as required by SCREEN 5 HMMM. Partial edge tiles are supported.
static void Background(u8 page,u16 cx,u8 cy,i16 left,i16 top,i16 right,i16 bottom)
{
    static u16 x,y,wx,tile,base;
    static u8 width,height,tx,ty;
    static u16 map;
    base=(u16)page*256;
    if(left<0)left=0;if(top<0)top=0;
    if(right>256)right=256;if(bottom>176)bottom=176;
    if(left>=right || top>=bottom)return;
    left&=~1;right=(right+1)&~1;
    for(y=top;y<bottom;y+=height)
    {
        ty=y&15;height=16-ty;if(y+height>bottom)height=bottom-y;
        map=(u16)(cy+(y>>4))*MAP_WIDTH;
        for(x=left;x<right;x+=width)
        {
            wx=cx+x;tx=wx&15;width=16-tx;
            if(x+width>right)width=right-x;
            tile=MapTile(map+(wx>>4));
            // Assign registers directly: avoid SDCC's IY spill issue with
            // this multi-argument inline call (corrupts destination Y).
            g_VDP_Command.SX=tile_x[LEVEL_INDEX][tile]+tx;
            g_VDP_Command.SY=tile_y[LEVEL_INDEX][tile]+ty;
            g_VDP_Command.DX=x;
            g_VDP_Command.DY=base+16+y;
            g_VDP_Command.NX=width;g_VDP_Command.NY=height;
            g_VDP_Command.ARG=0;g_VDP_Command.CMD=VDP_CMD_HMMM;
            VDP_CommandSetupR32();
        }
    }
}
// Fast exposed-column path: no per-tile rectangle clipping or row division.
static void Strip(u8 page,u16 left,u16 right)
{
    static u16 wx,tile,x,sy;
    static u8 width,row,offset;
    static u16 map;
    for(x=left;x<right;x+=width)
    {
        wx=camera_px+x;offset=wx&15;width=16-offset;
        if(x+width>right)width=right-x;
        map=(u16)camera_y*MAP_WIDTH+(wx>>4);
        sy=(u16)page*256+16;
        for(row=0;row<11;++row)
        {
            tile=MapTile(map);map+=MAP_WIDTH;
            g_VDP_Command.SX=tile_x[LEVEL_INDEX][tile]+offset;
            g_VDP_Command.SY=tile_y[LEVEL_INDEX][tile];
            g_VDP_Command.DX=hardware_scroll?(wx&255):x;
            g_VDP_Command.DY=sy;
            g_VDP_Command.NX=width;g_VDP_Command.NY=16;
            g_VDP_Command.ARG=0;g_VDP_Command.CMD=VDP_CMD_HMMM;
            VDP_CommandSetupR32();sy+=16;
        }
    }
}
// Bounds of changed half tiles, prepared from offline visual-difference masks.
extern u8 changed_l[11],changed_r[11];
void ChangedBounds(i16 shift) __banked;
void BarOn(void) __banked;
extern u8 bar_on;
void DrawScene(void) __banked
{
    u8 hw_buffer[132];
    DrawObject next[ACTOR_COUNT];
    static u8 page,arg;
    static u16 base,sx,sy,dx,dy,width,height;
    static i16 shiftx,shifty;
    
    page=front_page^1;arg=0;base=(u16)page*256;
    shiftx=(i16)(camera_px-page_px[page]);
    shifty=((i16)camera_y-page_cy[page])*16;
    while(flip_flag)Halt();   // previous frame's vblank flip must have been applied
    ActorsPrepare(page,next,hardware_scroll?RingStable(page):(page_px[page]==camera_px && page_cy[page]==camera_y));
    if(actors_partial==2){combat_dirty=0;return;}
    if(actors_partial==3){
        // The displayed bitmap still matches every software actor and the HUD.
        // Present only Keen's hardware attributes, without touching either page.
        hw_attributes=hw_buffer;HWUpload(front_page);
        VDP_CommandWait();HWFlip(front_page);flip_flag=1;++render_count;
        drawn_x=player_x;drawn_y=player_y;drawn_frame=hero_frame;
        combat_dirty=0;return;
    }
    if(hardware_scroll)RingScene(page);
    else if(page_px[page]!=65535 && shiftx>-128 && shiftx<128 && shifty>-96 && shifty<96)
    {
        u8 i;DrawObject* ob;
        for(i=0;i<ACTOR_COUNT;++i){ob=&actors_old[page][i];if((ob->visible&1) && (!actors_partial || (ob->visible&2))){
            if(i==1){VDP_CommandWait();enemy_probe[2]=0;}
            Background(page,page_px[page],page_cy[page],ob->x+SPR_OX(&sprites[LEVEL_INDEX][ob->frame])-page_px[page],ob->y+SPR_OY(&sprites[LEVEL_INDEX][ob->frame])-(u16)page_cy[page]*16,
                ob->x+SPR_OX(&sprites[LEVEL_INDEX][ob->frame])-page_px[page]+sprites[LEVEL_INDEX][ob->frame].w,ob->y+SPR_OY(&sprites[LEVEL_INDEX][ob->frame])-(u16)page_cy[page]*16+sprites[LEVEL_INDEX][ob->frame].h);
            if(i==1){VDP_CommandWait();enemy_probe[3]=0;}
        }}

        VDP_CommandWait();
        if(shiftx || shifty)
        {
            if(!shifty && shiftx>=-48 && shiftx<=48)
            {
                static u8 row;
                static i16 l,r,vl,vr;
                ChangedBounds(shiftx);
                for(row=0;row<11;++row)
                {
                    if(changed_l[row]==32)continue;
                    l=(u16)changed_l[row]*8;r=(u16)changed_r[row]*8;
                    vl=l;vr=r;
                    if(vl+shiftx<0)vl=-shiftx;
                    if(vr+shiftx>256)vr=256-shiftx;
                    if(vl<vr)
                    {
                        arg=0;sx=vl+shiftx;dx=vl;width=vr-vl;
                        if(dx>sx){sx+=width-2;dx+=width-2;arg=VDP_ARG_DIX_LEFT;}
                        VDP_CommandHMMM_Arg(sx,base+16+(u16)row*16,dx,base+16+(u16)row*16,width,16,arg);
                    }
                    if(l<vl)Background(page,camera_px,camera_y,l,(u16)row*16,vl,(u16)(row+1)*16);
                    if(vr<r)Background(page,camera_px,camera_y,vr,(u16)row*16,r,(u16)(row+1)*16);
                }
            }
            else
            {
            sx=shiftx>0?shiftx:0;dx=shiftx<0?-shiftx:0;
            sy=shifty>0?shifty:0;dy=shifty<0?-shifty:0;
            width=256-(shiftx<0?-shiftx:shiftx);
            height=176-(shifty<0?-shifty:shifty);
            // In-place memmove: choose reverse directions when moving right/down.
            if(dx>sx) {sx+=width-2;dx+=width-2;arg|=VDP_ARG_DIX_LEFT;}
            if(dy>sy) {sy+=height-1;dy+=height-1;arg|=VDP_ARG_DIY_UP;}
            VDP_CommandHMMM_Arg(sx,base+16+sy,dx,base+16+dy,width,height,arg);
            if(shiftx>0)Strip(page,256-shiftx,256);
            if(shiftx<0)Strip(page,0,-shiftx);
            if(shifty>0)Background(page,camera_px,camera_y,0,176-shifty,256,176);
            if(shifty<0)Background(page,camera_px,camera_y,0,0,256,-shifty);
            }
            ++scroll_copies;
        }
    }
    else {Background(page,camera_px,camera_y,0,0,256,176);++full_redraws;}
    if(!hardware_scroll)ActorsDraw(page);
    hw->scene_map_dirty&=~(1<<page);
    page_px[page]=camera_px;page_cy[page]=camera_y;
    hw_attributes=hw_buffer;HWUpload(page);
    VDP_CommandWait();
    // Everything that must change together at vblank is stored; SoundISR (H.TIMI) applies it.
    drawn_camera_px=camera_px;drawn_camera_py=camera_py;
    flip_r2=(page<<5)|0x1F;
    HWFlip(page);
    if((active_level==10 || active_level==1) && !bar_on)BarOn();
    flip_flag=1|2|(hardware_scroll?4:0);
    ++render_count;
    drawn_x=player_x;drawn_y=player_y;drawn_frame=hero_frame;
    drawn_cx=camera_x;drawn_cy=camera_y;front_page=page;combat_dirty=0;render_commit=0;
}
void FollowCamera(void) __banked
{
    static i16 x,y,target;
    static u8 step;
    step=hardware_scroll?2:8;x=player_x>>4;y=player_y>>4;
    // Same thresholds, bounds and rounding as repeated one-step scrolling,
    // but constant work even after a large vertical jump between renders.
    if(x-(i16)camera_px<96){
        target=(x-96)&-(i16)step;if(target<32)target=32;camera_px=target;
    }else if(x-(i16)camera_px>144){
        target=(x-144+step-1)&-(i16)step;
        if(target>(MAP_WIDTH-18)*16)target=(MAP_WIDTH-18)*16;camera_px=target;
    }
    camera_x=camera_px>>4;
    if(hardware_scroll){
        if(y-(i16)camera_py<48){target=y-48;if(target<32)target=32;camera_py=target;}
        else if(y-(i16)camera_py>112){target=y-112;if(target>(MAP_HEIGHT-14)*16)target=(MAP_HEIGHT-14)*16;camera_py=target;}
        camera_y=camera_py>>4;
    }else{
        if(y-(i16)camera_y*16<48){target=(y-48)>>4;if(target<2)target=2;camera_y=target;}
        else if(y-(i16)camera_y*16>112){target=(y-112+15)>>4;if(target>MAP_HEIGHT-13)target=MAP_HEIGHT-13;camera_y=target;}
        camera_py=(u16)camera_y*16;
    }
}

u8 ReadInput(void) __banked
{
    u8 joy=Joystick_Read(JOY_PORT_1);
    u8 in=0,mode=enemy_mode;
    if(Keyboard_IsKeyPressed(KEY_1))mode=0;
    if(Keyboard_IsKeyPressed(KEY_2))mode=1;
    if(Keyboard_IsKeyPressed(KEY_3))mode=2;
    if(Keyboard_IsKeyPressed(KEY_4))mode=3;
    if(mode!=enemy_mode){enemy_mode=mode;combat_dirty=1;}

    if(Keyboard_IsKeyPressed(KEY_LEFT) || !(joy&JOY_INPUT_DIR_LEFT)) in|=1;
    if(Keyboard_IsKeyPressed(KEY_RIGHT) || !(joy&JOY_INPUT_DIR_RIGHT)) in|=2;
    if(Keyboard_IsKeyPressed(KEY_UP) || !(joy&JOY_INPUT_DIR_UP)) in|=4;
    if(Keyboard_IsKeyPressed(KEY_DOWN) || !(joy&JOY_INPUT_DIR_DOWN)) in|=8;
    if(Keyboard_IsKeyPressed(KEY_SPACE) || !(joy&JOY_INPUT_TRIGGER_A)) in|=16;
    if(Keyboard_IsKeyPressed(KEY_R)) in|=32;
    if(Keyboard_IsKeyPressed(KEY_C) || !(joy&JOY_INPUT_TRIGGER_B))in|=64;
    return in;
}


// Frenchy textures moved to bank 103; bank 31 also contains Apple map data.
