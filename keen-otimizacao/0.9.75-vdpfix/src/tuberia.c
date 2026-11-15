#include "campaign.h"
// SPDX-License-Identifier: GPL-2.0-or-later
#include "game.h"
#include "world.h"
#include "pickups.h"
#include "tuberia_data.h"
#include "tuberia_header.h"
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);
extern const u8 g_Font_MGL_Sample6[];
extern volatile u8 hardware_scroll;
u16 world_x,world_y;
u8 world_active;
u16 world_completed;
static u8 wdir;
u8 world_notice;
u16 wcx,wcy;
u8 wpage,wframe,world_draw_busy;
static u16 old_wx[2],old_wy[2];
static u16 old_cx[2],old_cy[2];
u16 WorldExtra(u16 pattern) __banked;
static void WorldSource(u16 pattern){if(pattern>=512){g_VDP_Command.SX=0;g_VDP_Command.SY=WorldExtra(pattern);}else{g_VDP_Command.SX=(pattern&15)*16;g_VDP_Command.SY=512+(pattern>>4)*16;}}
static u16 WorldTile(u16 i){return MapTile(i);}
static void SetWorldTile(u16 i,u16 tile){MapSet(i,tile);}
static void PatchWorld(void){u8 i;for(i=0;i<sizeof(tuber_patch)/sizeof(tuber_patch[0]);++i)SetWorldTile(tuber_patch[i][0],tuber_patch[i][(world_completed&tuber_patch[i][3])?2:1]);}
static u8 WorldInput(void)
{
    u8 j=Joystick_Read(JOY_PORT_1),in=0;
    if(Keyboard_IsKeyPressed(KEY_LEFT)||!(j&JOY_INPUT_DIR_LEFT))in|=1;
    if(Keyboard_IsKeyPressed(KEY_RIGHT)||!(j&JOY_INPUT_DIR_RIGHT))in|=2;
    if(Keyboard_IsKeyPressed(KEY_UP)||!(j&JOY_INPUT_DIR_UP))in|=4;
    if(Keyboard_IsKeyPressed(KEY_DOWN)||!(j&JOY_INPUT_DIR_DOWN))in|=8;
    if(Keyboard_IsKeyPressed(KEY_SPACE)||Keyboard_IsKeyPressed(KEY_C)||!(j&JOY_INPUT_TRIGGER_A)||!(j&JOY_INPUT_TRIGGER_B))in|=16;
    if(Keyboard_IsKeyPressed(KEY_N))in|=32;
    return in;
}
static u8 Node(u16 pos)
{
    u8 lo=0,hi=sizeof(tuber_nodes)/sizeof(tuber_nodes[0]),mid;
    while(lo<hi){mid=lo+(hi-lo)/2;if(tuber_nodes[mid].pos<pos)lo=mid+1;else hi=mid;}
    return lo<sizeof(tuber_nodes)/sizeof(tuber_nodes[0])&&tuber_nodes[lo].pos==pos?tuber_nodes[lo].level:0;
}
static u8 Selected(void)
{
    u8 x,y,n;
    for(y=(world_y+2)>>4;y<=(world_y+14)>>4;++y)
        for(x=(world_x+2)>>4;x<=(world_x+13)>>4;++x){n=Node((u16)y*85+x);if(n)return n;}
    return 0;
}
static u8 Blocked(u16 x,u16 y,u8 side)
{
    if(x<32 || y<32 || x>=83*16 || y>=64*16)return 1;
    return tuber_walls[WorldTile((y>>4)*85+(x>>4))]&(1<<side);
}
static void MoveWorld(i8 dx,i8 dy)
{
    u16 x=world_x+dx,y=world_y+dy;
    if(x<32 || x>83*16-16)x=world_x;
    if(y<32 || y>64*16-16)y=world_y;
    if(dx && !Blocked(x+(dx>0?13:2),world_y+3,dx>0?3:1) && !Blocked(x+(dx>0?13:2),world_y+14,dx>0?3:1))world_x=x;
    if(dy && !Blocked(world_x+3,y+(dy>0?14:2),dy>0?0:2) && !Blocked(world_x+12,y+(dy>0?14:2),dy>0?0:2))world_y=y;
}
static void WorldCamera(void)
{
    i16 x=world_x-128,y=world_y-80;
    if(x<32)x=32;if(x>1072)x=1072;if(y<32)y=32;if(y>848)y=848;
    wcx=x&~1;wcy=y;
}
// Pixel rectangles, clipped to the viewport. HMMM moves whole SCREEN 5 bytes.
static void WorldBG(u8 page,i16 left,i16 top,i16 right,i16 bottom)
{
    u16 x,y,wx,wy,tile;u8 tx,ty,w,h;
    if(left<0)left=0;if(top<0)top=0;if(right>256)right=256;if(bottom>176)bottom=176;
    if(left>=right||top>=bottom)return;
    left&=~1;right=(right+1)&~1;
    for(y=top;y<bottom;y+=h){
        wy=wcy+y;ty=wy&15;h=16-ty;if(y+h>bottom)h=bottom-y;
        for(x=left;x<right;x+=w){
            wx=wcx+x;tx=wx&15;w=16-tx;if(x+w>right)w=right-x;
            tile=tuber_tiles[WorldTile((wy>>4)*85+(wx>>4))];
            g_VDP_Command.SX=(tile&15)*16+tx;g_VDP_Command.SY=512+(tile>>4)*16+ty;
            g_VDP_Command.DX=hardware_scroll?(wx&255):x;
            g_VDP_Command.DY=hardware_scroll?((u16)page*256+(wy&255)):(u16)page*256+16+y;
            g_VDP_Command.NX=w;g_VDP_Command.NY=h;g_VDP_Command.ARG=0;
            g_VDP_Command.CMD=VDP_CMD_HMMM;VDP_CommandSetupR32();
        }
    }
}
// Two circular pages: draw only into the hidden page, then commit at VBlank.
// Split all sprite/header transfers at circular edges, including diagonal wraps.
static void WorldWrapCopy(u8 page,u16 sx,u16 sy,u16 dx,u16 dy,u16 width,u8 height,u8 cmd)
{
    u16 x,y,w,h;
    for(y=0;y<height;y+=h){
        h=256-((dy+y)&255);if(h>height-y)h=height-y;
        for(x=0;x<width;x+=w){
            w=256-((dx+x)&255);if(w>width-x)w=width-x;
            g_VDP_Command.SX=sx+x;g_VDP_Command.SY=sy+y;
            g_VDP_Command.DX=(dx+x)&255;g_VDP_Command.DY=(u16)page*256+((dy+y)&255);
            g_VDP_Command.NX=w;g_VDP_Command.NY=h;g_VDP_Command.ARG=0;g_VDP_Command.CMD=cmd;VDP_CommandSetupR32();
        }
    }
}
static void WorldHeader(u8 page)
{
    u8 y,x=(wcx&255)>>1,n=128-x;
    u16 address;const u8* source=tuber_header[world_notice];
    VDP_CommandWait();
    for(y=0;y<16;++y){
        address=(u16)page*32768+(u16)((wcy-16+y)&255)*128;
        VPF(source,address+x,0,n);
        if(x)VPF(source+n,address,0,x);
        source+=128;
    }
}
static void WorldHardware(u8 page)
{
    i16 dx=(i16)wcx-old_cx[page],dy=(i16)wcy-old_cy[page];u16 pattern;
    if(old_cx[page]==65535 || dx<=-256 || dx>=256 || dy<=-176 || dy>=176)
        WorldBG(page,0,0,256,176);
    else{
        WorldBG(page,(i16)old_wx[page]-wcx,(i16)old_wy[page]-wcy,(i16)old_wx[page]-wcx+16,(i16)old_wy[page]-wcy+16);
        if(dx>0)WorldBG(page,256-dx,0,256,176);
        else if(dx<0)WorldBG(page,0,0,-dx,176);
        if(dy>0)WorldBG(page,0,176-dy,256,176);
        else if(dy<0)WorldBG(page,0,0,256,-dy);
    }
    pattern=tuber_sprites[wframe][0];
    WorldSource(pattern);WorldWrapCopy(page,g_VDP_Command.SX,g_VDP_Command.SY,world_x,world_y,16,16,VDP_CMD_LMMM+VDP_OP_AND);
    pattern=tuber_sprites[wframe][1];
    WorldSource(pattern);WorldWrapCopy(page,g_VDP_Command.SX,g_VDP_Command.SY,world_x,world_y,16,16,VDP_CMD_LMMM+VDP_OP_OR);
    if(old_cx[page]!=wcx || old_cy[page]!=wcy)
        WorldHeader(page);
    old_wx[page]=world_x;old_wy[page]=world_y;old_cx[page]=wcx;old_cy[page]=wcy;
    VDP_CommandWait();Halt();VDP_SetPage(page);
    VDP_SetVerticalOffset((wcy-16)&255);
    VDP_RegWrite(25,2);VDP_RegWrite(26,((wcx+7)>>3)&31);VDP_RegWrite(27,(8-wcx)&7);
    wpage=page;
}
static void WorldDraw(void)
{
    u8 page=wpage^1,source=wpage;u16 pattern;
    i16 dx,dy;u16 sx,sy,tx,ty,w,h;
    world_draw_busy=1;WorldCamera();
    if(hardware_scroll){WorldHardware(page);world_draw_busy=0;return;}
    dx=(i16)wcx-old_cx[source];dy=(i16)wcy-old_cy[source];
    if(old_cx[source]==65535 || dx<=-256 || dx>=256 || dy<=-176 || dy>=176)
        WorldBG(page,0,0,256,176);
    else if(old_cx[page]==wcx && old_cy[page]==wcy)
        WorldBG(page,(i16)old_wx[page]-wcx,(i16)old_wy[page]-wcy,(i16)old_wx[page]-wcx+16,(i16)old_wy[page]-wcy+16);
    else{
        // Copy the overlap from the displayed page without modifying it.
        sx=dx>0?dx:0;tx=dx<0?-dx:0;sy=dy>0?dy:0;ty=dy<0?-dy:0;
        w=256-sx-tx;h=176-sy-ty;
        VDP_CommandHMMM(sx,(u16)source*256+16+sy,tx,(u16)page*256+16+ty,w,h);
        WorldBG(page,(i16)old_wx[source]-wcx,(i16)old_wy[source]-wcy,(i16)old_wx[source]-wcx+16,(i16)old_wy[source]-wcy+16);
        if(dx>0)WorldBG(page,256-dx,0,256,176);
        else if(dx<0)WorldBG(page,0,0,-dx,176);
        if(dy>0)WorldBG(page,tx,176-dy,tx+w,176);
        else if(dy<0)WorldBG(page,tx,0,tx+w,-dy);
    }
    pattern=tuber_sprites[wframe][0];
    WorldSource(pattern);
    g_VDP_Command.DX=world_x-wcx;g_VDP_Command.DY=(u16)page*256+16+world_y-wcy;
    g_VDP_Command.NX=16;g_VDP_Command.NY=16;g_VDP_Command.ARG=0;g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_AND;VDP_CommandSetupR32();
    pattern=tuber_sprites[wframe][1];WorldSource(pattern);g_VDP_Command.CMD=VDP_CMD_LMMM+VDP_OP_OR;VDP_CommandSetupR32();
    old_wx[page]=world_x;old_wy[page]=world_y;old_cx[page]=wcx;old_cy[page]=wcy;
    VDP_CommandWait();Halt();VDP_SetPage(page);wpage=page;world_draw_busy=0;
}
static void WorldText(u8 notice)
{
    world_notice=notice?2:world_completed?1:0;
    if(hardware_scroll){old_cx[0]=old_cx[1]=65535;return;}
    VDP_CommandHMMV(0,0,256,16,0);VDP_CommandWait();
    VDP_CommandWait();VDP_CommandHMMM(0,0,0,256,256,16);VDP_CommandWait();
    if(hardware_scroll)old_cx[0]=65535;
}
void WorldMap(u8 completed) __banked
{
    u8 in,previous,last,now,elapsed,hz,anim=0,oldframe,world_selection;
    u16 credit=0,oldx,oldy;
    i8 dx,dy;
    world_active=0;
    if(!completed){world_x=48*16;world_y=39*16;world_completed=0;flower_ammo=0;}
    if(completed)world_completed|=1<<(completed-1);
    PatchWorld();
    VDP_CommandWait();VDP_SetVerticalOffset(0);
    if(hardware_scroll){VDP_RegWrite(25,0);VDP_RegWrite(26,0);VDP_RegWrite(27,0);}
    VDP_SetLineCount(VDP_LINE_192);VDP_SetPage(0);VDP_CommandHMMV(0,0,256,512,0);VDP_CommandWait();
    Print_SetBitmapFont(g_Font_MGL_Sample6);Print_SetColor(15,0);
    old_cx[0]=old_cx[1]=65535;old_cy[0]=old_cy[1]=65535;wpage=0;wdir=0;wframe=2;
    WorldText(0);WorldDraw();WorldDraw();world_selection=Selected();world_active=1;
    hz=VDP_GetFrequency()?50:60;last=*((volatile u8*)M_JIFFY);previous=WorldInput();
    for(;;){
        now=*((volatile u8*)M_JIFFY);elapsed=now-last;last=now;credit+=(u16)elapsed*70;
        in=WorldInput();
        if((in&32)&&!(previous&32)){
            CampaignNew();world_completed=0;flower_ammo=0;world_x=48*16;world_y=39*16;PatchWorld();
            old_cx[0]=old_cx[1]=65535;WorldText(0);WorldDraw();WorldDraw();
        }
        if((in&16)&&!(previous&16)){
            world_selection=Selected();
            if(world_selection>=1 && world_selection<=12 && !(world_completed&(1<<(world_selection-1)))){if(world_selection==11 && boobus_bombs<12){WorldText(0);Print_SetPosition(12,8);Print_DrawText("BOOBUS NEEDS AT LEAST 12 BOMBS");VDP_CommandWait();WorldDraw();}else{active_level=world_selection;SoundPlay(SFX_ENTER);break;}}
            if(world_selection && world_selection>12){WorldText(1);WorldDraw();}
        }
        previous=in;oldx=world_x;oldy=world_y;oldframe=wframe;
        dx=(in&3)==1?-1:(in&3)==2?1:0;dy=(in&12)==4?-1:(in&12)==8?1:0;
        if(dx||dy){wdir=dy<0?(dx<0?18:dx>0?21:6):dy>0?(dx<0?15:dx>0?12:9):dx<0?0:3;}
        while(credit>=hz){credit-=hz;if(dx||dy){MoveWorld(dx,dy);++anim;}}
        wframe=wdir+(dx||dy?((anim>>2)&3)==0?1:((anim>>2)&3)==2?0:2:2);
        if(oldx!=world_x||oldy!=world_y||oldframe!=wframe)WorldDraw();
        else Halt();
    }
    world_active=0;
    while(WorldInput()&16)Halt();
    VDP_SetVerticalOffset(0);
    if(hardware_scroll){VDP_RegWrite(26,0);VDP_RegWrite(27,0);}
    VDP_CommandWait();VDP_SetPage(0);VDP_CommandHMMV(0,0,256,512,0);VDP_CommandWait();
    if(hardware_scroll){VDP_RegWrite(25,2);VDP_SetLineCount(VDP_LINE_192);}
    else{
        VDP_SetLineCount(VDP_LINE_212);
        VDP_CommandWait();VDP_CommandHMMM(0,0,0,256,256,212);VDP_CommandWait();
    }
}
