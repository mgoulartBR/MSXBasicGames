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
// Adapted from kd_act1.c (Carrot Courier/flowers), kd_keen.c (Flower Power).
#include "combat.h"
#include "tomato.h"
#include "bridge.h"
#include "hill.h"
#include "mines.h"
#include "campaign.h"
CombatObject enemy,shots[2];
volatile u8 enemy_mode;
volatile u8 enemy_probe[4];
volatile u8 hero_frame,hero_visible,death_ticks,throw_ticks,combat_dirty;
volatile u16 shots_fired,flower_hits,flower_returns,defeats;
static u8 fire_previous,reset_previous;
// Static map: reuse a proven unchanged grounded physics step until position,
// direction or motion changes. Animation tick still advances at 70 Hz.
static u16 rest_x,rest_y;
static u8 rest_dir,rest_valid;
// Exact frame choices for each remaining lifetime, without software division.

static const u8 flower_frame[350]={27,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26};

void CombatReset(void) __banked
{
    InventoryReset();BonusReset();TomatoReset();BridgeReset();HillReset();MinesReset();RapidReset();PassReset();CityReset();AppleReset();GroveReset();BayReset();SwampReset();BoobusReset();CastleReset();rest_valid=0;
    // Nearest original Carrot Courier object in Horseradish Hill: tile 108,31.
    enemy.x=(active_level==6?50U:active_level==3?33U:active_level==2?31U:108U)*256;enemy.y=(active_level==6?32U:active_level==3?46U:active_level==2?87U:29U)*256;enemy.vx=enemy.vy=0;
    enemy.active=0;enemy.ground=1;enemy.state=0;enemy.dir=1;enemy.tick=0;enemy.timer=0;enemy.frame=21;
    shots[0].active=shots[1].active=0;death_ticks=throw_ticks=0;fire_previous=0;
    hero_frame=player_frame;hero_visible=1;combat_dirty=1;
}
static u8 Touch(u16 ax,u16 ay,u8 aw,u8 ah,u16 bx,u16 by,u8 bw,u8 bh)
{
    return (i16)(ax-bx)<(i16)bw*16 && (i16)(bx-ax)<(i16)aw*16 &&
        (i16)(ay-by)<(i16)bh*16 && (i16)(by-ay)<(i16)ah*16;
}
static void Carrot(CombatObject* a)
{
    static u16 ox,oy,oldbottom,newbottom,mid,surface;
    static i16 dx,dy,move,limit;
    static i8 y,top,bottom;
    static u8 wasground,wall,mx,old_dir;
    a->vx=0;
    if(active_level==1 && !a->active)return;
    if(active_level==3 && ((i16)(player_x-a->x)>5632 || (i16)(a->x-player_x)>5632 ||
       (i16)(player_y-a->y)>4608 || (i16)(a->y-player_y)>4608)){a->active=0;return;}
    if(!a->active)
    {
        if((i16)(player_y-a->y)>4608 || (i16)(a->y-player_y)>4608)return;
        if((i16)(player_x-a->x)>4608 || (i16)(a->x-player_x)>4608)return;
        a->active=1;
    }
    if(a->state==1)
    {
        if(--a->timer==0){a->state=0;a->ground=0;a->vy=0;++flower_returns;}
        else {
            a->frame=flower_frame[a->timer];
            oldbottom=a->y+496;
            if(simulation_ticks&1){a->vy+=3;if(a->vy>80)a->vy=80;}
            newbottom=oldbottom+a->vy;mid=a->x+184;mx=mid>>8;
            for(y=(oldbottom>>8)-1;y<=(i8)(newbottom>>8);++y)
            {
                wall=Wall(mx,y,0);if(!wall)continue;
                surface=(u16)y*256+Height(wall,(mid>>4)&15)-1;
                if((i16)(surface-oldbottom)>=-16 && (i16)(surface-newbottom)<=0)
                {a->y=surface-496;a->vy=0;return;}
            }
            a->y+=a->vy;
            if(a->vy<0 && Wall(mx,a->y>>8,2))a->vy=0;
            return;
        }
    }
    ox=a->x;oy=a->y;old_dir=a->dir;wasground=a->ground;dx=0;
    if(wasground)
    {
        if(++a->tick>=5){a->tick=0;dx=a->dir?128:-128;a->frame=((a->frame-(a->dir?21:16)+1)&3)+(a->dir?21:16);}
        if(a==&enemy && !dx && rest_valid && a->vy==0 && a->x==rest_x && a->y==rest_y && a->dir==rest_dir)return;
        if(a!=&enemy && !dx && a->timer==1 && a->vy==0)return;
        dy=(dx<0?-dx:dx)+16;
    }
    else
    {
        dx=a->dir?32:-32;
        if(simulation_ticks&1){a->vy+=3;if(a->vy>80)a->vy=80;}
        dy=a->vy;
    }
    a->x+=dx;a->y+=dy;oldbottom=oy+496;newbottom=a->y+496;mid=a->x+248;mx=mid>>8;
    a->ground=0;limit=-(dx<0?-dx:dx)-dy-16;
    for(y=(oldbottom>>8)-1;y<=(i8)(newbottom>>8);++y)
    {
        wall=Wall(mx,y,0);if(!wall)continue;
        surface=(u16)y*256+Height(wall,(mid>>4)&15)-1;move=(i16)(surface-newbottom);
        if(move<0 && move>=limit){a->y+=move;a->ground=1;a->vy=0;break;}
    }
    if(!a->ground && wasground){a->x=ox;a->y=oy;a->vy=-40;}
    if(!a->ground)
    {
        if(a->vy<0 && Wall(mx,a->y>>8,2)){a->y=((a->y>>8)+1)*256;a->vy=0;}
        a->frame=a->dir?25:20;
    }
    else if(!wasground)a->frame=a->dir?21:16;
    top=(a->y+32)>>8;bottom=(a->y+464)>>8;mx=(a->x+(a->dir?496:0))>>8;
    for(y=top;y<=bottom;++y)if(Wall(mx,y,a->dir?3:1))
    {a->x=ox;a->dir^=1;a->frame=a->dir?21:16;break;}
    if(a==&enemy)rest_valid=0;
    if(a==&enemy && wasground && !dx && a->ground && a->vy==0 && a->x==ox && a->y==oy && a->dir==old_dir){
        rest_x=a->x;rest_y=a->y;rest_dir=a->dir;rest_valid=1;
    }
    a->vx=(i16)(a->x-ox);
    if(a!=&enemy){a->timer=wasground && !dx && a->ground && a->vy==0 && a->x==ox && a->y==oy && a->dir==old_dir;if(a->x!=ox || a->y!=oy)combat_dirty=1;}
    if(active_level!=1 && (a->x<512 || a->x>((u16)MAP_WIDTH-4)*256U || a->y>((u16)MAP_HEIGHT-2)*256U)){a->x=(active_level==6?50U:active_level==3?33U:active_level==2?31U:108U)*256;a->y=(active_level==6?32U:active_level==3?46U:active_level==2?87U:29U)*256;a->ground=0;a->vy=0;rest_valid=0;}
}
// ClipToSpriteSide: relative motion chooses the side, never lethal contact.
static void CarrotContact(CombatObject* a)
{
 i16 left,right,dy;
 if(!a->active || a->state)return;
 dy=(i16)(player_y+48-a->y-32);
 if(dy>=448 || (i16)-dy>=448)return;
 left=(i16)(player_x+304-a->x-32);
 right=(i16)(a->x+480-player_x-64);
 if(left<=0 || right<=0)return;
 if((i16)(left-right)<player_vx-a->vx)GamePushSide(-left);
 else GamePushSide(right);
}
#include "campaign.h"
#include "inventory.c"

