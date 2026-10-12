/* MSX DOOM E1M1 — motor para o mapa convertido do WAD do usuario (dados PRIVADOS gerados por tools/e1m1_build.py).
 * MSX turbo R / Z80 + V9968 + Geo3D (SDCC). Paredes texturizadas pelo Geo3D a partir de um PVS por bloco de 128 unidades,
 * monstros e itens como billboards do atlas de sprites, HUD da barra de status do Doom.
 * STATUS: BUILD OK + SIMULATED + OPENMSX (perfil sintetico C-BIOS/R800 com script). FPGA/hardware UNTESTED.
 * Portas dinamicas, grades mascaradas e pisos radioativos; tetos lisos, sem elevadores. */
#include "e1m1_assets.h"
typedef unsigned char u8; typedef unsigned int u16; typedef int s16; typedef long s32;

__sfr __at(0x98) vdp_d;
__sfr __at(0x99) vc;
__sfr __at(0x9A) vp;
__sfr __at(0x9B) vi;
__sfr __at(0x9C) vp4;
__sfr __at(0x9D) gi;
__sfr __at(0x9F) gd;
__sfr __at(0xA0) psg_a;
__sfr __at(0xA1) psg_d;
__sfr __at(0xAA) ppi_c;
__sfr __at(0xA9) ppi_b;

#define BANK(n) (*(volatile u8*)0x6000=(n))
#define DATA ((const u8*)0x4000)
#define SCR_W 256
#define VIEW_H 178
#define HUD_Y 178
#define BAR_DY 178
#define PLAYER_R 14
#define STEP_HEIGHT 24
#define TEXY_WALL 512
#define MAX_SPR 10
#define MAX_SHOTS 4

static void vr(u8 r,u8 v){ vc=v; vc=r|0x80; }
static void wait_ce(void){ while(vc&1); }
static void wait_vblank(void){ vr(15,0); (void)vc; while(!(vc&0x80)); vr(15,2); }
static void vw(s16 v){ vi=(u8)v; vi=(u8)((u16)v>>8); }
static void gw(s16 v){ gd=(u8)v; gd=(u8)((u16)v>>8); }

static u8 page;
static void fill(s16 x,s16 y,s16 w,s16 h,u8 c){
  if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
  if(x+w>SCR_W)w=SCR_W-x; if(y+h>212)h=212-y;
  if(w<=0||h<=0)return;
  wait_ce(); vr(17,36);
  vw(x); vw(y+((u16)page<<8)); vw(w); vw(h); vi=c; vi=0; vi=0xC0;
}
static void blit(u16 sx,u16 sy,u16 dx,u16 dy,u16 w,u16 h){
  wait_ce(); vr(17,32);
  vw(sx); vw(sy); vw(dx); vw(dy+((u16)page<<8)); vw(w); vw(h); vi=0; vi=0; vi=0x98;
}
static void gtransfer(const u8 *p,u16 n) __naked __sdcccall(1) {
  (void)p;(void)n;
  __asm
    ld c,#0x9F
    ld a,d
    or a
    jr z,tr_rest
tr_blk:
    ld b,#0
    otir
    dec a
    jr nz,tr_blk
tr_rest:
    ld a,e
    or a
    ret z
    ld b,e
    otir
    ret
  __endasm;
}
static void vtransfer16k(const u8 *p) __naked __sdcccall(1) {
  (void)p;
  __asm
    ld c,#0x98
    ld a,#64
vt_blk:
    ld b,#0
    otir
    dec a
    jr nz,vt_blk
    ret
  __endasm;
}
static void set_vram_write(u8 blk,u16 addr){ vr(14,blk); vc=(u8)addr; vc=((u8)(addr>>8)&0x3F)|0x40; }
s16 mulq14(s16 a,s16 b) __sdcccall(1);
void math_init(void);

/* ---------------------------------------------------------------- tabelas em RAM (copiadas do banco META no boot) */
typedef struct{ s16 floor,ceil; u8 f0,f1,c0,c1,special,sky,fid,cid; } Sec;
typedef struct{ s16 x,y; u8 kind; s16 ang; } ThingRom;
typedef struct{ s16 x,y,hp; u8 kind,st,t,on,vis; } Th;
typedef struct{ s16 ax,ay,bx,by; u8 sec,u,v,w,h; } Door;
static s16 sin_tab[256];
static Sec secs[NSEC];
Door doors[NDOOR];
u8 door_pos[NDOOR],door_state[NDOOR],door_hold[NDOOR];
static u16 grid[BNX*BNY];
Th th[NTHING];
typedef struct{ s16 x,y,z,dx,dy,dz; u8 on,t,ttl,impact; } Shot;
Shot shots[MAX_SHOTS];
static u8 next_shot;
static void cpy(void *d,const u8 *s,u16 n){ u8 *q=(u8*)d; while(n--)*q++=*s++; }
static void load_meta(void){
  BANK(META_BANK);
  cpy(sin_tab,DATA+META_SIN,512);
  cpy(secs,DATA+META_SEC,sizeof(secs));
  cpy(grid,DATA+META_GRID,sizeof(grid));
  cpy(doors,DATA+META_DOOR,sizeof(doors));
}

