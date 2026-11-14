// SPDX-License-Identifier: GPL-2.0-or-later
// V9958: two independent 256x256 toroidal buffers, 256x192 viewport.
#include "msxgl.h"
#include "game.h"
#include "world.h"
#include "combat_assets.h"
#include "combat.h"
#include "solid_tiles.h"
#include "hw_tomato.h"
#include "restore_bands.h"
#include "slotscan.inc"
extern volatile u16 camera_px,camera_py,full_redraws,scroll_copies;

static u8 valid[2];
// Four clean backgrounds per page. Slots 2/3 use x224..255 only
// when no wide streamed Tater can overwrite that part of the hidden page.
extern DrawObject* actors_next;
void RingCopy(u16 tile,u8 tx,u8 ty,u8 dx,u16 dy,u16 w,u8 h) __sdcccall(0);
void RingReset(void) __banked {valid[0]=valid[1]=0;}
u16 ring_camera_x[2],ring_camera_y[2];
#define camx ring_camera_x
#define camy ring_camera_y
// 0: rebuild; 1: same viewport; 2: reusable world-aligned overlap.
u8 RingStable(u8 page) __banked
{
 i16 dx=camera_px-camx[page],dy=camera_py-camy[page];
 if(!valid[page] || dx<=-128 || dx>=128 || dy<=-96 || dy>=96)return 0;
 return (dx || dy)?2:1;
}

