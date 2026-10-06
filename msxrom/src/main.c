/* MSX DOOM - esqueleto de ROM para MSX turbo R + V9968 + Geo3D (SDCC, ROM 16 KiB sem mapper).
 * STATUS: BUILD OK apenas. Nunca executado em emulador/FPGA/hardware (UNTESTED).
 * Registradores Geo3D conforme geo3d_engine.v (alexmoncks/V9968_Cartridge); SCREEN 5 como nas demos. */
#include "level.h"
typedef unsigned char u8; typedef unsigned int u16; typedef int s16; typedef long s32;

__sfr __at(0x98) vdp_d;
__sfr __at(0x99) vc;      /* porta de comando/status do VDP */
__sfr __at(0x9A) vp;      /* paleta */
__sfr __at(0x9B) vi;      /* registrador indireto */
__sfr __at(0x9C) vp4;     /* Port#4: desbloqueio dos recursos estendidos V9968 */
__sfr __at(0x9D) gi;      /* Geo3D: índice (escrita) / status (leitura) */
__sfr __at(0x9F) gd;      /* Geo3D: dados */
__sfr __at(0xA0) psg_a;
__sfr __at(0xA1) psg_d;
__sfr __at(0xAA) ppi_c;
__sfr __at(0xA9) ppi_b;

#define SCR_W 256
#define SCR_H 212
#define CAM_Y 32
#define ENEMY_HP 3

static void vr(u8 r,u8 v){ vc=v; vc=r|0x80; }
static void wait_ce(void){ while(vc&1); }              /* S#2 bit0 = CE (R#15 = 2) */
static void wait_vblank(void){ vr(15,0); (void)vc; while(!(vc&0x80)); vr(15,2); }

static void vw(s16 v){ vi=(u8)v; vi=(u8)((u16)v>>8); }
static void gw(s16 v){ gd=(u8)v; gd=(u8)((u16)v>>8); }

/* LMMV: retângulo preenchido, colour 0..15 (SCREEN 5), na página "page" */
static u8 page;
static void fill(s16 x,s16 y,s16 w,s16 h,u8 c){
  if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
  if(x+w>SCR_W)w=SCR_W-x; if(y+h>SCR_H)h=SCR_H-y;
  if(w<=0||h<=0)return;
  wait_ce(); vr(17,36);
  vw(x); vw(y+((u16)page<<8)); vw(w); vw(h); vi=c; vi=0; vi=0x80;
}

/* stream de n bytes para a porta 0x9F (OTIR) */
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

/* ---- Geo3D ---- */
static void geo_init(void){
  gi=0x18; gw(170); gw(128); gw(106); gw(4); gw(256); gw(212);   /* F, CX, CY, ZNEAR, W, H */
  gi=0x5A; gw(-5000); gw(10500); gw(-11500);                     /* luz (espaço da câmera) */
}
static void geo_model(const u8 *v,const u8 *f,u8 nv,u8 nf){
  gi=0x40; gd=0;gd=0;gd=nv;gd=0;gd=0;gd=0;                       /* VADDR,EADDR,NVERT,NEDGE,COLOR,LOP */
  gi=0x50; gtransfer(v,(u16)nv*6);
  gi=0x58; gd=0; gd=nf;                                          /* FADDR, NFACE */
  gi=0x52; gtransfer(f,(u16)nf*11);
}
/* M (Q2.14, linha a linha) e T; RUN preenchido; espera o fim */
static void geo_run(const s16 *m,s16 tx,s16 ty,s16 tz){
  u8 i; gi=0; for(i=0;i<9;i++)gw(m[i]); gw(tx); gw(ty); gw(tz);
  gi=0x46; gw((u16)page<<8);
  gi=0x48; gd=3;                                                 /* RUN | faces preenchidas */
  while(gi&1); wait_ce();
}

/* (a*b)>>14 por soma e deslocamento. Evita (s32)a*b do SDCC 4.2.0, que gerou resultado errado
 * com multiplicador negativo (ver README, "Achados"). |a|,|b| < 32768. */
s16 mulq14(s16 a,s16 b){
  u8 neg=0,i; u16 ub; long acc=0,t;
  if(a<0){a=-a;neg^=1;} if(b<0){b=-b;neg^=1;}
  ub=(u16)b; t=(u16)a;
  for(i=0;i<16;i++){ if(ub&1)acc+=t; t<<=1; ub>>=1; }
  acc>>=14; return neg?-(s16)acc:(s16)acc;
}