/* ---------------------------------------------------------------- matematica */
static s16 isin(u8 a){ return sin_tab[a]; }
static s16 icos(u8 a){ return sin_tab[(u8)(a+64)]; }
static s16 s16abs(s16 v){ return v<0?-v:v; }
static u8 rnd_s=77;
static u8 rnd(void){ rnd_s^=rnd_s<<3; rnd_s^=rnd_s>>5; rnd_s^=rnd_s<<4; return rnd_s; }
static u8 door_blocked(s16 x,s16 y,u8 radius){
  u8 i;
  for(i=0;i<NDOOR;i++)if(door_pos[i]<56){
    Door *d=&doors[i]; s16 cx=(d->ax+d->bx)/2,cy=(d->ay+d->by)/2;
    if(s16abs(x-cx)<=(s16abs(d->bx-d->ax)>>1)+radius+8&&
       s16abs(y-cy)<=(s16abs(d->by-d->ay)>>1)+radius+8)return 1;
  }
  return 0;
}
static u8 doors_in_box(s16 x0,s16 y0,s16 x1,s16 y1){
  u8 i;s16 t;
  if(x0>x1){t=x0;x0=x1;x1=t;}if(y0>y1){t=y0;y0=y1;y1=t;}
  for(i=0;i<NDOOR;i++)if(door_pos[i]<56){
    Door *d=&doors[i];s16 cx=(d->ax+d->bx)/2,cy=(d->ay+d->by)/2;
    s16 rx=(s16abs(d->bx-d->ax)>>1)+8,ry=(s16abs(d->by-d->ay)>>1)+8;
    if(x0<=cx+rx&&x1>=cx-rx&&y0<=cy+ry&&y1>=cy-ry)return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------- raster do mapa (8 unidades por celula): setor ou 255 = bloqueado */
static u8 rast(s16 x,s16 y){
  s16 cx=(x-RAST_X0)>>3,cy=(y-RAST_Y0)>>3; u8 b,r;
  if(cx<0||cy<0||cx>=RAST_W||cy>=RAST_H) return 255;
  b=rast_banks[cy]; r=cy-(u16)b*RAST_ROWS;
  BANK(RAST_BANK0+b); return DATA[rast_offsets[r]+cx];
}
static u8 can_stand(s16 x,s16 y,s16 r,s16 fl,u8 limit_drop){
  u8 s,i; s16 ox,oy;
  if(door_blocked(x,y,r))return 0;
  for(i=0;i<5;i++){ ox=(i&1)?r:-r; oy=(i&2)?r:-r; if(i==4)ox=oy=0;
    s=rast(x+ox,y+oy); if(s==255) return 0;
    if(secs[s].floor>fl+STEP_HEIGHT || (limit_drop&&secs[s].floor<fl-STEP_HEIGHT)) return 0; }
  return 1;
}
static s16 floor_at(s16 x,s16 y){ u8 s=rast(x,y); return s==255?0:secs[s].floor; }
static void try_move(s16 *x,s16 *y,s16 dx,s16 dy,s16 r,u8 limit_drop){
  s16 fl=floor_at(*x,*y);
  if(dx&&can_stand(*x+dx,*y,r,fl,limit_drop))*x+=dx;
  if(dy&&can_stand(*x,*y+dy,r,fl,limit_drop))*y+=dy;
}
static u8 los(s16 x0,s16 y0,s16 x1,s16 y1){
  s16 dx=x1-x0,dy=y1-y0,n=(s16abs(dx)+s16abs(dy))>>4,i;
  u8 test_doors;
  if(n<2) return 1;
  test_doors=doors_in_box(x0,y0,x1,y1);
  dx/=n; dy/=n;                                                         /* passo de ~16 unidades */
  for(i=1;i<n;i++){ x0+=dx; y0+=dy; if(rast(x0,y0)==255||(test_doors&&door_blocked(x0,y0,0)))return 0; }
  return 1;
}

/* ---------------------------------------------------------------- Geo3D */
static void geo_init(void){
  gi=0x18; gw(160); gw(128); gw(VIEW_H/2); gw(2); gw(256); gw(VIEW_H);
  gi=0x60; gw(0); gw(TEXY_WALL); gd=255;      /* TSTRIDE e 8 bits: nivel 1 (sprites) le a linha 512+255+v = 767+v; o atlas de sprites comeca na linha 768 (v=1) */
}
static void geo_light(const s16 *l){ gi=0x5A; gw(l[0]); gw(l[1]); gw(l[2]); }
static u8 cell_nv,cell_nf,cur_h; static u16 cur_blk=0xFFFF;
static void blk_load(u16 bi){
  const u8 *p; u8 nv,nf,bank; u16 off;
  BANK(META_BANK); p=DATA+META_BLK+3*bi; bank=p[0]; off=p[1]|((u16)p[2]<<8);
  BANK(bank); p=DATA+off; nv=p[0]; nf=p[1]; p+=2;
  gi=0x40; gd=0;gd=0;gd=nv;gd=0;gd=0;gd=8;
  gi=0x50; gtransfer(p,(u16)nv*6);
  gi=0x58; gd=0; gd=nf;
  gi=0x52; gtransfer(p+(u16)nv*6,(u16)nf*11);
  gi=0x65; gd=0;
  gi=0x53; gtransfer(p+(u16)nv*6+(u16)nf*11,(u16)nf*8);
  cell_nv=nv; cell_nf=nf; cur_blk=bi;
}
static void geo_wait(void){ u16 t=50000; while((gi&1)&&--t); }
static u8 geo_present(void){ return gi!=0xFF; }
static void geo_run(const s16 *m,s16 tx,s16 ty,s16 tz){
  u8 i; gi=0; for(i=0;i<9;i++)gw(m[i]); gw(tx); gw(ty); gw(tz);
  gi=0x46; gw((u16)page<<8);
  gi=0x48; gd=7;
  geo_wait(); wait_ce();
}

/* ---------------------------------------------------------------- estado */
s16 px,py,eye,hp,armor,ammo; u8 yaw,cool,fire_t,hurt,over,kills,items_got,tick,sfx,msg_t,face_t;
u8 dbg;
static const char *msg;
static u8 keys,old_keys,fcnt;
#define K_LEFT 1
#define K_RIGHT 2
#define K_UP 4
#define K_DOWN 8
#define K_FIRE 16
#define K_USE 32
#define K_TAB 64
static void read_keys(void){
  u8 a; old_keys=keys; keys=0;
  ppi_c=(ppi_c&0xF0)|8; a=~ppi_b;
  if(a&0x10)keys|=K_LEFT; if(a&0x80)keys|=K_RIGHT; if(a&0x20)keys|=K_UP; if(a&0x40)keys|=K_DOWN; if(a&1)keys|=K_FIRE;
  ppi_c=(ppi_c&0xF0)|7; a=~ppi_b; if(a&0x80)keys|=K_USE; if(a&8)keys|=K_TAB;
  ppi_c=(ppi_c&0xF0)|2; a=~ppi_b; if(a&0x40)keys|=K_USE;
}
static void psg(u8 r,u8 v){ psg_a=r; psg_d=v; }
#include "e1m1_sound.h"

/* tipos: 0 zombie, 1 sargento, 2 imp, 3.. itens */
#define K_ZOMBIE 0
#define K_SERGEANT 1
#define K_IMP 2
#define K_STIM 3
#define K_MEDIKIT 4
#define K_HBONUS 5
#define K_ABONUS 6
#define K_GARMOR 7
#define K_BARMOR 8
#define K_CLIP 9
#define K_BOX 10
#define K_SHELLS 11
#define K_SBOX 12
#define K_SHOTGUN 13
#define K_BARREL 14
#define K_LAMP 15
#define K_PILLAR 16
#define K_CAND 17
#define S_ASLEEP 0
#define S_CHASE 1
#define S_ATTACK 2
#define S_PAIN 3
#define S_DYING 4
#define S_DEAD 5
static const s16 mon_hp[3]={20,30,60};
static u8 total_mon,total_items;
static void say(const char *m,u8 t){ msg=m; msg_t=t; }
static void new_game(void){
  u8 i; ThingRom tr;
  sound_reset();
  px=START_X; py=START_Y; yaw=0; hp=100; armor=0; ammo=50; over=0; cool=fire_t=hurt=0; kills=items_got=0; msg_t=0; tick=0; cur_blk=0xFFFF; face_t=0;
  eye=floor_at(px,py)+41; total_mon=total_items=0;
  BANK(META_BANK);
  for(i=0;i<NTHING;i++){
    cpy(&tr,DATA+META_THING+i*7,7);
    th[i].x=tr.x; th[i].y=tr.y; th[i].kind=tr.kind; th[i].on=1; th[i].st=S_ASLEEP; th[i].t=i*3; th[i].vis=0; th[i].hp=tr.kind<3?mon_hp[tr.kind]:tr.kind==K_BARREL?20:0;
    if(tr.kind<3)total_mon++; else if(tr.kind>=K_STIM&&tr.kind<=K_SHOTGUN)total_items++;
  }
  for(i=0;i<MAX_SHOTS;i++)shots[i].on=0;
  next_shot=0;
  for(i=0;i<NDOOR;i++){door_pos[i]=0;door_state[i]=0;door_hold[i]=0;}
}
static void door_use(void){
  u8 i,best=255; s16 nearest=75;
  for(i=0;i<NDOOR;i++){
    Door *d=&doors[i];
    s16 cx=(d->ax+d->bx)/2,cy=(d->ay+d->by)/2;
    s16 dx=s16abs(px-cx)-(s16abs(d->bx-d->ax)>>1);
    s16 dy=s16abs(py-cy)-(s16abs(d->by-d->ay)>>1);
    s16 dist=(dx>0?dx:0)+(dy>0?dy:0);
    if(dist<nearest){nearest=dist;best=i;}
  }
  if(best!=255){door_state[best]=1;door_hold[best]=80;say("PORTA ABRINDO",24);}
}
static u8 door_occupied(const Door *d){
  u8 j;s16 cx=(d->ax+d->bx)/2,cy=(d->ay+d->by)/2;
  if(s16abs(px-cx)+s16abs(py-cy)<100)return 1;
  for(j=0;j<NTHING;j++)if(th[j].on&&th[j].kind<3&&s16abs(th[j].x-cx)+s16abs(th[j].y-cy)<100)return 1;
  return 0;
}
static void door_tick(void){
  u8 i; s16 fl;
  for(i=0;i<NDOOR;i++){
    Door *d=&doors[i];fl=secs[d->sec].ceil-secs[d->sec].floor;
    if(door_state[i]==1){
      if(door_pos[i]+8>=fl){door_pos[i]=fl;door_state[i]=2;}
      else door_pos[i]+=8;
    } else if(door_state[i]==2){
      if(door_occupied(d))door_hold[i]=80;
      if(door_hold[i])door_hold[i]--;else door_state[i]=3;
    } else if(door_state[i]==3){
      if(door_occupied(d)){door_state[i]=1;door_hold[i]=80;}
      else if(door_pos[i]<=8){door_pos[i]=0;door_state[i]=0;}
      else door_pos[i]-=8;
    }
  }
}
static void damage_player(s16 d){
  if(armor>0){ s16 a=d/3; if(a>armor)a=armor; armor-=a; d-=a; }
  hp-=d; hurt=6; sound_play(hp<=0?SFX_PLAYER_DEATH:SFX_PAIN); face_t=12;
}
static void wake_all(s16 range){
  u8 i; for(i=0;i<NTHING;i++) if(th[i].kind<3&&th[i].st==S_ASLEEP&&th[i].on){ s16 dx=th[i].x-px,dy=th[i].y-py; if(s16abs(dx)+s16abs(dy)<range) th[i].st=S_CHASE; }
}
static void kill_mon(u8 i){ th[i].st=S_DYING; th[i].t=0; kills++; sound_play(th[i].kind==K_IMP?SFX_IMP_DEATH:SFX_DEATH); }
static void hit_thing(u8 i,s16 d){
  if(th[i].st>=S_DYING)return;
  th[i].hp-=d;
  if(th[i].kind==K_BARREL){ if(th[i].hp<=0){ th[i].st=S_DYING; th[i].t=0; } return; }
  th[i].st=S_PAIN; th[i].t=6;
  if(th[i].hp<=0) kill_mon(i); else sound_play(SFX_MON_PAIN);
}
static s16 blast_damage(s16 x,s16 y,s16 ox,s16 oy){
  s16 dx=s16abs(x-ox),dy=s16abs(y-oy),d=dx>dy?dx:dy;
  return d<128&&los(ox,oy,x,y)?128-d:0;
}
static void explode(u8 i){
  u8 j; s16 d,x=th[i].x,y=th[i].y;
  if(hp>0){ d=blast_damage(px,py,x,y); if(d)damage_player(d); }
  for(j=0;j<NTHING;j++){
    if(j==i||!th[j].on||th[j].st>=S_DYING||(th[j].kind>=3&&th[j].kind!=K_BARREL))continue;
    d=blast_damage(th[j].x,th[j].y,x,y); if(d)hit_thing(j,d);
  }
  wake_all(900); sound_play(SFX_BARREL);
}
static void death_tick(u8 i){
  Th *m=&th[i];
  if(++m->t==4&&m->kind==K_BARREL)explode(i);
  if(m->t>=10){ m->st=S_DEAD; if(m->kind==K_BARREL)m->on=0; }
}
static void shoot(void){
  u8 i,best=255; s16 bz=32000,s=isin(yaw),c=icos(yaw);
  if(ammo<=0){ say("SEM MUNICAO!",40); return; }
  ammo--; cool=12; fire_t=8; sound_play(SFX_PISTOL); wake_all(900);
  for(i=0;i<NTHING;i++){ s16 dx,dy,xc,zc;
    if((th[i].kind>=3&&th[i].kind!=K_BARREL)||!th[i].on||th[i].st>=S_DYING)continue;
    dx=th[i].x-px; dy=th[i].y-py;
    xc=mulq14(dx,c)-mulq14(dy,s); zc=mulq14(dx,s)+mulq14(dy,c);        /* x lateral, z a frente */
    if(zc>8&&zc<bz&&xc>-20&&xc<20&&los(px,py,th[i].x,th[i].y)){bz=zc;best=i;}
  }
  if(best!=255) hit_thing(best,5+(rnd()%11));
}
static void pickup(u8 i){
  u8 k=th[i].kind;
  switch(k){
    case K_STIM: if(hp>=100)return; hp+=10; if(hp>100)hp=100; say("PEGOU ESTIMULANTE",60); break;
    case K_MEDIKIT: if(hp>=100)return; hp+=25; if(hp>100)hp=100; say("PEGOU KIT MEDICO",60); break;
    case K_HBONUS: if(hp<200)hp++; say("PEGOU BONUS DE SAUDE",40); break;
    case K_ABONUS: if(armor<200)armor++; say("PEGOU BONUS DE ARMADURA",40); break;
    case K_GARMOR: if(armor>=100)return; armor=100; say("PEGOU ARMADURA",60); break;
    case K_BARMOR: if(armor>=200)return; armor=200; say("PEGOU MEGA ARMADURA",60); break;
    case K_CLIP: ammo+=10; say("PEGOU MUNICAO",50); break;
    case K_BOX: ammo+=50; say("PEGOU CAIXA DE MUNICAO",50); break;
    case K_SHELLS: ammo+=4; say("PEGOU CARTUCHOS",50); break;
    case K_SBOX: ammo+=20; say("PEGOU CAIXA DE CARTUCHOS",50); break;
    case K_SHOTGUN: ammo+=8; say("PEGOU ESPINGARDA",60); break;
    default: return;
  }
  if(ammo>200)ammo=200; th[i].on=0; items_got++; sound_play(SFX_ITEM);
}
static void shot_explode(Shot *s){
  s->impact=1; s->t=0; sound_play(SFX_FIRE_IMPACT);
}
static void launch_imp(u8 i){
  s16 dx=px-th[i].x,dy=py-th[i].y,d=s16abs(dx),a=s16abs(dy);
  Shot *s=&shots[next_shot];
  next_shot=(next_shot+1)&(MAX_SHOTS-1);
  if(a>d)d=a; if(!d)d=1;
  s->x=th[i].x; s->y=th[i].y; s->z=floor_at(s->x,s->y)+40;
  s->dx=dx*10/d; s->dy=dy*10/d;
  s->dz=(eye-8-s->z)*10/d;
  s->ttl=100; s->t=0; s->impact=0; s->on=1;
}
static void shot_tick(void){
  u8 i,sec; s16 nx,ny,nz; Shot *s;
  for(i=0;i<MAX_SHOTS;i++){
    s=&shots[i]; if(!s->on)continue;
    if(s->impact){ if(++s->t>=6)s->on=0; continue; }
    if(!--s->ttl){s->on=0;continue;}
    nx=s->x+s->dx; ny=s->y+s->dy; nz=s->z+s->dz;
    if(s16abs(nx-px)<22&&s16abs(ny-py)<22&&s16abs(nz-eye)<24){
      s->x=nx;s->y=ny;s->z=nz;damage_player(9+(rnd()&15));shot_explode(s);continue;
    }
    sec=rast(nx,ny);
    if(sec==255||door_blocked(nx,ny,5)||!can_stand(nx,ny,5,floor_at(s->x,s->y),0)||nz<secs[sec].floor+3||nz>secs[sec].ceil-3){shot_explode(s);continue;}
    s->x=nx;s->y=ny;s->z=nz;s->t++;
  }
}
static void mon_attack(u8 i){
  s16 dx=px-th[i].x,dy=py-th[i].y,d=s16abs(dx)+s16abs(dy); u8 k=th[i].kind,n=1,j; s16 chance,dm;
  th[i].st=S_ATTACK; th[i].t=8; sound_play(k==K_IMP?SFX_FIRE:k==K_SERGEANT?SFX_SHOTGUN:SFX_PISTOL);
  if(k==K_IMP&&d>=80){launch_imp(i);return;}
  if(k==K_SERGEANT)n=3;
  chance=190-d/6; if(chance<60)chance=60;
  if(k==K_IMP&&d<110) chance=240;
  for(j=0;j<n;j++){ if((s16)rnd()<chance){ dm=(1+rnd()%5)*3; if(k==K_IMP)dm=(1+rnd()%8)*3; damage_player(dm); } }
}
static void mon_update(u8 i){
  Th *m=&th[i]; s16 dx=px-m->x,dy=py-m->y,ad=s16abs(dx)+s16abs(dy),sx,sy,sp;
  if(m->st==S_DEAD)return;
  if(m->st==S_DYING){ death_tick(i); return; }
  if(ad>1800&&m->st==S_ASLEEP)return;
  if(m->st==S_ASLEEP){ if(ad<1100&&(((fcnt+i)&3)==0)&&los(px,py,m->x,m->y)) m->st=S_CHASE; return; }
  if(m->st==S_PAIN||m->st==S_ATTACK){ if(--m->t==0)m->st=S_CHASE; return; }
  m->t++;
  sp=2;
  if(ad>56){
    sx=(s16abs(dx)>6)?(dx>0?sp:-sp):0; sy=(s16abs(dy)>6)?(dy>0?sp:-sp):0;
    if(s16abs(dx)>s16abs(dy)*2)sy=0; else if(s16abs(dy)>s16abs(dx)*2)sx=0;
    try_move(&m->x,&m->y,sx,sy,12,1);
  }
  if(((fcnt+i)&7)==0&&ad<1000&&los(m->x,m->y,px,py)){
    if(m->kind==K_IMP){ if(ad<90||(rnd()&3)==0) mon_attack(i); }
    else if((rnd()&3)==0) mon_attack(i);
  }
}
static void update(void){
  u8 i; s16 s=isin(yaw),c=icos(yaw),tgt; u8 sc;
  fcnt++; tick++;
  if(keys&K_LEFT)yaw-=3; if(keys&K_RIGHT)yaw+=3;
  if(keys&K_UP) try_move(&px,&py,mulq14(s,7),mulq14(c,7),PLAYER_R,0);
  if(keys&K_DOWN) try_move(&px,&py,-mulq14(s,6),-mulq14(c,6),PLAYER_R,0);
  if(cool)cool--; if(fire_t)fire_t--; if(hurt)hurt--; if(face_t)face_t--; if(msg_t&&!--msg_t)msg=0;
  if((keys&K_FIRE)&&!cool)shoot();
  if((keys&K_USE)&&!(old_keys&K_USE))door_use();
  door_tick();
  tgt=floor_at(px,py)+41; if(tgt>eye){ eye+=(tgt-eye+3)>>2; } else if(tgt<eye){ eye-=(eye-tgt+3)>>2; }
  for(i=0;i<NTHING;i++){
    if(!th[i].on)continue;
    if(th[i].kind<3) mon_update(i);
    else if(th[i].kind==K_BARREL){ if(th[i].st==S_DYING)death_tick(i); else th[i].t++; }
    else if(th[i].kind<=K_SHOTGUN){ s16 dx=px-th[i].x,dy=py-th[i].y; if(s16abs(dx)+s16abs(dy)<36) pickup(i); }
  }
  shot_tick();
  sc=rast(px,py);
  if(sc!=255){ u8 sp=secs[sc].special;
    if(sp==5||sp==7||sp==16||sp==4){ if((tick&31)==0){ damage_player(sp==7?5:sp==5?10:20); say("RADIACAO!",20); } } }
  { s16 dx=px-EXIT_X,dy=py-EXIT_Y; if(s16abs(dx)+s16abs(dy)<110){ if(!msg_t)say("APERTE ENTER PARA SAIR",8); if((keys&K_USE)&&hp>0)over=2; } }
  if(hp<=0){ hp=0; over=1; }
}

/* ---------------------------------------------------------------- HUD */
static const char fontchars[]=FONT_CHARS;
static u8 glyph(char ch){ u8 i; for(i=0;fontchars[i];i++) if(fontchars[i]==ch)return i; return 0; }
static void text(u16 x,u16 y,u16 row,const char *s){ for(;*s;s++,x+=6) blit((u16)glyph(*s)*6,row,x,y,5,7); }
static char *debug_number(char *p,s16 value){
  char digits[5]; u8 n=0; u16 magnitude=(u16)value;
  if(value<0){ *p++='-'; magnitude=0-magnitude; }
  do{ digits[n++]='0'+magnitude%10; magnitude/=10; }while(magnitude);
  while(n)*p++=digits[--n];
  return p;
}
static void debug_text(void){
  char line[24],*p=line; s16 angle=90-((u16)yaw*45)/32;
  if(angle<0)angle+=360;
  *p++='X'; p=debug_number(p,px); *p++=' '; *p++='Y'; p=debug_number(p,py);
  *p++=' '; *p++='A'; p=debug_number(p,angle); *p=0;
  fill(2,12,148,11,C_BLACK); text(4,14,FONT_GRAY,line);
}
static void number(s16 v,s16 xr,u8 pct){
  u8 d[4],n=0,i; s16 x; u16 w=digit_tab[0][2];
  if(v<0)v=0; if(v>999)v=999;
  do{ d[n++]=v%10; v/=10; }while(v);
  x=xr; if(pct){ blit(digit_tab[10][0],digit_tab[10][1],x,BAR_DY+3,digit_tab[10][2],digit_tab[10][3]); }
  for(i=0;i<n;i++){ x-=w; blit(digit_tab[d[i]][0],digit_tab[d[i]][1],x,BAR_DY+3,digit_tab[d[i]][2],digit_tab[d[i]][3]); }
}
static void hud_static(void){
  fill(0,HUD_Y,SCR_W,34,0); blit(BAR_X,BAR_Y,0,BAR_DY,BAR_W,BAR_H); blit(0,ARMS_Y,83,BAR_DY,ARMS_W,ARMS_H);
}
static s16 sh_hp=-1,sh_ar=-1,sh_am=-1; static u8 sh_face=255;
static u8 face_idx(void){ if(hp<=0)return 5; { s16 f=(100-hp)/20; if(f<0)f=0; if(f>4)f=4; return f; } }
static void hud_dynamic(void){
  u8 f=face_idx();
  if(hp!=sh_hp){ blit(BAR_X+38,BAR_Y,38,BAR_DY,45,BAR_H); number(hp,72,1); }
  if(armor!=sh_ar){ blit(BAR_X+140,BAR_Y,140,BAR_DY,50,BAR_H); number(armor,177,1); }
  if(ammo!=sh_am){ blit(BAR_X,BAR_Y,0,BAR_DY,38,BAR_H); number(ammo,35,0); }
  if(f!=sh_face){ blit(BAR_X+114,BAR_Y,114,BAR_DY,26,BAR_H); blit(face_tab[f][0],face_tab[f][1],face_tab[f][4],BAR_DY+face_tab[f][5],face_tab[f][2],face_tab[f][3]); }
}
static void hud_update(void){
  u8 p0=page;
  if(hp==sh_hp&&armor==sh_ar&&ammo==sh_am&&face_idx()==sh_face)return;
  for(page=0;page<2;page++)hud_dynamic();
  page=p0; sh_hp=hp; sh_ar=armor; sh_am=ammo; sh_face=face_idx();
}

/* ---------------------------------------------------------------- render */
static s16 M[9]; static s16 TX,TY,TZ; static s16 LC[3];
static const s16 LW[3]={-5734,11207,-10486};
static void make_view(void){
  s16 s=isin(yaw),c=icos(yaw);
  M[0]=c; M[1]=0; M[2]=-s; M[3]=0; M[4]=16384; M[5]=0; M[6]=s; M[7]=0; M[8]=c;
  TX=-(mulq14(c,px)-mulq14(s,py)); TY=-eye; TZ=-(mulq14(s,px)+mulq14(c,py));
  LC[0]=mulq14(M[0],LW[0])+mulq14(M[2],LW[2]); LC[1]=mulq14(M[4],LW[1]); LC[2]=mulq14(M[6],LW[0])+mulq14(M[8],LW[2]);
}
static void put16(u8 *p,s16 v){ p[0]=(u8)v; p[1]=(u8)((u16)v>>8); }
static u8 vbuf[24*MAX_SPR],fbuf[11*MAX_SPR],tbuf[8*MAX_SPR];
#define SPR_NX (-1204)
#define SPR_NY 2353
#define SPR_NZ (-2202)
static u8 mon_slot(const Th *m){
  if(m->st==S_DEAD)return mon_death[m->kind][4];
  if(m->st==S_DYING)return mon_death[m->kind][m->t>>1];
  if(m->st==S_PAIN)return mon_pain[m->kind];
  if(m->st==S_ATTACK)return mon_attack_frames[m->kind][m->t>5?0:m->t>2?1:2];
  if(m->st==S_ASLEEP)return mon_walk[m->kind][0];
  return mon_walk[m->kind][(m->t>>1)&3];
}
static void draw_gun(void){
  u8 f=0; const int *g;
  if(fire_t>5)f=1; else if(fire_t>3)f=2; else if(fire_t>1)f=3;
  g=gun_tab[f]; blit(g[0],g[1],g[4],g[5],g[2],g[3]);
  if(fire_t>5){ g=gun_tab[4]; blit(g[0],g[1],g[4],g[5],g[2],g[3]); }
}
static void overlay(void){
  fill(40,60,176,56,C_WHITE); fill(42,62,172,52,C_PANEL);
  if(over==2){ text(84,70,FONT_ORANGE,"E1M1 COMPLETA!"); }
  else text(92,70,FONT_RED,"VOCE MORREU");
  text(60,88,FONT_GRAY,"ABATES"); { char b[8]; u8 n=0; s16 k=kills; if(k>=10)b[n++]='0'+k/10; b[n++]='0'+k%10; b[n++]='/'; b[n++]='0'+total_mon; b[n]=0; text(110,88,FONT_GRAY,b); }
  text(60,100,FONT_GRAY,"APERTE ESPACO");
}
typedef struct{ s16 x0,y0,x1,y1,floor; } Water;
static u8 emit_water_rect(const Water *r,u8 k){
  u8 i,j,n=0,c,a,end,vi[4]; s16 x[4],y[4],z[4],ox[5],oy[5],dx,dy;
  u8 *v,*f,*t; s16 ww,hh,num,den,frac;
  x[0]=x[3]=r->x0;x[1]=x[2]=r->x1;y[0]=y[1]=r->y0;y[2]=y[3]=r->y1;
  for(i=0;i<4;i++){dx=x[i]-px;dy=y[i]-py;z[i]=mulq14(M[6],dx)+mulq14(M[8],dy);}
  for(i=0;i<4;i++){
    j=(i+1)&3;
    if(z[i]>=24){ox[n]=x[i];oy[n++]=y[i];}
    if((z[i]>=24)!=(z[j]>=24)){
      num=24-z[i];den=z[j]-z[i];if(den<0){num=-num;den=-den;}
      while(den>127){num>>=1;den>>=1;}
      frac=((num<<8)/den)<<6;
      ox[n]=x[i]+mulq14(x[j]-x[i],frac);
      oy[n++]=y[i]+mulq14(y[j]-y[i],frac);
    }
  }
  if(n<3)return k;
  end=k+(n==5?2:1);if(end>4)return k;
  ww=r->x1-r->x0;hh=r->y1-r->y0;
  for(i=0;k<end;k++,i++){
    v=vbuf+24*k;f=fbuf+11*k;t=tbuf+8*k;c=cell_nv+4*k;
    vi[0]=0;vi[1]=i?4:(n==3?2:3);vi[2]=i?3:(n==3?1:2);vi[3]=i?3:1;
    for(j=0;j<4;j++){
      a=vi[j];
      put16(v+6*j,ox[a]);put16(v+6*j+2,r->floor);put16(v+6*j+4,oy[a]);
      t[2*j]=(ox[a]-r->x0)*15/ww;t[2*j+1]=240+(oy[a]-r->y0)*15/hh;
      f[j]=c+j;
    }
    put16(f+4,0);put16(f+6,-16384);put16(f+8,0);f[10]=0x80;
  }
  return k;
}
static u8 emit_water(u8 sec){
  const Water *r;u8 i,k=0;s16 bx=(px&~127)-40,by=(py&~127)-40;
  if(sec==255||secs[sec].special!=7)return 0;
  BANK(META_BANK);r=(const Water *)(DATA+META_WATER);
  for(i=0;i<NWATER&&k<4;i++,r++)if(r->x0<bx+208&&r->x1>bx&&r->y0<by+208&&r->y1>by)k=emit_water_rect(r,k);
  return k;
}
static void emit_door(u8 i,u8 k){
  Door *d=&doors[i]; u8 *v=vbuf+24*k,*f=fbuf+11*k,*t=tbuf+8*k;
  s16 ax=d->ax,ay=d->ay,bx=d->bx,by=d->by,bot=secs[d->sec].floor+door_pos[i],top=secs[d->sec].ceil,tmp;
  u8 base=cell_nv+4*k,vbot;
  if((s32)(bx-ax)*(py-ay)-(s32)(by-ay)*(px-ax)>0){tmp=ax;ax=bx;bx=tmp;tmp=ay;ay=by;by=tmp;}
  put16(v,bx);put16(v+2,bot);put16(v+4,by);
  put16(v+6,ax);put16(v+8,bot);put16(v+10,ay);
  put16(v+12,ax);put16(v+14,top);put16(v+16,ay);
  put16(v+18,bx);put16(v+20,top);put16(v+22,by);
  f[0]=base;f[1]=base+1;f[2]=base+2;f[3]=base+3;
  put16(f+4,0);put16(f+6,-16384);put16(f+8,0);f[10]=0x80;
  vbot=d->v+(d->h-1)*(top-bot)/(top-secs[d->sec].floor);
  t[0]=d->u+d->w-1;t[1]=vbot;t[2]=d->u;t[3]=vbot;
  t[4]=d->u;t[5]=d->v;t[6]=d->u+d->w-1;t[7]=d->v;
}
static u8 add_sprites(u8 sec){
  u8 i,k=emit_water(sec),limit=MAX_SPR-k,n,cn=0,slot;
  u8 cand[MAX_SPR]; s16 cd[MAX_SPR]; u8 *v,*f,*t; s16 j;
  /* sprites: os MAX_SPR mais proximos dentro do campo de visao e com linha de visao */
  for(i=0;i<NTHING;i++){
    s16 dx,dy,zc,xc,d; Th *m=&th[i];
    if(!m->on)continue;
    dx=m->x-px; dy=m->y-py; d=s16abs(dx)+s16abs(dy); if(d>1500)continue;
    zc=mulq14(M[6],dx)+mulq14(M[8],dy); if(zc<12)continue;
    xc=mulq14(M[0],dx)+mulq14(M[2],dy); if(s16abs(xc)>zc+zc/2+40)continue;
    if(((fcnt+i)&3)==0) m->vis=los(px,py,m->x,m->y);
    if(!m->vis)continue;
    if(cn<limit){cand[cn]=i;cd[cn]=d;cn++;}
    else {u8 w=0;for(j=1;j<limit;j++)if(cd[j]>cd[w])w=j;if(d<cd[w]){cand[w]=i;cd[w]=d;}}
  }
  for(i=0;i<MAX_SHOTS;i++)if(shots[i].on){
    s16 dx=shots[i].x-px,dy=shots[i].y-py,d=s16abs(dx)+s16abs(dy),zc,xc;
    if(d>1500)continue;
    zc=mulq14(M[6],dx)+mulq14(M[8],dy); if(zc<12)continue;
    xc=mulq14(M[0],dx)+mulq14(M[2],dy); if(s16abs(xc)>zc+zc/2+40)continue;
    if(!los(px,py,shots[i].x,shots[i].y))continue;
    if(cn<limit){cand[cn]=NTHING+i;cd[cn]=d;cn++;}
    else {u8 w=0;for(j=1;j<limit;j++)if(cd[j]>cd[w])w=j;if(d<cd[w]){cand[w]=NTHING+i;cd[w]=d;}}
  }
  for(i=0;i<NDOOR;i++)if(door_pos[i]<secs[doors[i].sec].ceil-secs[doors[i].sec].floor){
    s16 dx=(doors[i].ax+doors[i].bx)/2-px,dy=(doors[i].ay+doors[i].by)/2-py;
    s16 d=s16abs(dx)+s16abs(dy),zc,xc;
    if(d>1500)continue;
    zc=mulq14(M[6],dx)+mulq14(M[8],dy);if(zc<12)continue;
    xc=mulq14(M[0],dx)+mulq14(M[2],dy);if(s16abs(xc)>zc+zc/2+96)continue;
    if(cn<limit){cand[cn]=NTHING+MAX_SHOTS+i;cd[cn]=d;cn++;}
    else {u8 w=0;for(j=1;j<limit;j++)if(cd[j]>cd[w])w=j;cand[w]=NTHING+MAX_SHOTS+i;cd[w]=d;}
  }
  for(n=0;n<cn;n++){
    Th *m; Shot *s; s16 hx,hy,fl,mx,my; u8 u,w,b,sw,sh;
    if(cand[n]>=NTHING+MAX_SHOTS){emit_door(cand[n]-NTHING-MAX_SHOTS,k++);continue;}
    if(cand[n]>=NTHING){
      s=&shots[cand[n]-NTHING]; mx=s->x;my=s->y;
      slot=SPR_BAL1A0+(s->impact?2+(s->t>>1):(s->t>>2)&1);
      fl=s->z-(spr_size[slot][2]+spr_size[slot][3])/2;
    } else {
      m=&th[cand[n]]; mx=m->x; my=m->y;
      slot=(m->kind<3)?mon_slot(m):kind_spr[m->kind];
      if(m->kind==K_BARREL)slot=m->st==S_DYING?SPR_BEXPA0+(m->t>>1):SPR_BAR1A0+((m->t>>2)&1);
      fl=floor_at(mx,my);
    }
    v=vbuf+24*k; f=fbuf+11*k; t=tbuf+8*k;
    hx=mulq14(M[0],spr_size[slot][0]); hy=mulq14(M[2],spr_size[slot][0]);
    put16(v,mx+hx);put16(v+2,fl+spr_size[slot][2]);put16(v+4,my+hy);
    put16(v+18,mx+hx);put16(v+20,fl+spr_size[slot][3]);put16(v+22,my+hy);
    hx=mulq14(M[0],spr_size[slot][1]); hy=mulq14(M[2],spr_size[slot][1]);
    put16(v+6,mx+hx);put16(v+8,fl+spr_size[slot][2]);put16(v+10,my+hy);
    put16(v+12,mx+hx);put16(v+14,fl+spr_size[slot][3]);put16(v+16,my+hy);
    b=cell_nv+4*k; f[0]=b;f[1]=b+1;f[2]=b+2;f[3]=b+3; put16(f+4,SPR_NX);put16(f+6,SPR_NY);put16(f+8,SPR_NZ); f[10]=0x80;
    u=spr_rect[slot][0]; w=spr_rect[slot][1]; sw=spr_rect[slot][2]-1; sh=spr_rect[slot][3]-1;
    t[0]=u;t[1]=w;t[2]=u+sw;t[3]=w;t[4]=u+sw;t[5]=w+sh;t[6]=u;t[7]=w+sh;
    k++;
  }
  return k;
}
static void render(void){
  u8 k; u16 bi; s16 bx,by;
  const Sec *sc; u8 s0=rast(px,py);
  { s16 sn=isin(yaw),cs=icos(yaw); u8 sa=rast(px+mulq14(sn,96),py+mulq14(cs,96)); if(sa==255)sa=s0;
    if(s0!=255){sc=&secs[sa];fill(0,0,SCR_W,89,secs[s0].c0);fill(0,89,SCR_W,45,sc->f0);fill(0,134,SCR_W,VIEW_H-134,sc->f1);}
  }
  if(s0==255){fill(0,0,SCR_W,89,C_PANEL);fill(0,89,SCR_W,89,C_BLACK);}
  wait_ce();make_view();
  bx=(px>>7)-BX0;by=(py>>7)-BY0;
  if(bx>=0&&by>=0&&bx<BNX&&by<BNY){bi=grid[by*BNX+bx];
    if(bi!=0xFFFF){u8 h=cur_h,d=(u8)(yaw-(u8)(cur_h<<6));
      if(d>=128)d=(u8)(256-d);
      if(cur_blk==0xFFFF||d>36)h=(u8)(((u8)(yaw+32)>>6)&3);
      bi=(bi<<2)|h;if(bi!=cur_blk){blk_load(bi);cur_h=h;}}
  }
  k=add_sprites(s0);
  geo_light(LC);
  gi=0x40; gd=cell_nv; gd=0; gd=cell_nv+4*k;
  gi=0x58; gd=cell_nf; gd=cell_nf+k;
  if(k){ gi=0x50; gtransfer(vbuf,(u16)24*k); gi=0x52; gtransfer(fbuf,(u16)11*k); gi=0x65; gd=cell_nf; gi=0x53; gtransfer(tbuf,(u16)8*k); }
  geo_run(M,TX,TY,TZ);
  draw_gun();
  if(msg) text(4,4,FONT_ORANGE,msg);
  if(hurt) { fill(0,0,SCR_W,2,C_RED); fill(0,VIEW_H-2,SCR_W,2,C_RED); }
  if(over) overlay();
  if(dbg) debug_text();
}

static void no_geo(void){
  page=0; fill(0,0,SCR_W,HUD_Y,C_PANEL);
  text(46,70,FONT_ORANGE,"GEO3D NAO ENCONTRADO"); text(46,86,FONT_GRAY,"USE EXT GEO3D NO OPENMSX"); text(46,102,FONT_GRAY,"TIPO DE ROM ASCII16");
  vr(2,0x1F); vr(1,0x40); for(;;);
}

void main(void){
  u8 i; u16 n;
  math_init();
  vp4=0;
  vr(0,14); vr(1,0); vr(2,31); vr(7,0); vr(8,10); vr(9,128);
  vr(21,0); vr(20,0x11);
  vr(51,0);vr(52,0);vr(53,0);vr(54,0);vr(55,255);vr(56,1);vr(57,255);vr(58,3);   /* janela de origem LRMM: X 0..511 (o endereco da VRAM faz wrap modulo 256), Y 0..1023 */
  vr(16,0);
  BANK(META_BANK); { const u8 *pp=DATA+META_PAL; for(n=0;n<768;n++)vp=pp[n]; }
  vr(15,2);
  set_vram_write(3,0x1400); BANK(AREAA_BANK); vtransfer16k(DATA);          /* linhas 212..275: tiles dos flats (as 256.. sao limpas depois) */
  set_vram_write(7,0x1400); BANK(AREAB_BANK); vtransfer16k(DATA);          /* linhas 468..531 (fontes); as 512.. sao sobrescritas a seguir */
  set_vram_write(8,0);
  for(i=0;i<NVRAM_BANKS;i++){ BANK(VRAM_BANK0+i); vtransfer16k(DATA); }
  sound_init(); load_meta(); new_game(); keys=old_keys=dbg=0;
  for(page=0;page<2;page++){ fill(0,0,SCR_W,212,0); hud_static(); }
  vr(1,0x40);
  if(!geo_present()) no_geo();
  geo_init();
  page=0; sh_hp=-1; sh_ar=-1; sh_am=-1; sh_face=255; hud_update();
  for(;;){
    read_keys();
    if((keys&K_TAB)&&!(old_keys&K_TAB))dbg^=1;
    if(over){
      if((keys&K_FIRE)&&!(old_keys&K_FIRE)){ new_game(); sh_hp=-1; sh_ar=-1; sh_am=-1; sh_face=255; }
      else for(i=0;i<NTHING;i++)if(th[i].on&&th[i].st==S_DYING)death_tick(i);
    } else update();
    page^=1; render(); hud_update(); wait_ce();
    wait_vblank(); vr(2,0x1F|((u16)page<<5));
    sound_tick();
  }
}
