/* MSX DOOM E1M1 — motor para o mapa convertido do WAD do usuario (dados PRIVADOS gerados por tools/e1m1_build.py).
 * MSX turbo R / Z80 + V9968 + Geo3D (SDCC). Paredes texturizadas pelo Geo3D a partir de um PVS por bloco de 128 unidades,
 * monstros e itens como billboards do atlas de sprites, HUD da barra de status do Doom.
 * STATUS: BUILD OK + SIMULATED. Nunca executado em openMSX/FPGA/hardware (UNTESTED).
 * Fase 1: portas abertas para sempre, pisos/tetos como faixas de cor, sem iluminacao por setor, sem elevadores. */
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
#define BAR_DY 182
#define PLAYER_R 14
#define TEXY_WALL 512
#define MAX_SPR 10

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

/* ---------------------------------------------------------------- tabelas em RAM (copiadas do banco META no boot) */
typedef struct{ s16 floor,ceil; u8 f0,f1,c0,c1,special,sky,fid,cid; } Sec;
typedef struct{ s16 x,y; u8 kind; s16 ang; } ThingRom;
typedef struct{ s16 x,y,hp; u8 kind,st,t,on,vis; } Th;
static s16 sin_tab[256];
static Sec secs[NSEC];
static u16 grid[BNX*BNY];
Th th[NTHING];
static void cpy(void *d,const u8 *s,u16 n){ u8 *q=(u8*)d; while(n--)*q++=*s++; }
static void load_meta(void){
  BANK(META_BANK);
  cpy(sin_tab,DATA+META_SIN,512);
  cpy(secs,DATA+META_SEC,sizeof(secs));
  cpy(grid,DATA+META_GRID,sizeof(grid));
}

/* ---------------------------------------------------------------- matematica */
static s16 isin(u8 a){ return sin_tab[a]; }
static s16 icos(u8 a){ return sin_tab[(u8)(a+64)]; }
static s16 s16abs(s16 v){ return v<0?-v:v; }
static u8 rnd_s=77;
static u8 rnd(void){ rnd_s^=rnd_s<<3; rnd_s^=rnd_s>>5; rnd_s^=rnd_s<<4; return rnd_s; }