/* ---- estado do jogo ---- */
s16 px,pz; u8 yaw; s16 hp; u8 cool,flash,hurt,over;   /* globais: lidas pelos testes */
typedef struct{s16 x,z; s16 hp;}Enemy;
Enemy en[NENEMY]; u8 alive_count; u8 sfx;
static u8 keys,old_keys;
#define K_LEFT 1
#define K_RIGHT 2
#define K_UP 4
#define K_DOWN 8
#define K_FIRE 16

static void read_keys(void){
  u8 a; ppi_c=(ppi_c&0xF0)|8; a=~ppi_b; old_keys=keys; keys=0;   /* linha 8: SPACE,←,↑,↓,→ */
  if(a&0x10)keys|=K_LEFT; if(a&0x80)keys|=K_RIGHT; if(a&0x20)keys|=K_UP; if(a&0x40)keys|=K_DOWN; if(a&1)keys|=K_FIRE;
}
static u8 solid_at(s16 x,s16 z){ s16 cx=x>>6,cz=z>>6; if(x<0||z<0||cx>=MAPW||cz>=MAPH)return 1; return map_solid[cz*MAPW+cx]; }
static void try_move(s16 *x,s16 *z,s16 dx,s16 dz,s16 r){
  if(!solid_at(*x+dx+(dx>0?r:-r),*z))*x+=dx;
  if(!solid_at(*x,*z+dz+(dz>0?r:-r)))*z+=dz;
}
static s16 isin(u8 a){ return sin_tab[a]; }
static s16 icos(u8 a){ return sin_tab[(u8)(a+64)]; }

static void psg(u8 r,u8 v){ psg_a=r; psg_d=v; }
static void sound_tick(void){
  if(sfx){ psg(6,15); psg(7,0xB7); psg(8,sfx>15?15:sfx); sfx-=2; if(sfx>200)sfx=0; } else psg(8,0);
}

static void new_game(void){
  u8 i; px=START_X; pz=START_Z; yaw=32; hp=100; over=0; cool=flash=hurt=0; alive_count=NENEMY;
  for(i=0;i<NENEMY;i++){ en[i].x=enemy_start[i][0]; en[i].z=enemy_start[i][1]; en[i].hp=ENEMY_HP; }
}

static u8 los(s16 x0,s16 z0,s16 x1,s16 z1){
  u8 i; for(i=1;i<12;i++) if(solid_at(x0+(x1-x0)*i/12,z0+(z1-z0)*i/12))return 0;   /* |delta*i|<=11264: cabe em s16 */
  return 1;
}
static void shoot(void){
  u8 i,best=255; s16 bz=32000; s16 s=isin(yaw),c=icos(yaw);
  cool=10; flash=3; sfx=30;
  for(i=0;i<NENEMY;i++){ if(en[i].hp<=0)continue;
    s16 dx=en[i].x-px,dz=en[i].z-pz;
    s16 xc=mulq14(dx,c)-mulq14(dz,s), zc=mulq14(dx,s)+mulq14(dz,c);
    if(zc>8&&xc>-22&&xc<22&&zc<bz&&los(px,pz,en[i].x,en[i].z)){bz=zc;best=i;}
  }
  if(best!=255 && --en[best].hp<=0){ alive_count--; sfx=60; }
}
static void update(void){
  u8 i; s16 s=isin(yaw),c=icos(yaw),mv=0;
  if(keys&K_LEFT)yaw-=3; if(keys&K_RIGHT)yaw+=3;
  if(keys&K_UP)mv=1; if(keys&K_DOWN)mv=-1;
  if(mv) try_move(&px,&pz,mulq14(s,5*mv),mulq14(c,5*mv),8);
  if(cool)cool--; if(flash)flash--; if(hurt)hurt--;
  if((keys&K_FIRE)&&!(old_keys&K_FIRE)&&!cool)shoot();
  for(i=0;i<NENEMY;i++){ if(en[i].hp<=0)continue;
    s16 dx=px-en[i].x,dz=pz-en[i].z,ad=(dx<0?-dx:dx)+(dz<0?-dz:dz);
    if(ad>60) try_move(&en[i].x,&en[i].z,dx>0?1:-1,dz>0?1:-1,12);
    else if(!(old_keys&0x80)&&(u8)(i+yaw+px)%16==0){ hp-=2; hurt=4; sfx=20; }   /* ataque corpo a corpo */
  }
  if(hp<=0||alive_count==0)over=1;
}

