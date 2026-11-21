// SPDX-License-Identifier: GPL-2.0-or-later
// Spud City's original object plane. No new persistent RAM allocation.
#include "actornext.inc"
#include "ticks.h"
#include "city.h"
#include "city_spawns.h"
#include "city_boxes.h"
#include "combat_assets.h"
#include "campaign.h"
#include "game.h"
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
static u8 CityBase(u8 i,u8 dir){return i<7?(dir?124:130):i<10?(dir?112:108):i==10?(dir?21:16):i==11?138:(dir?152:158);}
static u8 CityBox(u8 frame){return frame<26?frame-16:frame<116?10+frame-108:frame<136?18+frame-124:frame<144?30+frame-136:38+frame-152;}
static u8 CityTouch(CombatObject* a,u16 x,u16 y,u8 w,u8 h){const i8* b=city_boxes[CityBox(a->frame)];return (i16)x<=(i16)(a->x>>4)+b[2] && (i16)(x+w)>(i16)(a->x>>4)+b[0] && (i16)y<=(i16)(a->y>>4)+b[3] && (i16)(y+h)>(i16)(a->y>>4)+b[1];}
void CityReset(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=6)return;
 for(i=0;i<15;++i){a=city_actors+i;a->x=(u16)city_spawn[i][0]*256;a->y=(u16)city_spawn[i][1]*256;
 if(i<7)a->y-=497;else if(i==10 || i>=12)a->y-=512;
 a->vx=a->vy=0;a->timer=i==11?200:0;a->active=a->ground=a->state=a->tick=0;a->dir=1;a->frame=CityBase(i,1);}
 city_random=73;city_bombs=city_overflow=0;
 for(i=0;i<6;++i)city_rise[i]=0;
 for(i=0;i<4;++i)city_seeds[i].active=0;
 for(i=0;i<3;++i)pass_fry[i].active=0;
}
void CitySelect(void) __banked
{
 u8 i;CombatObject* a;i16 x,y;if(active_level!=6)return;
 for(i=0;i<15;++i){a=city_actors+i;x=(a->x>>4)-camera_px;y=(a->y>>4)-camera_py;
 if(a->active){if(x<-96 || x>352 || y<-96 || y>288)a->active=0;}
 else if(x>=-48 && x<=304 && y>=-48 && y<=240)a->active=1;}
 EnemyCache();
}
u8 CityHit(CombatObject* shot) __banked
{
 u8 i;CombatObject* a;if(active_level!=6)return 0;
 for(i=0;i<15;++i){a=city_actors+i;if(i==11 || !a->active || a->state==1 || !CityTouch(a,shot->x>>4,shot->y>>4,8,8))continue;
 if(i>=7 && i<10)a->y-=256;a->state=1;a->timer=350;a->frame=26;a->vy=0;a->tick=a->ground=0;shot->active=0;++flower_hits;SoundPlay(SFX_HIT);combat_dirty=1;return 1;}return 0;
}
static void CityMelon(CombatObject* a)
{
 u8 i;MinesSeed* seed;if(a->timer)--a->timer;
 if(a->timer==6)for(i=0;i<4;++i){seed=city_seeds+i;if(seed->active)continue;seed->x=a->x+128;seed->y=a->y+384;seed->vx=0;seed->vy=20;seed->active=1;seed->tick=0;seed->frame=142;break;}
 if(!a->timer)a->timer=212;a->frame=138+(a->timer<=12);
}
static u8 CityWall(u8 x,u8 y,u8 side){if(x>=MAP_WIDTH || y>=MAP_HEIGHT)return 1;return world_collision[352+(u16)world_collision[MapTile((u16)y*MAP_WIDTH+x)]*4+side];}
static u16 CityHeight(u8 s,u8 x){switch(s&7){case 0:return 256;case 1:return 0;case 2:return (u16)x*8;case 3:return 128+(u16)x*8;case 4:return (u16)x*16;case 5:return 120-(u16)x*8;case 6:return 248-(u16)x*8;default:return 240-(u16)x*16;}}
static void CityKill(void){if(!death_ticks){death_ticks=70;shots[0].active=shots[1].active=0;++defeats;SoundPlay(SFX_DEATH);combat_dirty=1;}}
static void CitySeed(MinesSeed* s)
{
 u16 ox=s->x,oy=s->y,bottom=s->y+112,mid,surface;u8 wall,mx;
 if(!s->active)return;
 s->x+=s->vx;s->y+=s->vy;if(++s->tick&1){s->vy+=3;if(s->vy>80)s->vy=80;}
 if(!(s->tick&3))s->frame^=1;
 if((i16)((s->x>>4)-camera_px)<-64 || (i16)((s->x>>4)-camera_px)>320 ||
    (i16)((s->y>>4)-camera_py)<-64 || (i16)((s->y>>4)-camera_py)>256){s->active=0;return;}
 mx=(s->x+(s->vx>0?112:0))>>8;
 if(s->vx && CityWall(mx,(s->y+48)>>8,s->vx>0?3:1)){s->x=ox;s->vx=-s->vx/2;}
 mid=s->x+56;mx=mid>>8;
 if(s->vy<0 && CityWall(mx,s->y>>8,2)){s->y=oy;s->vy=-s->vy/2;}
 wall=CityWall(mx,(s->y+112)>>8,0);
 if(wall && s->vy>=0){surface=((s->y+112)&0xff00)+CityHeight(wall,(mid>>4)&15);
  if(s->y+112>=surface && bottom<=surface+80){s->y=surface-113;MinesBounce(s,wall);}
 }
 if(s->active && (i16)(player_x+64-s->x)<128 && (i16)(s->x-player_x-64)<256 &&
    (i16)(player_y+48-s->y)<128 && (i16)(s->y-player_y-48)<448)CityKill();
}void CityTick(void) __banked
{
 u8 i;CombatObject* a;if(active_level!=6)return;
 for(i=ACTOR_FIRST(city_actors,15);i<15;i=ACTOR_AFTER(city_actors,15,i)){a=city_actors+i;combat_dirty=1;
 if(i<10)EnemyStep(a,i>=7);else if(i==10)CityCarrotTick(a);else if(i==11)CityMelon(a);else CityFrenchTick(a);
 if(death_ticks)return;}
 CityFriesTick();if(death_ticks)return;
 for(i=0;i<4;++i)if(city_seeds[i].active){combat_dirty=1;CitySeed(city_seeds+i);if(death_ticks)return;}
}
// Bomb rise countdown + pickup test, Z80. Touch box (same as the C original, mod 2^16):
//   -255 <= px+64-x <= 255  and  -447 <= py+48-y <= 255, with x,y = spawn tile * 256.
// t=px+0x13F-x must satisfy t<0x1FF; u=py+0x1EF-y must satisfy u<0x2BF (high byte first: most bombs are far).
static void CityBombsAsm(void) __naked
{
 __asm
 ld hl,#(_city_spawn+45)
 ld de,#(_world_map+9260)
 ld bc,#0x0601
cb_loop:
 ld a,(de)
 or a
 jr z,cb_norise
 dec a
 ld (de),a
 ld a,#1
 ld (_combat_dirty),a
cb_norise:
 ld a,(_world_map+9258)
 and c
 jr nz,cb_next
 ld a,(_player_x)
 add a,#0x3f
 ld a,(_player_x+1)
 adc a,#1
 sub (hl)
 cp #2
 jr nc,cb_next
 or a
 jr nz,cb_xedge
cb_xok:
 ld a,(_player_y)
 add a,#0xef
 ld a,(_player_y+1)
 adc a,#1
 inc hl
 sub (hl)
 dec hl
 cp #3
 jr nc,cb_next
 cp #2
 jr nz,cb_hit
 ld a,(_player_y)
 add a,#0xef
 cp #0xbf
 jr nc,cb_next
cb_hit:
 ld a,(_world_map+9258)
 or c
 ld (_world_map+9258),a
 ld a,#40
 ld (de),a
 push hl
 ld hl,(_boobus_bombs)
 inc hl
 ld (_boobus_bombs),hl
 pop hl
 ld a,#1
 ld (_combat_dirty),a
cb_next:
 inc de
 inc hl
 inc hl
 inc hl
 sla c
 dec b
 jp nz,cb_loop
 ret
cb_xedge:
 ld a,(_player_x)
 add a,#0x3f
 inc a
 jr nz,cb_xok
 jr cb_next
 __endasm;
}
void CityObjectsTick(void) __banked
{
 if(active_level!=6)return;if(Tick20Zero())combat_dirty=1;
 CityBombsAsm();
}
void CityLoseBombs(void) __banked {u8 i,bit;for(i=0,bit=1;i<6;++i,bit<<=1)if((city_bombs&bit) && boobus_bombs)--boobus_bombs;city_bombs=0;}
// Z80 version of the C original (collect City actors, the 3 fries, the 4 seeds and the bombs into 13 draw records).
// `ca_x` = out, `ca_r` = bomb frame (102+anim). Scratch: ca_map = records written, ca_tile = frame, ca_wx/ca_wy = x/y for `cc_put`.
static void CityCollectAsm(void) __naked
{
 __asm
 push ix
 ld hl,(_ca_x)
 ld de,#5
 add hl,de
 ld b,#13
 ld de,#6
 xor a
cc_clr:
 ld (hl),a
 add hl,de
 djnz cc_clr
 ld (_world_map+9257),a
 ld (_ca_map),a
 ld (_ca_map+1),a
 ld a,(_enemy_mode)
 and #2
 jp nz,cc_bombs
 ld ix,#(_world_map+9000)
 ld b,#15
cc_l1:
 push bc
 ld a,(ix+10)
 or a
 jr z,cc_n1
 ld c,(ix+11)
 ld a,b
 cp #5
 jr z,cc_f0
 ld a,(ix+13)
 cp #1
 ld a,#1
 jr nz,cc_f1
cc_f0:
 xor a
cc_f1:
 call cc_do
cc_n1:
 pop bc
 ld de,#16
 add ix,de
 djnz cc_l1
 ld ix,#(_world_map+9312)
 ld a,(_active_level)
 cp #6
 jr z,cc_fl
 ld ix,#(_world_map+9240)
cc_fl:
 ld b,#3
cc_l2:
 push bc
 ld a,(ix+10)
 or a
 jr z,cc_n2
 ld c,(ix+11)
 ld a,#1
 call cc_do
cc_n2:
 pop bc
 ld de,#16
 add ix,de
 djnz cc_l2
 ld ix,#(_world_map+9360)
 ld b,#4
cc_l3:
 push bc
 ld a,(ix+6)
 or a
 jr z,cc_n3
 ld c,(ix+7)
 ld a,#1
 call cc_do
cc_n3:
 pop bc
 ld de,#9
 add ix,de
 djnz cc_l3
cc_bombs:
 ld hl,#(_city_spawn+45)
 ld de,#(_world_map+9260)
 ld bc,#0x0601
cc_bomb:
 ld a,(_world_map+9258)
 and c
 jr z,cc_show
 ld a,(de)
 or a
 jr z,cc_bnext
cc_show:
 ld a,(de)
 ld (_ca_tile),a
 push hl
 push de
 push bc
 ld e,(hl)
 ld d,#0
 inc hl
 ld c,(hl)
 ld l,e
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld (_ca_wx),hl
 ld l,c
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 add hl,hl
 ld a,(_ca_tile)
 or a
 jr z,cc_norise
 ld c,a
 ld a,#40
 sub c
 srl a
 ld e,a
 ld d,#0
 or a
 sbc hl,de
 ld (_ca_wy),hl
 ld a,#89
 jr cc_bput
cc_norise:
 ld (_ca_wy),hl
 ld a,(_ca_r)
cc_bput:
 call cc_put
 pop bc
 pop de
 pop hl
cc_bnext:
 inc de
 inc hl
 inc hl
 inc hl
 sla c
 dec b
 jp nz,cc_bomb
 pop ix
 ret
cc_do:
 push af
 ld a,c
 ld (_ca_tile),a
 ld e,(ix+0)
 ld d,(ix+1)
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 srl d
 rr e
 ld l,(ix+2)
 ld h,(ix+3)
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 pop af
 or a
 jr z,cc_nooff
 push hl
 ex de,hl
 ld bc,#-24
 add hl,bc
 ex de,hl
 pop hl
 ld bc,#-4
 add hl,bc
cc_nooff:
 ld (_ca_wx),de
 ld (_ca_wy),hl
 ld a,(_ca_tile)
cc_put:
 push af
 ld l,a
 ld h,#0
 add hl,hl
 add hl,hl
 add hl,hl
 ld de,(_sprites+10)
 add hl,de
 call cc_clip
 or a
 jr z,cc_pend
 ld a,(_ca_map)
 cp #13
 jr nc,cc_ovf
 ld l,a
 ld h,#0
 ld d,h
 ld e,l
 add hl,hl
 add hl,de
 add hl,hl
 ld de,(_ca_x)
 add hl,de
 ld de,(_ca_wx)
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 ld de,(_ca_wy)
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 pop af
 ld (hl),a
 inc hl
 ld (hl),#1
 ld hl,#_ca_map
 inc (hl)
 ret
cc_ovf:
 ld a,#1
 ld (_world_map+9257),a
cc_pend:
 pop af
 ret
cc_clip:
 inc hl
 inc hl
 inc hl
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 inc hl
 ld a,(hl)
 and #127
 ld e,a
 inc hl
 ld a,(hl)
 and #127
 ld d,a
 ld a,e
 ld hl,(_ca_wx)
 add a,l
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 push de
 ld de,(_camera_px)
 or a
 sbc hl,de
 pop de
 ld a,h
 dec a
 cp #0x7F
 jr c,cc_hide
 ld a,l
 add a,c
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 bit 7,h
 jr nz,cc_hide
 or h
 or l
 jr z,cc_hide
 ld hl,(_ca_wy)
 ld a,d
 add a,l
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 ld de,(_camera_py)
 or a
 sbc hl,de
 ld a,h
 or a
 jr z,cc_ylow
 bit 7,a
 jr z,cc_hide
 jr cc_ytop
cc_ylow:
 ld a,(_hardware_scroll)
 or a
 ld a,#176
 jr z,cc_lim
 ld a,#192
cc_lim:
 cp l
 jr c,cc_hide
 jr z,cc_hide
cc_ytop:
 ld a,l
 add a,b
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 bit 7,h
 jr nz,cc_hide
 or l
 jr z,cc_hide
 ld a,#1
 ret
cc_hide:
 xor a
 ret
 __endasm;
}
void CityCollect(DrawObject* out) __banked
{
 ca_x=(u16)out;ca_r=102+(simulation_ticks/20)%3;
 CityCollectAsm();
}