// Call-site gates: Carrot/CarrotContact return immediately for inactive Hill actors, but the call and
// its SDCC prologue cost ~0.2 ms each. Same effect, no call (Carrot sets vx=0 before returning).
#define CARROT_STEP(a) do{ if(active_level==1 && !(a)->active)(a)->vx=0; else Carrot(a); }while(0)
// Hill actors live at a fixed address: unrolled, absolute addressing, level already known to be 1.
#define HSTEP(n) do{ if(!hill_actors[n].active)hill_actors[n].vx=0; else Carrot(hill_actors+(n)); }while(0)
#define HCONTACT(n) do{ if(hill_actors[n].active && !hill_actors[n].state)CarrotContact(hill_actors+(n)); }while(0)
#define CARROT_CONTACT(a) do{ if((a)->active && !(a)->state)CarrotContact(a); }while(0)
void CombatTick(u8 input) __banked
{
    u8 i,fire;
    u8 reset=(input&32) && !reset_previous;
    reset_previous=input&32;
    static CombatObject* s;
    if(enemy.active || shots[0].active || shots[1].active || death_ticks || throw_ticks)combat_dirty=1;
    fire=(input&64)!=0;
    if(death_ticks)
    {
        --death_ticks;hero_visible=(death_ticks&8)==0;hero_frame=player_frame;
        if(!death_ticks)DeathFinish();
        fire_previous=fire;return;
    }
    if(reset){death_ticks=70;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;return;}
    if(grounded && (floor_type&~7)==8){death_ticks=70;shots[0].active=shots[1].active=0;combat_dirty=1;++defeats;SoundPlay(SFX_DEATH);return;}
    if(simulation_ticks&1){InventoryTick();BonusTick();}   // pickups at 35 Hz: Keen moves < 3 px per pair
    if(active_level==10)SwampObjectsTick();
    if(active_level<4 && !(enemy_mode&1)){enemy_probe[0]=0;CARROT_STEP(&enemy);if(active_level==1){HSTEP(0);HSTEP(1);HSTEP(2);HSTEP(3);}enemy_probe[1]=0;}
    if(!(enemy_mode&1) && tomato_count && !(simulation_ticks&3) && tomato_budget){--tomato_budget;TomatoTick();}
    if(active_level==3 && !(enemy_mode&1))BridgeTick();
    if(active_level==1 && !(enemy_mode&1))HillTick();
    if(active_level==2 && !(enemy_mode&1))MinesTick();
    if(active_level==4 && !(enemy_mode&1))RapidTick();
    if(active_level==5 && !(enemy_mode&1))PassTick();
    if(active_level==6 && !(enemy_mode&1))CityTick();
    if(active_level==7 && !(enemy_mode&1))AppleTick();
    if(active_level==12 && !(enemy_mode&1))CastleTick();
    if(active_level==11 && !(enemy_mode&1))BoobusTick();
    if(active_level==10 && !(enemy_mode&1))SwampTick();
    if(active_level==9 && !(enemy_mode&1))BayTick();
    if(active_level==8 && !(enemy_mode&1))GroveTick();
    if(throw_ticks)--throw_ticks;
    if(fire && !fire_previous && !(active_level==11?boobus_bombs:flower_ammo))++empty_throws;
    if(fire && !fire_previous && (active_level==11?boobus_bombs:flower_ammo))
    {
        for(i=0;i<2;++i)if(!shots[i].active)
        {
            s=&shots[i];if(active_level==11)--boobus_bombs;else --flower_ammo;s->active=1;s->state=0;s->x=player_x+(facing_left?0:256);s->y=player_y+128;
            s->vx=facing_left?-64:64;s->vy=-20;
            if(input&4){s->vx=0;s->vy=-64;}
            if((input&8) && !grounded){s->vx=0;s->vy=64;}
            if(climbing){
                s->x=player_x+(facing_left?-128:384);s->y=player_y;
                if(input&4){s->x=player_x+128;s->y=player_y-128;s->vx=0;s->vy=-64;}
                else if(input&8){s->x=player_x+128;s->y=player_y+128;s->vx=0;s->vy=64;}
            }
            s->timer=140;s->frame=active_level==11?102:32;throw_ticks=18;combat_dirty=1;++shots_fired;SoundPlay(active_level==11?SFX_BOMB:SFX_THROW);break;
        }
    }
    fire_previous=fire;
    if(shots[0].active)ShotTick(&shots[0]);   // ShotTick returns at once for inactive shots
    if(shots[1].active)ShotTick(&shots[1]);
    if(active_level<4 && !(enemy_mode&1)){
    enemy_probe[0]=0;
    CARROT_CONTACT(&enemy);
    if(active_level==1){HCONTACT(0);HCONTACT(1);HCONTACT(2);HCONTACT(3);}
    enemy_probe[1]=0;
    }
    hero_visible=1;hero_frame=throw_ticks?(facing_left?30:31):player_frame;
}

// Per-frame actor selection, moved out of the (full) fixed bank.
void LevelSelect(void) __banked
{
    TomatoSelect();if(active_level==12)CastleSelect();if(active_level==11)BoobusSelect();if(active_level==10)SwampSelect();if(active_level==9)BaySelect();if(active_level==8)GroveSelect();if(active_level==7)AppleSelect();if(active_level==6)CitySelect();if(active_level==5)PassSelect();if(active_level==4)RapidSelect();if(active_level==3)BridgeSelect();if(active_level==1)HillSelect();if(active_level==2)MinesSelect();
}
// One 35 Hz logic step: Keen physics once, enemies/items still see both tick parities.
void LogicStep2(u8 input) __banked
{
    Game_Tick2(input);
    if(level_done)return;
    --simulation_ticks;CombatTick(input);++simulation_ticks;
    if(!level_done)CombatTick(input);
}
void CityCarrotTick(CombatObject* a) __banked {Carrot(a);CarrotContact(a);}
