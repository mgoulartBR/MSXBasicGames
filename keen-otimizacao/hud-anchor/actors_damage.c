// SPDX-License-Identifier: GPL-2.0-or-later
#include "game.h"
#include "combat.h"
#include "world.h"
#include "combat_assets.h"
#include "hw_tomato.h"
#include "scanwin.inc"
extern DrawObject* actors_next;
extern volatile u8 hardware_scroll,front_page,drawn_frame;
extern volatile u16 camera_px,camera_py,drawn_x,drawn_y,drawn_camera_px,drawn_camera_py;
extern u16 ring_camera_x[2],ring_camera_y[2];
void ActorsCollect(u8 page) __banked;
void HudDamage(u8 page) __banked;
u8 HudDirty(u8 page) __banked;
// Stack-owned next-frame records live until DrawScene returns. No third RAM page.
// visible bit 0 = on screen; bit 1 = must restore/redraw this page.
typedef struct {i16 left,right,top,bottom;} ActorBounds;
// Clipping already calculates the geometry needed by damage propagation.
// Keep it in the stack-owned cache instead of discarding and recomputing it.
static u8 ClipBounds(DrawObject* ob,ActorBounds* bounds,u8 hero)
{
    const SpriteDef* sp=&sprites[LEVEL_INDEX][ob->frame];
    i16 x=ob->x+SPR_OX(sp),y=ob->y+SPR_OY(sp);
    i16 sx=x-camera_px,sy=y-camera_py;
    if(!(sx>-(i16)sp->w && sx<256 && sy>-(i16)sp->h && sy<(hardware_scroll?192:176)))return 0;
    if(hero){bounds->left=x&~15;bounds->right=(x+sp->w+15)&~15;bounds->top=y&~15;bounds->bottom=(y+sp->h+15)&~15;}
    else{bounds->left=x;bounds->right=x+sp->w;bounds->top=y;bounds->bottom=y+sp->h;}
    return 1;
}
// SDCC call(1): object HL, bounds DE, mode byte at SP+2.
// mode 0 = exact extent, 1 = tile-expanded hero, 2 = even restore.
// Preserve IX/IY and remove the one-byte stack argument on return.
#ifdef __SDCC
static void MakeBounds(DrawObject* ob,ActorBounds* bounds,u8 mode) __sdcccall(1) __naked
{
 ob;bounds;mode;
 __asm
    push ix
    push iy
    push hl
    pop ix
    push de
    pop iy
    ld hl,#6
    add hl,sp
    ld a,(hl)
    push af
    ld a,(_active_level)
    dec a
    ld l,a
    ld h,#0
    add hl,hl
    ld bc,#_sprites
    add hl,bc
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld l,4(ix)
    ld h,#0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,bc
    inc hl
    inc hl
    inc hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    and a,#127
    ld c,a
    inc hl
    ld a,(hl)
    and a,#127
    ld b,a
    push bc
    push de
    ld l,0(ix)
    ld h,1(ix)
    ld b,#0
    add hl,bc
    ld 0(iy),l
    ld 1(iy),h
    ld c,e
    add hl,bc
    ld 2(iy),l
    ld 3(iy),h
    pop de
    pop bc
    ld l,2(ix)
    ld h,3(ix)
    ld c,b
    ld b,#0
    add hl,bc
    ld 4(iy),l
    ld 5(iy),h
    ld c,d
    add hl,bc
    ld 6(iy),l
    ld 7(iy),h
    pop af
    or a,a
    jr z,geom956_done
    cp #1
    jr z,geom956_hero
    res 0,0(iy)
    ld l,2(iy)
    ld h,3(iy)
    inc hl
    res 0,l
    ld 2(iy),l
    ld 3(iy),h
    jr geom956_done
geom956_hero:
    ld a,0(iy)
    and a,#0xf0
    ld 0(iy),a
    ld a,4(iy)
    and a,#0xf0
    ld 4(iy),a
    ld de,#15
    add hl,de
    ld a,l
    and a,#0xf0
    ld 6(iy),a
    ld 7(iy),h
    ld l,2(iy)
    ld h,3(iy)
    add hl,de
    ld a,l
    and a,#0xf0
    ld 2(iy),a
    ld 3(iy),h
geom956_done:
    pop iy
    pop ix
    pop hl
    inc sp
    jp (hl)
 __endasm;
}
#else
// Portable oracle for host equivalence tests.
static void MakeBounds(DrawObject* ob,ActorBounds* bounds,u8 mode)
{
 const SpriteDef* sp=&sprites[LEVEL_INDEX][ob->frame];
 i16 x=ob->x+SPR_OX(sp),y=ob->y+SPR_OY(sp);
 if(mode==1){bounds->left=x&~15;bounds->right=(x+sp->w+15)&~15;bounds->top=y&~15;bounds->bottom=(y+sp->h+15)&~15;}
 else{bounds->left=mode==2?(x&~1):x;bounds->right=mode==2?((x+sp->w+1)&~1):x+sp->w;bounds->top=y;bounds->bottom=y+sp->h;}
}
#endif
static void GetBounds(DrawObject* ob,ActorBounds* bounds,u8 hero)
{
 MakeBounds(ob,bounds,hero?1:2);
}
static void GetTouchBounds(DrawObject* ob,ActorBounds* bounds,u8 hero)
{
 MakeBounds(ob,bounds,hero?1:0);
}
// SDCC call(1): HL/DE arguments, boolean result in A. Preserve IX/IY.
// Signed comparisons retain overflow correction and strict inequalities.
static u8 TouchCached(const ActorBounds* damage,const ActorBounds* candidate) __sdcccall(1) __naked
{
 damage;candidate;
 __asm
    push ix
    push iy
    push hl
    pop ix
    push de
    pop iy
    ld a,0(ix)
    sub a,2(iy)
    ld a,1(ix)
    sbc a,3(iy)
    jp po,touch955_cmp1
    xor a,#0x80
touch955_cmp1:
    jp p,touch955_false
    ld a,0(iy)
    sub a,2(ix)
    ld a,1(iy)
    sbc a,3(ix)
    jp po,touch955_cmp2
    xor a,#0x80
touch955_cmp2:
    jp p,touch955_false
    ld a,4(ix)
    sub a,6(iy)
    ld a,5(ix)
    sbc a,7(iy)
    jp po,touch955_cmp3
    xor a,#0x80
touch955_cmp3:
    jp p,touch955_false
    ld a,4(iy)
    sub a,6(ix)
    ld a,5(iy)
    sbc a,7(ix)
    jp po,touch955_cmp4
    xor a,#0x80
touch955_cmp4:
    jp p,touch955_false
    ld a,#1
    jr touch955_return
touch955_false:
    xor a,a
touch955_return:
    pop iy
    pop ix
    ret
 __endasm;
}
// New strips overwrite only the entering part of the circular bitmap.
// Unchanged world-aligned actors in the overlap retain their pixels.
static u8 Exposed(u8 page,DrawObject* ob)
{
 const SpriteDef* sp=&sprites[LEVEL_INDEX][ob->frame];
 i16 x=ob->x+SPR_OX(sp),y=ob->y+SPR_OY(sp);
 i16 dx=camera_px-ring_camera_x[page],dy=camera_py-ring_camera_y[page];
 if(dx>0 && x+sp->w>(i16)camera_px+256-dx)return 1;
 if(dx<0 && x<(i16)camera_px-dx)return 1;
 if(dy>0 && y+sp->h>(i16)camera_py+192-dy)return 1;
 if(dy<0 && y<(i16)camera_py-dy)return 1;
 return 0;
}
// The HUD panel moves with the camera. When nothing else is near it, RingScene copies
// the old panel to its new place with one VDP command instead of restoring + redrawing 18 pieces.
static u8 HudMoveClear(u8 page,DrawObject* next)
{
 DrawObject* o=actors_old[page]+14;DrawObject* n=next+14;DrawObject* rec;const SpriteDef* sp;
 i16 ddx=(i16)n->x-(i16)o->x,ddy=(i16)n->y-(i16)o->y,l,r,t,b,x,y;u8 idx[ACTOR_COUNT],i,j,k,c;ScanArgs sa;
 if(!(o->visible&1) || ddx>16 || ddx<-16 || ddy>4 || ddy<-4)return 0;
 l=(i16)(o->x<n->x?o->x:n->x)-2;r=(i16)(o->x>n->x?o->x:n->x)+74;
 t=(i16)(o->y<n->y?o->y:n->y)-2;b=(i16)(o->y>n->y?o->y:n->y)+34;
 // Sprite boxes start at x+ox (<=35) / y+oy (<=14) and are <=72x64: the Z80 window scan keeps
 // only records that can reach the panel boxes (+16 px margin) before the exact test.
 sa.out=idx;sa.ox=l-123;sa.oy=t-94;sa.w=(u16)(r-l)+139;sa.h=(u16)(b-t)+110;sa.stride=sizeof(DrawObject);sa.n=ACTOR_COUNT;
 // Old boxes of changed actors reach the panel only through damage propagation (the panel stays
 // a propagation candidate); unchanged ones equal their new records.
 for(k=1;k<2;++k){
  rec=next;sa.items=(const u8*)rec;sa.count=0;
  c=ScanWin(&sa);
  for(j=0;j<c;++j){
   i=idx[j];
   if(i==14)continue;
   rec=(k?next:actors_old[page])+i;
   if(!(rec->visible&1))continue;
   if(!k && (next[i].visible&1) && rec->x==next[i].x && rec->y==next[i].y && rec->frame==next[i].frame)continue;
   sp=&sprites[LEVEL_INDEX][rec->frame];x=rec->x+SPR_OX(sp);y=rec->y+SPR_OY(sp);
   if(x-16<r && x+(i16)sp->w+16>l && y-16<b && y+(i16)sp->h+16>t)return 0;
  }
  sa.out=idx;
 }
 return 1;
}
#include "slotscan.inc"
void ActorsPrepare(u8 page,DrawObject* next,u8 preserve) __banked
{
    ActorBounds bounds,previous,next_bounds[ACTOR_COUNT];
    u8 queue[ACTOR_COUNT],count=0,head=0;
    u8 candidates[ACTOR_COUNT],candidate_count=0,known[ACTOR_COUNT];
    u8 i,j,slot,compare,hud_dirty,had_old,has_new,hudmove=0,jj,visit;SlotArgs sl;
    DrawObject* old;DrawObject* ob;DrawObject* shown;DrawObject* prior;DrawObject* other;
    HWPrepare(page);
    // Scrolling must repaint all visible objects; collect and draw them once.
    if(!preserve && active_level!=2){actors_next=0;actors_partial=0;HudDamage(page);return;}
    actors_next=next;ActorsCollect(page);
    if(active_level==2){HWTomatoPrepare(page,next);if(!preserve){actors_partial=0;HudDamage(page);for(i=0;i<ACTOR_COUNT;++i)next[i].visible|=2;return;}}
    compare=drawn_camera_px==camera_px && drawn_camera_py==camera_py &&
       (((drawn_x>>4)==(player_x>>4) && (drawn_y>>4)==(player_y>>4) && drawn_frame==hero_frame) ||
        (hw->visible[page] && hw->visible[front_page])) &&
       !(hw->scene_map_dirty&(1<<front_page)) && hw->visible[page]==hw->visible[front_page] && !HudDirty(front_page) &&
       (active_level!=2 || (hw_tomato[page]==255 && hw_tomato[front_page]==255));
    hud_dirty=HudDirty(page);
    if(preserve==2){
        DrawObject* o14=actors_old[page]+14;
        // Anchored panel (HudAnchor) that did not move and is not under the entering strip: leave it alone.
        if(!hud_dirty && (o14->visible&1) && o14->x==next[14].x && o14->y==next[14].y && !Exposed(page,next+14)){}
        else if(hardware_scroll && !hud_dirty && HudMoveClear(page,next))hudmove=1;
        else{HudDamage(page);hud_dirty=1;}
    }
    old=actors_old[page];ob=next;shown=actors_old[front_page];
    // Empty slots (not drawn now, not drawn on either page) fall through every test below
    // without side effects; the Z80 scan lists the others (always including HUD slot 14).
    // `queue` doubles as the list: writes (count<=jj) never overtake reads.
    sl.a=(const u8*)next+5;sl.db=(u16)((const u8*)old-(const u8*)next);sl.dc=(u16)((const u8*)shown-(const u8*)next);
    sl.out=queue;sl.n=sl.n0=ACTOR_COUNT;sl.count=0;
    visit=SlotScan(&sl);
    for(jj=0;jj<visit;++jj){
        i=queue[jj];old=actors_old[page]+i;ob=next+i;shown=actors_old[front_page]+i;
        // Unchanged on-screen records already passed clipping on this page.
        if(ob->visible)known[i]=0;
        if(i!=14 && ob->visible && !((old->visible&1) && old->x==ob->x && old->y==ob->y && old->frame==ob->frame)){
            if(ClipBounds(ob,next_bounds+i,i==0))known[i]=1;
            else ob->visible=0;
        }
        if(compare && i!=14 && ((shown->visible&1)!=ob->visible || (ob->visible &&
           (shown->x!=ob->x || shown->y!=ob->y || shown->frame!=ob->frame))))compare=0;
        if(ob->visible&1)candidates[candidate_count++]=i;
        if(i==14 && (preserve!=2 || hudmove)){if(hud_dirty)ob->visible|=2;if(hudmove)ob->visible|=64;continue;}
        if(!(old->visible&1) && !ob->visible)continue;
        if((old->visible&1) && ob->visible && old->x==ob->x && old->y==ob->y && old->frame==ob->frame &&
           (preserve!=2 || !Exposed(page,ob)))continue;
        ob->visible|=2;queue[count++]=i;
    }
    if(compare){
        actors_partial=((drawn_x>>4)!=(player_x>>4) || (drawn_y>>4)!=(player_y>>4) || drawn_frame!=hero_frame)?3:2;
        return;
    }
    actors_partial=!(hw->scene_map_dirty&(1<<page));
    if(!actors_partial){HudDamage(page);for(i=0;i<ACTOR_COUNT;++i)next[i].visible|=2;return;}
    // Follow a bounded work queue of damaged rectangles. Each object enters
    // once; no repeated 29-by-29 search and no extra persistent RAM page.
    // Cache only candidates actually queried by the propagation queue.
    // Already damaged records and fast exits do not build unused bounds.
    for(i=0;i<count;++i)actors_old[page][queue[i]].visible|=2;
    // Already queued actors cannot become propagation candidates. Remove
    // them once, rather than rechecking them for every damaged rectangle.
    // Keep painter order and the HUD's old-page damage exception unchanged.
    for(j=0,i=0;j<candidate_count;++j){
        slot=candidates[j];
        if((next[slot].visible&2) && (slot!=14 || (actors_old[page][slot].visible&2)))continue;
        candidates[i++]=slot;
    }
    candidate_count=i;
    while(head<count){
        slot=queue[head++];prior=actors_old[page]+slot;other=next+slot;
        had_old=prior->visible&1;has_new=other->visible&1;
        if(had_old)GetBounds(prior,&previous,slot==0);
        if(has_new){
            if(known[slot]){bounds=next_bounds[slot];if(slot){bounds.left&=~1;bounds.right=(bounds.right+1)&~1;}}
            else GetBounds(other,&bounds,slot==0);
        }
        for(j=0;j<candidate_count;++j){
            i=candidates[j];old=actors_old[page]+i;ob=next+i;
            if(!(ob->visible&1) || ((ob->visible&2) && (i!=14 || (old->visible&2))))continue;
            if(!known[i]){GetTouchBounds(ob,next_bounds+i,i==0);known[i]=1;}
            if((had_old && TouchCached(&previous,next_bounds+i)) || (has_new && TouchCached(&bounds,next_bounds+i))){
                old->visible|=2;ob->visible|=2;queue[count++]=i;
                if(i==14)HudDamage(page);
            }
        }
    }

}