/* ---- render ---- */
static s16 M[9]; static s16 TX,TY,TZ;
static void make_view(void){
  s16 s=isin(yaw),c=icos(yaw);
  M[0]=c; M[1]=0; M[2]=-s; M[3]=0; M[4]=16384; M[5]=0; M[6]=s; M[7]=0; M[8]=c;
  TX=-(mulq14(c,px)-mulq14(s,pz)); TY=-CAM_Y; TZ=-(mulq14(s,px)+mulq14(c,pz));
}
static s16 dist2(s16 x,s16 z){ s16 dx=x-px,dz=z-pz; return (dx<0?-dx:dx)+(dz<0?-dz:dz); }  /* distância Manhattan, só para ordenar */

static void draw_gun(void){
  s16 k=flash?4:0;
  if(flash){ fill(116,118,24,14,14); fill(122,112,12,10,7); }
  fill(120,132+k,16,40,15); fill(122,132+k,3,40,14);              /* cano */
  fill(112,150+k,32,22,3); fill(114,150+k,28,3,15);               /* corrediça */
  fill(104,172+k,48,40,2); fill(108,176+k,40,36,3);               /* empunhadura/mão */
}
static void draw_hud(void){
  u8 i; fill(0,198,SCR_W,14,0);
  fill(4,201,102,8,2); fill(5,202,(hp>0?hp:0),6,hurt?14:7);        /* barra de vida */
  for(i=0;i<NENEMY;i++) fill(130+i*10,201,8,8,en[i].hp>0?12:2);   /* inimigos restantes */
  fill(124,100,9,1,14); fill(127,97,3,7,14);                       /* mira central */
}
static void render(void){
  u8 i,j,order[4]; s16 mx[9]; u8 ord[NENEMY]; s16 t;
  fill(0,0,SCR_W,106,0); fill(0,106,SCR_W,106,15);                 /* teto e chão */
  make_view();
  for(i=0;i<NCHUNK;i++)order[i]=i;                                 /* chunks do mais distante ao mais próximo */
  for(i=1;i<NCHUNK;i++){ u8 v=order[i]; j=i; while(j>0&&dist2(chunks[order[j-1]].cx,chunks[order[j-1]].cz)<dist2(chunks[v].cx,chunks[v].cz)){order[j]=order[j-1];j--;} order[j]=v; }
  for(i=0;i<NCHUNK;i++){ const Chunk *ch=&chunks[order[i]]; geo_model(ch->v,ch->f,ch->nv,ch->nf); geo_run(M,TX,TY,TZ); }
  for(i=0;i<NENEMY;i++)ord[i]=i;
  for(i=1;i<NENEMY;i++){ u8 v=ord[i]; j=i; while(j>0&&dist2(en[ord[j-1]].x,en[ord[j-1]].z)<dist2(en[v].x,en[v].z)){ord[j]=ord[j-1];j--;} ord[j]=v; }
  geo_model(enemy_v,enemy_f,ENEMY_NV,ENEMY_NF);
  for(i=0;i<NENEMY;i++){ const Enemy *e=&en[ord[i]]; if(e->hp<=0||!los(px,pz,e->x,e->z))continue;   /* sem Z-buffer entre RUNs: oculta inimigo atrás de parede */
    s16 zc=mulq14(M[6],e->x)+mulq14(M[8],e->z)+TZ; if(zc<12)continue;
    for(j=0;j<9;j++)mx[j]=M[j];
    t=TX+mulq14(M[0],e->x)+mulq14(M[2],e->z);
    geo_run(mx,t,TY,zc);
  }
  draw_gun(); draw_hud();
  if(over){ fill(0,60,SCR_W,60,hp>0?14:3); }                       /* vitória (laranja) / derrota (vermelho) */
}

void main(void){
  u8 i;
  __asm
    ld a,#5
    call 0x005F        ; BIOS CHGMOD: SCREEN 5
    di
  __endasm;
  vp4=0; vr(20,1); vr(21,0);                                       /* desbloqueio V9968 (doc: R#21=0,R#20=1) */
  vr(9,0x80); vr(8,0x0A); vr(2,0x1F);                              /* 212 linhas, sprites off, página 0 */
  vr(16,0); for(i=0;i<32;i++)vp=palette[i];
  geo_init(); vr(15,2);
  new_game(); keys=old_keys=0;
  for(;;){
    read_keys();
    if(over){ if((keys&K_FIRE)&&!(old_keys&K_FIRE))new_game(); } else update();
    page^=1; render(); wait_ce();
    wait_vblank(); vr(2,0x1F|((u16)page<<5));                     /* exibe a página recém-desenhada */
    sound_tick();
  }
}