/* ---------------------------------------------------------------- raster do mapa (8 unidades por celula): setor ou 255 = bloqueado */
static u8 rast(s16 x,s16 y){
  s16 cx=(x-RAST_X0)>>3,cy=(y-RAST_Y0)>>3; u8 b; u16 r;
  if(cx<0||cy<0||cx>=RAST_W||cy>=RAST_H) return 255;
  b=0; r=cy; while(r>=RAST_ROWS){ r-=RAST_ROWS; b++; }
  BANK(RAST_BANK0+b); return DATA[r*RAST_W+cx];
}
static u8 can_stand(s16 x,s16 y,s16 r,s16 fl){
  u8 s,i; s16 ox,oy;
  for(i=0;i<5;i++){ ox=(i&1)?r:-r; oy=(i&2)?r:-r; if(i==4)ox=oy=0;
    s=rast(x+ox,y+oy); if(s==255) return 0; if(secs[s].floor>fl+24) return 0; }
  return 1;
}
static s16 floor_at(s16 x,s16 y){ u8 s=rast(x,y); return s==255?0:secs[s].floor; }
static void try_move(s16 *x,s16 *y,s16 dx,s16 dy,s16 r){
  s16 fl=floor_at(*x,*y);
  if(dx&&can_stand(*x+dx,*y,r,fl))*x+=dx;
  if(dy&&can_stand(*x,*y+dy,r,fl))*y+=dy;
}
static u8 los(s16 x0,s16 y0,s16 x1,s16 y1){
  s16 dx=x1-x0,dy=y1-y0,n=(s16abs(dx)+s16abs(dy))>>4,i;
  if(n<2) return 1;
  dx/=n; dy/=n;                                                         /* passo de ~16 unidades */
  for(i=1;i<n;i++){ x0+=dx; y0+=dy; if(rast(x0,y0)==255) return 0; }
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
static u8 geo_ok=1;
static void geo_wait(void){ u16 t=50000; while((gi&1)&&--t); if(!t)geo_ok=0; }
static u8 geo_present(void){ return gi!=0xFF; }
static void geo_run(const s16 *m,s16 tx,s16 ty,s16 tz){
  u8 i; gi=0; for(i=0;i<9;i++)gw(m[i]); gw(tx); gw(ty); gw(tz);
  gi=0x46; gw((u16)page<<8);
  gi=0x48; gd=7;
  geo_wait(); wait_ce();
}

/* ---------------------------------------------------------------- estado */
s16 px,py,eye,hp,armor,ammo; u8 yaw,cool,fire_t,hurt,over,kills,items_got,tick,sfx,msg_t,face_t;
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
}
static void psg(u8 r,u8 v){ psg_a=r; psg_d=v; }
static void sound_tick(void){
  if(sfx){ psg(6,15); psg(7,0xB7); psg(8,sfx>15?15:sfx); sfx-=2; if(sfx>200)sfx=0; } else psg(8,0);
}

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
  px=START_X; py=START_Y; yaw=0; hp=100; armor=0; ammo=50; over=0; cool=fire_t=hurt=0; kills=items_got=0; msg_t=0; tick=0; cur_blk=0xFFFF; face_t=0;
  eye=floor_at(px,py)+41; total_mon=total_items=0;
  BANK(META_BANK);
  for(i=0;i<NTHING;i++){
    cpy(&tr,DATA+META_THING+i*7,7);
    th[i].x=tr.x; th[i].y=tr.y; th[i].kind=tr.kind; th[i].on=1; th[i].st=S_ASLEEP; th[i].t=i*3; th[i].vis=0; th[i].hp=tr.kind<3?mon_hp[tr.kind]:0;
    if(tr.kind<3)total_mon++; else if(tr.kind>=K_STIM&&tr.kind<=K_SHOTGUN)total_items++;
  }
}
static void damage_player(s16 d){
  if(armor>0){ s16 a=d/3; if(a>armor)a=armor; armor-=a; d-=a; }
  hp-=d; hurt=6; sfx=40; face_t=12;
}
static void wake_all(s16 range){
  u8 i; for(i=0;i<NTHING;i++) if(th[i].kind<3&&th[i].st==S_ASLEEP&&th[i].on){ s16 dx=th[i].x-px,dy=th[i].y-py; if(s16abs(dx)+s16abs(dy)<range) th[i].st=S_CHASE; }
}
static void kill_mon(u8 i){ th[i].st=S_DYING; th[i].t=10; kills++; sfx=60; }
static void hit_thing(u8 i,s16 d){
  if(th[i].kind==K_BARREL) return;
  if(th[i].st>=S_DYING)return;
  th[i].hp-=d; th[i].st=S_PAIN; th[i].t=6;
  if(th[i].hp<=0) kill_mon(i);
}
static void shoot(void){
  u8 i,best=255; s16 bz=32000,s=isin(yaw),c=icos(yaw);
  if(ammo<=0){ say("SEM MUNICAO!",40); return; }
  ammo--; cool=12; fire_t=8; sfx=30; wake_all(900);
  for(i=0;i<NTHING;i++){ s16 dx,dy,xc,zc;
    if(th[i].kind>=3||!th[i].on||th[i].st>=S_DYING)continue;
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
  if(ammo>200)ammo=200; th[i].on=0; items_got++; sfx=24;
}
static void mon_attack(u8 i){
  s16 dx=px-th[i].x,dy=py-th[i].y,d=s16abs(dx)+s16abs(dy); u8 k=th[i].kind,n=1,j; s16 chance,dm;
  th[i].st=S_ATTACK; th[i].t=8; sfx=36;
  if(k==K_SERGEANT)n=3;
  chance=190-d/6; if(chance<60)chance=60;
  if(k==K_IMP&&d<110) chance=240;
  for(j=0;j<n;j++){ if((s16)rnd()<chance){ dm=(1+rnd()%5)*3; if(k==K_IMP)dm=(1+rnd()%8)*3; damage_player(dm); } }
}
static void mon_update(u8 i){
  Th *m=&th[i]; s16 dx=px-m->x,dy=py-m->y,ad=s16abs(dx)+s16abs(dy),sx,sy,sp;
  if(m->st==S_DEAD)return;
  if(m->st==S_DYING){ if(--m->t==0)m->st=S_DEAD; return; }
  if(ad>1800&&m->st==S_ASLEEP)return;
  m->t++;
  if(m->st==S_ASLEEP){ if(ad<1100&&(((fcnt+i)&3)==0)&&los(px,py,m->x,m->y)) m->st=S_CHASE; return; }
  if(m->st==S_PAIN||m->st==S_ATTACK){ if(--m->t==0)m->st=S_CHASE; return; }
  sp=2;
  if(ad>56){
    sx=(s16abs(dx)>6)?(dx>0?sp:-sp):0; sy=(s16abs(dy)>6)?(dy>0?sp:-sp):0;
    if(s16abs(dx)>s16abs(dy)*2)sy=0; else if(s16abs(dy)>s16abs(dx)*2)sx=0;
    try_move(&m->x,&m->y,sx,sy,12);
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
  if(keys&K_UP) try_move(&px,&py,mulq14(s,7),mulq14(c,7),PLAYER_R);
  if(keys&K_DOWN) try_move(&px,&py,-mulq14(s,6),-mulq14(c,6),PLAYER_R);
  if(cool)cool--; if(fire_t)fire_t--; if(hurt)hurt--; if(face_t)face_t--; if(msg_t&&!--msg_t)msg=0;
  if((keys&K_FIRE)&&!cool)shoot();
  tgt=floor_at(px,py)+41; if(tgt>eye){ eye+=(tgt-eye+3)>>2; } else if(tgt<eye){ eye-=(eye-tgt+3)>>2; }
  for(i=0;i<NTHING;i++){
    if(!th[i].on)continue;
    if(th[i].kind<3) mon_update(i);
    else if(th[i].kind<=K_SHOTGUN){ s16 dx=px-th[i].x,dy=py-th[i].y; if(s16abs(dx)+s16abs(dy)<36) pickup(i); }
  }
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
static void number(s16 v,s16 xr,u8 pct){
  u8 d[4],n=0,i; s16 x; u16 w=digit_tab[0][2];
  if(v<0)v=0; if(v>999)v=999;
  do{ d[n++]=v%10; v/=10; }while(v);
  x=xr; if(pct){ blit(digit_tab[10][0],digit_tab[10][1],x,BAR_DY+3,digit_tab[10][2],digit_tab[10][3]); }
  for(i=0;i<n;i++){ x-=w; blit(digit_tab[d[i]][0],digit_tab[d[i]][1],x,BAR_DY+3,w,digit_tab[0][3]); }
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
  if(ammo!=sh_am){ blit(BAR_X,BAR_Y,0,BAR_DY,40,BAR_H); number(ammo,35,0); }
  if(f!=sh_face){ blit(BAR_X+115,BAR_Y,115,BAR_DY,24,BAR_H); blit(face_tab[f][0],face_tab[f][1],114,BAR_DY+1,face_tab[f][2],face_tab[f][3]); }
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
  u8 base=m->kind*6;                                                 /* zombie 0, sargento 6, imp 12 */
  if(m->st==S_DEAD)return base+5;
  if(m->st==S_DYING)return base+4;
  if(m->st==S_PAIN)return base+3;
  if(m->st==S_ATTACK)return base+2;
  return base+((m->t>>2)&1);
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
/* Piso e teto texturizados: uma LRMM por linha de tela (mapeamento afim por linha = perspectiva correta de plano). Tile de 16x16 texels
 * por 64 unidades; a regiao replicada (256x64) faz o wrap em X (modulo 256) e deixa a faixa de V caber sem cruzar o limite. So ate zr<=120 unidades. */
/* Teto: preto solido. O Geo3D nao tem profundidade por pixel, entao paredes/objetos mais altos que o teto atras de uma abertura apareceriam por cima dele.
 * Depois do Geo3D, pintamos de preto tudo ACIMA da linha do teto: um leque de 8 raios (passos de 32 de profundidade) da, por faixa de colunas, o setor
 * em cada trecho; a linha de corte e a mais baixa entre as projecoes do teto de cada trecho no seu fim (z = 160*H/(linha)). Entre faixas, interpola. */
static u8 ycut[8];
static void ceil_probe(s16 s,s16 c){
  u8 g,i,sc,last,yc; s16 x,y,d,dx,dy,t,H; u16 r;
  for(g=0;g<8;g++){
    t=16+32*g-128; dx=mulq14(s,32)+(mulq14(c,32)*t)/160; dy=mulq14(c,32)+(mulq14(-s,32)*t)/160;
    x=px; y=py; d=0; last=255; yc=0;
    for(i=0;i<12;i++){
      sc=rast(x,y); if(sc==255)break;
      if(sc!=last){ if(last!=255){ H=secs[last].ceil-eye; if(H>4){ r=(160u*H)/d; if(r<89){ r=89-r; if(r>yc)yc=(u8)r; } } } last=sc; }
      x+=dx; y+=dy; d+=32;
    }
    if(last!=255){ H=secs[last].ceil-eye; if(H>4){ r=(160u*H)/(i<12?d:1500); if(r<89){ r=89-r; if(r>yc)yc=(u8)r; } } }
    ycut[g]=yc;
  }
}
static void ceil_fill(void){
  u8 j,g,f,a,b; s16 y;
  for(j=0;j<16;j++){ s16 x=8+16*j;
    if(x<=16)y=ycut[0]; else if(x>=240)y=ycut[7];
    else{ g=(u8)((x-16)>>5); f=(u8)((x-16)&31); a=ycut[g]; b=ycut[g+1];
      y=((a>b?a-b:b-a)>24)?(a<b?a:b):(s16)a+(((s16)b-(s16)a)*(s16)f)/32; }
    if(y>0)fill(j*16,0,16,y,C_BLACK); }
}
static void render(void){
  u8 i,k=0,n,cn=0,slot; u16 bi; s16 bx,by; u8 cand[MAX_SPR]; s16 cd[MAX_SPR]; u8 *v,*f,*t; s16 j;
  const Sec *sc; u8 s0=rast(px,py);
  /* teto: preto liso (um plano de teto por quadro vazava para os setores vizinhos). Piso: cores do setor 96 unidades a frente (nao pula ao trocar de setor);
   * com TAB, piso texturizado por setor (leque de raios). */
  { s16 sn=isin(yaw),cs=icos(yaw); u8 sa=rast(px+mulq14(sn,96),py+mulq14(cs,96)); if(sa==255)sa=s0;
  if(s0!=255){ sc=&secs[sa]; fill(0,0,SCR_W,89,C_BLACK); fill(0,89,SCR_W,45,sc->f0); fill(0,134,SCR_W,VIEW_H-134,sc->f1); if(!(fcnt&1))ceil_probe(sn,cs); } }
  if(s0==255){ fill(0,0,SCR_W,89,C_PANEL); fill(0,89,SCR_W,89,C_BLACK); }
  wait_ce();
  make_view();
  bx=(px>>7)-BX0; by=(py>>7)-BY0;
  if(bx>=0&&by>=0&&bx<BNX&&by<BNY){ bi=grid[by*BNX+bx];
    if(bi!=0xFFFF){ u8 h=cur_h,d=(u8)(yaw-(u8)(cur_h<<6));       /* 4 conjuntos por bloco (um por direcao de 90 graus), com histerese de ~5 graus */
      if(d>=128)d=(u8)(256-d);
      if(cur_blk==0xFFFF||d>36)h=(u8)(((u8)(yaw+32)>>6)&3);
      bi=(bi<<2)|h; if(bi!=cur_blk){ blk_load(bi); cur_h=h; } } }
  /* sprites: os MAX_SPR mais proximos dentro do campo de visao e com linha de visao */
  for(i=0;i<NTHING;i++){
    s16 dx,dy,zc,xc,d; Th *m=&th[i];
    if(!m->on)continue;
    dx=m->x-px; dy=m->y-py; d=s16abs(dx)+s16abs(dy); if(d>1500)continue;
    zc=mulq14(M[6],dx)+mulq14(M[8],dy); if(zc<12)continue;
    xc=mulq14(M[0],dx)+mulq14(M[2],dy); if(s16abs(xc)>zc+zc/2+40)continue;
    if(((fcnt+i)&3)==0) m->vis=los(px,py,m->x,m->y);
    if(!m->vis)continue;
    if(cn<MAX_SPR){ cand[cn]=i; cd[cn]=d; cn++; }
    else { u8 w=0; for(j=1;j<MAX_SPR;j++) if(cd[j]>cd[w])w=j; if(d<cd[w]){ cand[w]=i; cd[w]=d; } }
  }
  for(n=0;n<cn;n++){
    Th *m=&th[cand[n]]; s16 hw,ht,hx,hy,fl; u8 u,w,b;
    slot=(m->kind<3)?mon_slot(m):kind_spr[m->kind];
    hw=(s16)spr_scale[slot]*3; ht=(s16)spr_scale[slot]*8;
    hx=mulq14(M[0],hw); hy=mulq14(M[2],hw);
    fl=floor_at(m->x,m->y); v=vbuf+24*k; f=fbuf+11*k; t=tbuf+8*k;
    put16(v,m->x-hx);put16(v+2,fl+ht);put16(v+4,m->y-hy);  put16(v+6,m->x+hx);put16(v+8,fl+ht);put16(v+10,m->y+hy);
    put16(v+12,m->x+hx);put16(v+14,fl);put16(v+16,m->y+hy); put16(v+18,m->x-hx);put16(v+20,fl);put16(v+22,m->y-hy);
    b=cell_nv+4*k; f[0]=b;f[1]=b+1;f[2]=b+2;f[3]=b+3; put16(f+4,SPR_NX);put16(f+6,SPR_NY);put16(f+8,SPR_NZ); f[10]=0x80;
    u=(slot%SPR_COLS)*SPR_CW; w=(slot/SPR_COLS)*SPR_CH+1;                 /* +1: ver TSTRIDE */
    t[0]=u;t[1]=w;t[2]=u+SPR_CW-1;t[3]=w;t[4]=u+SPR_CW-1;t[5]=w+SPR_CH-1;t[6]=u;t[7]=w+SPR_CH-1;
    k++;
  }
  geo_light(LC);
  gi=0x40; gd=cell_nv; gd=0; gd=cell_nv+4*k;
  gi=0x58; gd=cell_nf; gd=cell_nf+k;
  if(k){ gi=0x50; gtransfer(vbuf,(u16)24*k); gi=0x52; gtransfer(fbuf,(u16)11*k); gi=0x65; gd=cell_nf; gi=0x53; gtransfer(tbuf,(u16)8*k); }
  geo_run(M,TX,TY,TZ);
  ceil_fill();
  draw_gun();
  if(msg) text(4,4,FONT_ORANGE,msg);
  if(hurt) { fill(0,0,SCR_W,2,C_RED); fill(0,VIEW_H-2,SCR_W,2,C_RED); }
  if(over) overlay();
}

static void mark(u8 n){ u8 p0=page; for(page=0;page<2;page++) fill(228+n*6,205,4,4,(n>=2&&!geo_ok)?C_RED:C_WHITE); page=p0; }
static void no_geo(void){
  page=0; fill(0,0,SCR_W,HUD_Y,C_PANEL);
  text(46,70,FONT_ORANGE,"GEO3D NAO ENCONTRADO"); text(46,86,FONT_GRAY,"USE EXT GEO3D NO OPENMSX"); text(46,102,FONT_GRAY,"TIPO DE ROM ASCII16");
  vr(2,0x1F); vr(1,0x40); for(;;);
}

void main(void){
  u8 i; u16 n;
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
  load_meta(); new_game(); keys=old_keys=0;
  for(page=0;page<2;page++){ fill(0,0,SCR_W,212,0); hud_static(); }
  vr(1,0x40);
  mark(0);
  if(!geo_present()) no_geo();
  mark(1); geo_init();
  page=0; sh_hp=-1; sh_ar=-1; sh_am=-1; sh_face=255; hud_update();
  { u8 first=1;
  for(;;){
    read_keys();
    if(over){ if((keys&K_FIRE)&&!(old_keys&K_FIRE)){ new_game(); sh_hp=-1; sh_ar=-1; sh_am=-1; sh_face=255; } } else update();
    page^=1; render(); hud_update(); wait_ce();
    if(first){ mark(2); }
    wait_vblank(); vr(2,0x1F|((u16)page<<5));
    if(first){ mark(3); first=0; }
    sound_tick();
  } }
}