// World-aligned tiles never cross the 256-pixel VRAM wrap boundary.
// Repeat an already drawn complete tile strip by doubling valid VRAM pixels.
// Callers stop each run at the circular page boundary.
static void RepeatStrip(u16 x,u16 y,u16 width,u16 height,u8 horizontal)
{
 u16 filled=16,total=horizontal?width:height,part;
 while(filled<total){
  part=total-filled;if(part>filled)part=filled;
  g_VDP_Command.SX=x;g_VDP_Command.SY=y;
  g_VDP_Command.DX=x+(horizontal?filled:0);
  g_VDP_Command.DY=y+(horizontal?0:filled);
  g_VDP_Command.NX=horizontal?part:width;
  g_VDP_Command.NY=horizontal?height:part;
  g_VDP_Command.CMD=VDP_CMD_HMMM;VDP_CommandSetupR32();filled+=part;
 }
}
static void RectX(u8 page,u16 cx,u16 cy,i16 l,i16 t,i16 r,i16 b,u8 exact)
{
    static u16 x,y,wx,wy,tile,base;
    static u8 w,h,tx,ty;
    static u16 map;
    if(l<0)l=0;if(t<0)t=0;if(r>256)r=256;if(b>192)b=192;
    if(l>=r || t>=b)return;
    if(!exact){l&=~1;r=(r+1)&~1;}
    base=(u16)page*256;
    for(y=t;y<b;y+=h)
    {
        wy=cy+y;ty=wy&15;h=16-ty;if(y+h>b)h=b-y;
        wx=cx+l;tx=wx&15;
        map=(wy>>4)*MAP_WIDTH+(wx>>4);
#if !FAST_RING_COPY
        g_VDP_Command.DY=base+(wy&255);g_VDP_Command.NY=h;
        g_VDP_Command.CMD=VDP_CMD_HMMM;
#endif
        g_VDP_Command.ARG=0;
        for(x=l;x<r;x+=w)
        {
            w=16-tx;if(x+w>r)w=r-x;
            tile=MapTile(map++);
#if !FAST_RING_COPY
            g_VDP_Command.SX=tile_x[LEVEL_INDEX][tile]+tx;
            g_VDP_Command.SY=tile_y[LEVEL_INDEX][tile]+ty;
            g_VDP_Command.DX=wx&255;g_VDP_Command.NX=w;
            if(exact)g_VDP_Command.CMD=((g_VDP_Command.SX|g_VDP_Command.DX|w)&1)?VDP_CMD_LMMM+VDP_OP_IMP:VDP_CMD_HMMM;
#endif
#if FAST_RING_COPY
            RingCopy(tile,tx,ty,wx&255,base+(wy&255),w,h);
#else
            VDP_CommandSetupR32();
#endif
            wx+=w;tx=0;
        }
    }
}
static void Rect(u8 page,u16 cx,u16 cy,i16 l,i16 t,i16 r,i16 b){RectX(page,cx,cy,l,t,r,b,0);}
// HUD panel follows the camera: copy it inside the page (one command) and restore only the
// uncovered L-shaped remainder pixel-exactly. Returns 0 if the generic restore+redraw is needed.
void HudDamage(u8 page) __banked;
static u8 HudMove(u8 page)
{
 DrawObject* o=actors_old[page]+14;DrawObject* n=actors_next+14;
 i16 ddx=(i16)n->x-(i16)o->x,ddy=(i16)n->y-(i16)o->y,sx=o->x&255,sy=o->y&255,L,T;u16 base=(u16)page*256;u8 even;
 if(sx+72>256 || sy+32>256 || sx+ddx<0 || sx+ddx+72>256 || sy+ddy<0 || sy+ddy+32>256)return 0;
 g_VDP_Command.NX=72;g_VDP_Command.NY=32;g_VDP_Command.ARG=0;
 g_VDP_Command.SX=sx;g_VDP_Command.SY=base+sy;g_VDP_Command.DX=sx+ddx;g_VDP_Command.DY=base+sy+ddy;
 even=!((sx|(sx+ddx))&1);
 if(ddx>0){g_VDP_Command.SX+=even?70:71;g_VDP_Command.DX+=even?70:71;g_VDP_Command.ARG|=VDP_ARG_DIX_LEFT;}
 if(ddy>0){g_VDP_Command.SY+=31;g_VDP_Command.DY+=31;g_VDP_Command.ARG|=VDP_ARG_DIY_UP;}
 g_VDP_Command.CMD=even?VDP_CMD_HMMM:VDP_CMD_LMMM+VDP_OP_IMP;
 VDP_CommandSetupR32();
 L=(i16)o->x-camx[page];T=(i16)o->y-camy[page];
 if(ddx>0)RectX(page,camx[page],camy[page],L,T,L+ddx,T+32,1);
 if(ddx<0)RectX(page,camx[page],camy[page],L+72+ddx,T,L+72,T+32,1);
 if(ddy>0)RectX(page,camx[page],camy[page],L,T,L+72,T+ddy,1);
 if(ddy<0)RectX(page,camx[page],camy[page],L,T+32+ddy,L+72,T+32,1);
 *o=*n;o->visible=1;
 return 1;
}
static void Rows(u8 page,u16 cx,u16 cy,i16 l,i16 t,i16 r,i16 b)
{
    static u16 x,y,wx,wy,tile,base,w,part,edge;
    static u8 h,tx,ty,color;
    static u16 map;
    if(l<0)l=0;if(t<0)t=0;if(r>256)r=256;if(b>192)b=192;
    if(l>=r || t>=b)return;
    l&=~1;r=(r+1)&~1;base=(u16)page*256;
    for(y=t;y<b;y+=h)
    {
        wy=cy+y;ty=wy&15;h=16-ty;if(y+h>b)h=b-y;
        wx=cx+l;tx=wx&15;
        map=(wy>>4)*MAP_WIDTH+(wx>>4);
#if !FAST_RING_COPY
        g_VDP_Command.DY=base+(wy&255);g_VDP_Command.NY=h;
        g_VDP_Command.CMD=VDP_CMD_HMMM;
#endif
        g_VDP_Command.ARG=0;
        for(x=l;x<r;x+=w)
        {
            w=16-tx;if(x+w>r)w=r-x;
            tile=MapTile(map++);
            color=active_level==1?solid_tiles[tile]:255;
            if(color!=255){
                // One fill for adjacent uniform tiles of the same color.
                // Split at the circular page edge, including a full 256px run.
                edge=256-(wx&255);if(edge>r-x)edge=r-x;
                while(w<edge && solid_tiles[MapTile(map)]==color){
                    part=edge-w;if(part>16)part=16;
                    w+=part;++map;
                }
                g_VDP_Command.CLR=(color<<4)|color;
                g_VDP_Command.CMD=VDP_CMD_HMMV;
            }else{
                edge=256-(wx&255);if(edge>r-x)edge=r-x;
                if(!tx && w==16){
                    while(w+16<=edge && MapTile(map)==tile){w+=16;++map;}
                }
#if !FAST_RING_COPY
                g_VDP_Command.SX=tile_x[LEVEL_INDEX][tile]+tx;
                g_VDP_Command.SY=tile_y[LEVEL_INDEX][tile]+ty;
                g_VDP_Command.CMD=VDP_CMD_HMMM;
#endif
            }
#if FAST_RING_COPY
            if(color!=255){
#endif
            g_VDP_Command.DX=wx&255;g_VDP_Command.DY=base+(wy&255);
            g_VDP_Command.NY=h;g_VDP_Command.NX=(color==255 && w>16)?16:w;
#if FAST_RING_COPY
            }
#endif
            if(color==255){
#if FAST_RING_COPY
                RingCopy(tile,tx,ty,wx&255,base+(wy&255),(w>16?16:w),h);
#else
                VDP_CommandSetupR32();
#endif
            }else VDP_CommandSetupR36();
            if(color==255 && w>16)RepeatStrip(wx&255,base+(wy&255),w,h,1);
            wx+=w;tx=0;
        }
    }
}
// Exposed columns: keep column geometry fixed while traversing map rows.
static void Columns(u8 page,u16 l,u16 r)
{
    static u16 x,wx,wy,tile,y,base;
    static u8 w,h,tx,ty;
    static u16 map;
    base=(u16)page*256;
    for(x=l;x<r;x+=w)
    {
        wx=camera_px+x;tx=wx&15;w=16-tx;if(x+w>r)w=r-x;
        map=(camera_py>>4)*MAP_WIDTH+(wx>>4);
        wy=camera_py;ty=wy&15;
#if !FAST_RING_COPY
        g_VDP_Command.DX=wx&255;g_VDP_Command.NX=w;
        g_VDP_Command.CMD=VDP_CMD_HMMM;
#endif
        g_VDP_Command.ARG=0;
        for(y=0;y<192;y+=h)
        {
            h=16-ty;if(y+h>192)h=192-y;
            tile=MapTile(map);map+=MAP_WIDTH;
            {
                u8 color=active_level==1?solid_tiles[tile]:255;
                u16 edge=256-(wy&255);if(edge>192-y)edge=192-y;
                if(color!=255){
                    while(h+16<=edge && solid_tiles[MapTile(map)]==color){h+=16;map+=MAP_WIDTH;}
                    g_VDP_Command.CLR=(color<<4)|color;g_VDP_Command.CMD=VDP_CMD_HMMV;
                }else{
                    if(!ty && h==16){
                        while(h+16<=edge && MapTile(map)==tile){h+=16;map+=MAP_WIDTH;}
                    }
#if !FAST_RING_COPY
                    g_VDP_Command.SX=tile_x[LEVEL_INDEX][tile]+tx;
                    g_VDP_Command.SY=tile_y[LEVEL_INDEX][tile]+ty;
                    g_VDP_Command.CMD=VDP_CMD_HMMM;
#endif
                }
#if FAST_RING_COPY
                if(color!=255){
#endif
                g_VDP_Command.DX=wx&255;g_VDP_Command.NX=w;
                g_VDP_Command.DY=base+(wy&255);g_VDP_Command.NY=(color==255 && h>16)?16:h;
#if FAST_RING_COPY
                }
#endif
                if(color==255){
#if FAST_RING_COPY
                    RingCopy(tile,tx,ty,wx&255,base+(wy&255),w,h>16?16:h);
#else
                    VDP_CommandSetupR32();
#endif
                }else VDP_CommandSetupR36();
                if(color==255 && h>16)RepeatStrip(wx&255,base+(wy&255),w,h,0);
            }
            wy+=h;ty=0;
        }
    }
}
static void CleanCopy(u8 page,u8 slot,DrawObject* ob,u8 restore)
{
 const SpriteDef* sp=&sprites[LEVEL_INDEX][ob->frame];
 u16 x=(ob->x+SPR_OX(sp))&~1U,y=ob->y+SPR_OY(sp);
 u8 w=((ob->x+SPR_OX(sp)+sp->w+1)&~1U)-x;
 u8 h=sp->h,done=0,part,cy=((restore?camy[page]:camera_py)+192+(slot&1)*32)&255;
 while(done<h){
  part=h-done;if((u16)cy+part>256)part=256-cy;
  g_VDP_Command.SX=restore?(slot<2?96:224):x&255;
  g_VDP_Command.SY=(u16)page*256+(restore?cy:(y&255)+done);
  g_VDP_Command.DX=restore?x&255:(slot<2?96:224);
  g_VDP_Command.DY=(u16)page*256+(restore?(y&255)+done:cy);
  g_VDP_Command.NX=w;g_VDP_Command.NY=part;
  g_VDP_Command.ARG=0;g_VDP_Command.CMD=VDP_CMD_HMMM;
  VDP_CommandSetupR32();done+=part;cy=0;
 }
}
static u8 CleanRestore(u8 page,u8 id,DrawObject* ob)
{
 u8 slot;
 if(active_level>2 || (hw->scene_map_dirty&(1<<page)))return 0;
 for(slot=0;slot<4;++slot)if(ob->visible&(4<<slot)){CleanCopy(page,slot,ob,1);return 1;}
 return 0;
}
static void CleanSave(u8 page)
{
 u8 i,slot=0,w,limit=4;DrawObject* ob;const SpriteDef* sp;i16 x,y;
 {DrawObject* o=actors_old[page];for(i=0;i<ACTOR_COUNT;++i,++o)o->visible&=~60;}
 if(active_level>2 || !actors_next)return;
 // Tater masks/colors use the entire x128..255 texture width.
 if(active_level==2 && !FIXED_MINES_TEXTURES){
  for(i=0;i<ACTOR_COUNT;++i){ob=actors_next+i;
   if((ob->visible&1) && (ob->frame==128 || ob->frame==129 || ob->frame==134 || ob->frame==135))limit=2;
  }
  if(limit==4){
   w=hw->bitmap_cached[page];if(w==128 || w==129 || w==134 || w==135)hw->bitmap_cached[page]=255;
   w=hw->bitmap_second[page];if(w==128 || w==129 || w==134 || w==135)hw->bitmap_second[page]=255;
  }
 }
  for(i=15;i<26 && slot<limit;++i){
  ob=actors_next+i;
  if((ob->visible&3)!=3 || !((ob->frame>=16 && ob->frame<30) || (ob->frame>=90 && ob->frame<=93)))continue;
  sp=&sprites[LEVEL_INDEX][ob->frame];x=ob->x+SPR_OX(sp);y=ob->y+SPR_OY(sp);
  w=((x+sp->w+1)&~1)-(x&~1);
  if(x<(i16)camera_px || x+sp->w>(i16)camera_px+256 || y<(i16)camera_py || y+sp->h>(i16)camera_py+192 ||
     w>32 || sp->h>32 || ((x&~1)&255)+w>256 || (y&255)+sp->h>256)continue;
  ob->visible|=4<<slot;CleanCopy(page,slot,ob,0);++slot;
 }
}
// Precomputed tile-grid cost choices; clipped cases keep the original rectangle.
static u8 BandRestore(u8 page,u8 frame,i16 x,i16 y)
{
 u8 id,n,count;const u8* data;u16 phase;
 if(active_level>2 || frame>=166)return 0;
 id=restore_ids[LEVEL_INDEX][frame];if(id==255)return 0;
 phase=(((camy[page]+y)&15)<<5)|(((camx[page]+x)&15)<<1)|(x&1);
 if(!(restore_use[id][phase>>3]&(1<<(phase&7))))return 0;
 data=restore_bands[id];count=*data++;
 for(n=0;n<count;++n,data+=4)
  Rect(page,camx[page],camy[page],(x+data[0])&~1,y+data[1],(x+data[2]+1)&~1,y+data[3]);
 return 1;
}
void RingScene(u8 page) __banked
{
    static i16 dx,dy,l,r;
#if EXP_TEXTURE_SELECT
    // Geometry/visibility is now prepared: move only textures needed to draw.
    u8 texture_y=(camera_py+192)&255;
    if(hw->bitmap_y[page]!=texture_y){
        if(!CacheScroll(page,texture_y))hw->bitmap_cached[page]=hw->bitmap_second[page]=255;
        hw->bitmap_y[page]=texture_y;hw->bitmap_tall&=~(1<<page);
    }
#endif
    dx=camera_px-camx[page];dy=camera_py-camy[page];
    if(valid[page] && dx>-128 && dx<128 && dy>-96 && dy<96)
    {
        typedef struct {i16 l,t,r,b;} Damage;
        Damage pending[ACTOR_COUNT];Damage *a,*q;u8 count=0,i,j,k;
        DrawObject* ob=actors_old[page];const SpriteDef* sp;i16 x,y,ox,oy;u8 clipped;
        if(actors_next && (actors_next[14].visible&66)==64 && !HudMove(page)){
            HudDamage(page);actors_next[14].visible|=2;actors_old[page][14].visible|=2;
        }
        SlotArgs sl;u8 list[ACTOR_COUNT],nl,jl;
        // Z80 scan keeps only slots with some visible bit set (superset of visible&1).
        sl.a=(const u8*)ob+5;sl.db=sl.dc=0;sl.out=list;sl.n=sl.n0=ACTOR_COUNT;sl.count=0;
        nl=SlotScan(&sl);
        for(jl=0;jl<nl;++jl)if(i=list[jl],ob=actors_old[page]+i,(ob->visible&1) && (!actors_partial || (ob->visible&2))){
            sp=&sprites[LEVEL_INDEX][ob->frame];x=ob->x+SPR_OX(sp)-camx[page];y=ob->y+SPR_OY(sp)-camy[page];
            ox=x;oy=y;l=x+sp->w;r=y+sp->h;clipped=0;
            if(x<dx){x=dx;clipped=1;}if(y<dy){y=dy;clipped=1;}
            if(l>256+dx){l=256+dx;clipped=1;}if(r>192+dy){r=192+dy;clipped=1;}
            if(x<0){x=0;clipped=1;}if(y<0){y=0;clipped=1;}
            if(l>256){l=256;clipped=1;}if(r>192){r=192;clipped=1;}
            if(x>=l || y>=r || (!clipped && CleanRestore(page,i,ob)))continue;
#if EXP_RESTORE_BANDS
            if(!clipped && BandRestore(page,ob->frame,ox,oy))continue;
#endif
            a=pending+count++;a->l=x&~1;a->t=y;a->r=(l+1)&~1;a->b=r;
        }
        // Merge intersecting rectangles without increasing total restored area.
        for(i=0;i<count;++i){
            a=pending+i;
            for(j=i+1;j<count;){
                q=pending+j;
                if(a->l<q->r && q->l<a->r && a->t<q->b && q->t<a->b){
                    i16 nl=a->l<q->l?a->l:q->l,nt=a->t<q->t?a->t:q->t;
                    i16 nr=a->r>q->r?a->r:q->r,nb=a->b>q->b?a->b:q->b;
                    if((!actors_partial || (a->l==q->l && a->r==q->r) || (a->t==q->t && a->b==q->b) || (nl==a->l && nt==a->t && nr==a->r && nb==a->b) || (nl==q->l && nt==q->t && nr==q->r && nb==q->b)) && (u16)(nr-nl)*(nb-nt)<=(u16)(a->r-a->l)*(a->b-a->t)+(u16)(q->r-q->l)*(q->b-q->t)){
                        a->l=nl;a->t=nt;a->r=nr;a->b=nb;
                        --count;for(k=j;k<count;++k)pending[k]=pending[k+1];j=i+1;continue;
                    }
                }
                ++j;
            }
            Rect(page,camx[page],camy[page],a->l,a->t,a->r,a->b);
        }

        // Paint the entering columns, then entering rows over the shared width.
        l=0;r=256;
        if(dx>0){Columns(page,256-dx,256);r=256-dx;}
        if(dx<0){Columns(page,0,-dx);l=-dx;}
        if(dy>0)Rows(page,camera_px,camera_py,l,192-dy,r,192);
        if(dy<0)Rows(page,camera_px,camera_py,l,0,r,-dy);
        if(dx || dy)++scroll_copies;
    }
    else {Rows(page,camera_px,camera_py,0,0,256,192);++full_redraws;}
    CleanSave(page);ActorsDraw(page);
    valid[page]=1;camx[page]=camera_px;camy[page]=camera_py;
}
