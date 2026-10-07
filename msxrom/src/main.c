/* MSX DOOM — ROM ASCII16 para MSX turbo R + V9968 + Geo3D (SDCC). SCREEN 8 com paleta EPAL de 256 cores,
 * paredes texturizadas pelo Geo3D (LRMM), inimigos como billboards texturizados, HUD por blits de fonte.
 * STATUS: BUILD OK + SIMULATED apenas. Nunca executado em emulador MSX/FPGA/hardware (UNTESTED).
 * Registradores: geo3d_engine.v e vdp_cpu_interface.v (hra1129/alexmoncks V9968_Cartridge); sequência de
 * inicialização conforme NEON_REVENANT / V9968_Geo3D_SampleDemo. */
#include "assets.h"
typedef unsigned char u8; typedef unsigned int u16; typedef int s16; typedef long s32;

__sfr __at(0x98) vdp_d;   /* dados de VRAM */
__sfr __at(0x99) vc;      /* comando/status */
__sfr __at(0x9A) vp;      /* paleta */
__sfr __at(0x9B) vi;      /* registrador indireto */
__sfr __at(0x9C) vp4;     /* Port#4: bit7 = trava dos registradores estendidos */
__sfr __at(0x9D) gi;      /* Geo3D: índice / status */
__sfr __at(0x9F) gd;      /* Geo3D: dados */
__sfr __at(0xA0) psg_a;
__sfr __at(0xA1) psg_d;
__sfr __at(0xAA) ppi_c;
__sfr __at(0xA9) ppi_b;

#define BANK(n) (*(volatile u8*)0x6000=(n))      /* ASCII16: janela 0x4000-0x7FFF */
#define DATA ((const u8*)0x4000)
#define SCR_W 256
#define VIEW_H 178                                /* janela 3D; abaixo dela fica o HUD */
#define HUD_Y 178
#define CAM_Y 32
#define ENEMY_HP 3
#define PLAYER_R 14                               /* tiras de 16 unidades + ZNEAR=2: ver README */

static void vr(u8 r,u8 v){ vc=v; vc=r|0x80; }
static void wait_ce(void){ while(vc&1); }          /* S#2 bit0 = CE (R#15 = 2) */
static void wait_vblank(void){ vr(15,0); (void)vc; while(!(vc&0x80)); vr(15,2); }
static void vw(s16 v){ vi=(u8)v; vi=(u8)((u16)v>>8); }
static void gw(s16 v){ gd=(u8)v; gd=(u8)((u16)v>>8); }

static u8 page;
/* HMMV */
static void fill(s16 x,s16 y,s16 w,s16 h,u8 c){
  if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
  if(x+w>SCR_W)w=SCR_W-x; if(y+h>212)h=212-y;
  if(w<=0||h<=0)return;
  wait_ce(); vr(17,36);
  vw(x); vw(y+((u16)page<<8)); vw(w); vw(h); vi=c; vi=0; vi=0xC0;      /* HMMV: 1 byte/pixel em SCREEN 8, mais rápido que LMMV */
}
/* LMMM com TIMP (índice 0 = transparente); sy é absoluto (atlas na VRAM), dy relativo à página */
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
static void vtransfer16k(const u8 *p) __naked __sdcccall(1) {   /* 16384 bytes -> porta 0x98 */
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

/* ---------------------------------------------------------------- Geo3D */
#define TEXY_WALL 512
#define TEXY_SPR 576
static void geo_tex(u16 texy,u8 stride){ gi=0x60; gw(0); gw(texy); gd=stride; }
static void geo_init(void){
  gi=0x18; gw(170); gw(128); gw(VIEW_H/2); gw(2); gw(256); gw(VIEW_H);   /* F, CX, CY, ZNEAR, W, H */
  geo_tex(TEXY_WALL,64);
}
static void geo_light(const s16 *l){ gi=0x5A; gw(l[0]); gw(l[1]); gw(l[2]); }
/* carrega um modelo texturizado: vértices, faces (11 B) e coordenadas de textura (8 B/face) */
static void geo_model(const u8 *v,u8 nv,const u8 *f,u8 nf,const u8 *t){
  gi=0x40; gd=0;gd=0;gd=nv;gd=0;gd=0;gd=8;                      /* VADDR,EADDR,NVERT,NEDGE,COLOR,LOP=TIMP */
  gi=0x50; gtransfer(v,(u16)nv*6);
  gi=0x58; gd=0; gd=nf;                                          /* FADDR, NFACE */
  gi=0x52; gtransfer(f,(u16)nf*11);
  gi=0x65; gd=0;                                                 /* TADDR */
  gi=0x53; gtransfer(t,(u16)nf*8);
}
static void geo_run(const s16 *m,s16 tx,s16 ty,s16 tz){
  u8 i; gi=0; for(i=0;i<9;i++)gw(m[i]); gw(tx); gw(ty); gw(tz);
  gi=0x46; gw((u16)page<<8);
  gi=0x48; gd=7;                                                 /* RUN | faces preenchidas | texturas */
  while(gi&1); wait_ce();
}

s16 mulq14(s16 a,s16 b) __sdcccall(1);          /* src/math.s: (a*b)>>14 sem usar (s32)a*b do SDCC 4.2.0 */

/* ---------------------------------------------------------------- estado do jogo */
typedef struct{s16 x,z,hp; s16 t,atk,hurt,dying;}Enemy;
s16 px,pz; u8 yaw; s16 hp; u8 cool,flash,hurt,over;
Enemy en[NENEMY]; u8 alive_count; u8 kills; u8 sfx; u8 msg_id,msg_t;
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
static u8 solid_at(s16 x,s16 z){ s16 cx=x>>6,cz=z>>6; if(x<0||z<0||cx>=MAPW||cz>=MAPH)return 1; return map_solid[(cz<<4)+cx];      /* MAPW=16 */ }
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
  u8 i; px=START_X; pz=START_Z; yaw=32; hp=100; over=0; cool=flash=hurt=0; alive_count=NENEMY; kills=0; msg_id=0; msg_t=0;
  for(i=0;i<NENEMY;i++){ en[i].x=enemy_start[i][0]; en[i].z=enemy_start[i][1]; en[i].hp=ENEMY_HP; en[i].t=i*5; en[i].atk=en[i].hurt=en[i].dying=0; }
}
static u8 los(s16 x0,s16 z0,s16 x1,s16 z1){
  s16 dx=(x1-x0)>>4,dz=(z1-z0)>>4; u8 i;                     /* 16 amostras, sem divisão */
  for(i=1;i<16;i++){ x0+=dx; z0+=dz; if(solid_at(x0,z0))return 0; }
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
  if(best!=255){ en[best].hurt=4; if(--en[best].hp<=0){ alive_count--; kills++; en[best].dying=10; msg_id=1; msg_t=90; sfx=60; } }
}
static void update(void){
  u8 i; s16 s=isin(yaw),c=icos(yaw),mv=0;
  if(keys&K_LEFT)yaw-=3; if(keys&K_RIGHT)yaw+=3;
  if(keys&K_UP)mv=1; if(keys&K_DOWN)mv=-1;
  if(mv) try_move(&px,&pz,mulq14(s,5*mv),mulq14(c,5*mv),PLAYER_R);
  if(cool)cool--; if(flash)flash--; if(hurt)hurt--; if(msg_t&&!--msg_t)msg_id=0;
  if((keys&K_FIRE)&&!(old_keys&K_FIRE)&&!cool)shoot();
  for(i=0;i<NENEMY;i++){
    if(en[i].hp<=0){ if(en[i].dying)en[i].dying--; continue; }
    { s16 dx=px-en[i].x,dz=pz-en[i].z,ad=(dx<0?-dx:dx)+(dz<0?-dz:dz);
      if(ad>900)continue;                                    /* longe do jogador: parado (nao custa CPU) */
      en[i].t++; if(en[i].hurt)en[i].hurt--; if(en[i].atk)en[i].atk--;
      if(ad>60) try_move(&en[i].x,&en[i].z,dx>0?1:-1,dz>0?1:-1,12);
      else if(!en[i].atk&&(u8)(i+yaw+px)%16==0){ hp-=2; hurt=4; sfx=20; en[i].atk=12; }   /* ataque corpo a corpo */
    }
  }
  if(hp<=0||alive_count==0)over=1;
}

/* ---------------------------------------------------------------- HUD (blits de fonte; páginas 0 e 1) */
static const char fontchars[]=FONT_CHARS;
#define FONT_GRAY 896
#define FONT_ORANGE 904
#define FONT_RED 912
#define FONT_RED2 920
static u8 glyph(char ch){ u8 i; for(i=0;fontchars[i];i++) if(fontchars[i]==ch)return i; return 0; }
static void text(u16 x,u16 y,u16 row,const char *s){ for(;*s;s++,x+=6) blit((u16)glyph(*s)*6,row,x,y,5,7); }
static void bigtext(u16 x,u16 y,const char *s){ for(;*s;s++,x+=12) blit((u16)glyph(*s)*12,FONT_RED2,x,y,10,14); }
static char numbuf[8];
static const char *itoa3(s16 v){ u8 i=0; if(v<0)v=0; if(v>=100)numbuf[i++]='0'+v/100; if(v>=10)numbuf[i++]='0'+(v/10)%10; numbuf[i++]='0'+v%10; numbuf[i]=0; return numbuf; }
static const char *const msgs[2]={"MSX DOOM","ABATIDO!"};
static s16 sh_hp=-1; static u8 sh_kills=255,sh_alive=255,sh_msg=255;

static void hud_static(void){
  fill(0,HUD_Y,SCR_W,34,C_HUD_BG); fill(0,HUD_Y,SCR_W,1,C_HUD_BORDER); fill(82,HUD_Y,1,34,C_HUD_BORDER); fill(172,HUD_Y,1,34,C_HUD_BORDER);
  text(30,HUD_Y+3,FONT_GRAY,"VIDA"); text(92,HUD_Y+3,FONT_GRAY,"ABATES"); text(182,HUD_Y+3,FONT_GRAY,"INIMIGOS");
}
static void hud_dynamic(void){
  char t[8]; u8 i,n;
  if(hp!=sh_hp){ s16 v=hp<0?0:hp; const char *s=itoa3(v); n=0; while(s[n])n++;
    fill(22,191,52,14,C_HUD_BG); bigtext(22,191,s); bigtext(22+12*n,191,"%");
    fill(8,207,66,3,C_BAR_BG); fill(8,207,(u16)v*66/100,3,C_BAR_RED); }
  if(kills!=sh_kills){ const char *s=itoa3(kills); fill(134,HUD_Y+3,36,7,C_HUD_BG); for(i=0;s[i];i++)t[i]=s[i]; t[i++]='/'; t[i]=0; text(134,HUD_Y+3,FONT_GRAY,t); text(134+6*i,HUD_Y+3,FONT_GRAY,itoa3(NENEMY)); }
  if(msg_id!=sh_msg){ fill(92,195,72,7,C_HUD_BG); text(92,195,FONT_ORANGE,msgs[msg_id]); }
  if(alive_count!=sh_alive){ fill(205,191,40,14,C_HUD_BG); bigtext(214,191,itoa3(alive_count)); }
}
static void hud_update(void){
  u8 p0=page;
  if(hp==sh_hp&&kills==sh_kills&&msg_id==sh_msg&&alive_count==sh_alive)return;
  for(page=0;page<2;page++)hud_dynamic();
  page=p0; sh_hp=hp; sh_kills=kills; sh_msg=msg_id; sh_alive=alive_count;
}

/* ---------------------------------------------------------------- render */
static s16 M[9]; static s16 TX,TY,TZ; static s16 LC[3];
static const s16 LW[3]={-5734,11207,-10486};       /* luz no mundo: gera apenas os niveis 0,2,4 nas paredes */
static const s16 IDENT[9]={16384,0,0,0,16384,0,0,0,16384};
static void make_view(void){
  s16 s=isin(yaw),c=icos(yaw);
  M[0]=c; M[1]=0; M[2]=-s; M[3]=0; M[4]=16384; M[5]=0; M[6]=s; M[7]=0; M[8]=c;
  TX=-(mulq14(c,px)-mulq14(s,pz)); TY=-CAM_Y; TZ=-(mulq14(s,px)+mulq14(c,pz));
  LC[0]=mulq14(M[0],LW[0])+mulq14(M[2],LW[2]); LC[1]=mulq14(M[4],LW[1]); LC[2]=mulq14(M[6],LW[0])+mulq14(M[8],LW[2]);
}
static s16 dist1(s16 x,s16 z){ s16 dx=x-px,dz=z-pz; return (dx<0?-dx:dx)+(dz<0?-dz:dz); }   /* Manhattan: só para ordenar */
static s16 s16abs(s16 v){ return v<0?-v:v; }

static u8 enemy_sprite(u8 i){
  const Enemy *e=&en[i]; u8 base=(i&1)*6;
  if(e->hp<=0) return base+(e->dying?4:5);
  if(e->hurt) return base+3;
  if(e->atk) return base+2;
  return base+((e->t>>3)&1);
}
static void draw_enemy(u8 i,s16 xc,s16 zc){
  u8 k=enemy_sprite(i); u8 u=(k%10)*24,v=(k/10)*32; u8 tex[8];
  static const u8 vtx0[24]={ 0xEB,0xFF, 24,0, 0,0,  21,0, 24,0, 0,0,  21,0, 0xE0,0xFF, 0,0,  0xEB,0xFF, 0xE0,0xFF, 0,0 };   /* (-21,24),(21,24),(21,-32),(-21,-32) */
  tex[0]=u;tex[1]=v;tex[2]=u+23;tex[3]=v;tex[4]=u+23;tex[5]=v+31;tex[6]=u;tex[7]=v+31;
  { static const u8 face[11]={0,1,2,3, 0,0, 0,0, 0x00,0xC0, 0x80};   /* normal (0,0,-16384) = para a câmera */
    geo_model(vtx0,4,face,1,tex); }
  geo_run(IDENT,xc,0,zc);
}
static void draw_chunk(u8 i,u8 near){
  const Chunk *c=&chunks[i]; const u8 *p; u8 nv,nf;
  BANK(c->bank);
  if(near){ p=DATA+c->off_n; nv=c->nv_n; nf=c->nf_n; } else { p=DATA+c->off_f; nv=c->nv_f; nf=c->nf_f; }
  geo_model(p,nv,p+(u16)nv*6,nf,p+(u16)nv*6+(u16)nf*11);
  geo_run(M,TX,TY,TZ);
}
static void draw_gun(void){
  if(flash){ blit(64,832,96,114,64,64); blit(128,832,104,86,48,32); } else blit(0,832,96,114,64,64);
}
static void render(void){
  u8 i,j,k,n=0,order[NCHUNK],dk[NCHUNK],vis=0; u8 vi_[NENEMY]; s16 vzc[NENEMY],vxc[NENEMY];
  s16 dx0,dz0,zrow,xrow,stzi,stzj,stxi,stxj;
  for(i=0;i<6;i++) fill(0,i*15,SCR_W,i==5?(89-75):15,C_CEIL5-i);              /* teto: do topo (claro) ao horizonte (escuro) */
  for(i=0;i<8;i++) fill(0,89+i*11,SCR_W,i==7?(VIEW_H-166):11,C_FLOOR0+i);        /* chão: do horizonte (escuro) para perto (claro) */
  make_view(); geo_light(LC); geo_tex(TEXY_WALL,64);
  /* visibilidade dos 16 chunks (grade 4x4, centros em 256*i+128 / 256*j+128): 4 multiplicações + somas */
  dx0=128-px; dz0=128-pz;
  zrow=mulq14(M[6],dx0)+mulq14(M[8],dz0); xrow=mulq14(M[0],dx0)+mulq14(M[2],dz0);
  stzi=M[6]>>6; stzj=M[8]>>6; stxi=M[0]>>6; stxj=M[2]>>6;                      /* 256/16384 = 1/64 */
  for(j=0;j<4;j++){ s16 zc=zrow,xc=xrow;
    for(i=0;i<4;i++){ s16 v; k=(j<<2)|i; v=zc+chunks[k].rad;
      dk[k]=(u8)((s16abs((s16)(i<<8)+dx0)+s16abs((s16)(j<<8)+dz0))>>4);        /* distância/16 (cabe em u8) */
      if(v>=4 && s16abs(xc)-chunks[k].rad <= v-(v>>3)) order[n++]=k;          /* tan(meio campo) ~ 0.75 -> margem 0.875 */
      zc+=stzi; xc+=stxi; }
    zrow+=stzj; xrow+=stxj; }
  for(i=1;i<n;i++){ u8 v=order[i]; j=i; while(j>0&&dk[order[j-1]]<dk[v]){order[j]=order[j-1];j--;} order[j]=v; }   /* longe -> perto */
  for(i=0;i<n;i++) draw_chunk(order[i],dk[order[i]]<=20);                      /* 20*16 = 320 unidades: LOD perto/longe */
  for(i=0;i<NENEMY;i++){ const Enemy *e=&en[i]; s16 dx,dz,zc,xc;
    dx=e->x-px; dz=e->z-pz;
    if(s16abs(dx)+s16abs(dz)>1400)continue;
    zc=mulq14(M[6],dx)+mulq14(M[8],dz); if(zc<12||zc>1500)continue;
    xc=mulq14(M[0],dx)+mulq14(M[2],dz); if(s16abs(xc)>zc+40)continue;           /* fora do campo de visão */
    if(!los(px,pz,e->x,e->z))continue;                 /* sem Z-buffer entre RUNs: oculta inimigo atrás de parede */
    vi_[vis]=i; vzc[vis]=zc; vxc[vis]=xc; vis++;
  }
  for(i=1;i<vis;i++){ u8 v=vi_[i]; s16 z=vzc[i],x=vxc[i]; j=i; while(j>0&&vzc[j-1]<z){vi_[j]=vi_[j-1];vzc[j]=vzc[j-1];vxc[j]=vxc[j-1];j--;} vi_[j]=v;vzc[j]=z;vxc[j]=x; }   /* longe -> perto */
  if(vis){ geo_tex(TEXY_SPR,0); for(i=0;i<vis;i++) draw_enemy(vi_[i],vxc[i],vzc[i]); }
  draw_gun();
  fill(124,88,5,1,C_WHITE); fill(132,88,5,1,C_WHITE); fill(130,81,1,5,C_WHITE); fill(130,91,1,5,C_WHITE);   /* mira */
  if(over){ fill(0,60,SCR_W,40,hp>0?C_BAR_RED:C_BAR_BG); }
}

static void set_vram_write(u8 blk,u16 addr){ vr(14,blk); vc=(u8)addr; vc=((u8)(addr>>8)&0x3F)|0x40; }

void main(void){
  u8 i;
  vp4=0;                                              /* destrava os registradores estendidos do V9968 */
  vr(0,14); vr(1,0); vr(2,31); vr(7,0); vr(8,10); vr(9,128);      /* SCREEN 8, 212 linhas, tela apagada */
  vr(21,0); vr(20,0x11);                                          /* modo V9968; EPAL=1, HS=1 */
  vr(51,0);vr(52,0);vr(53,0);vr(54,0);vr(55,255);vr(56,0);vr(57,255);vr(58,3);   /* janela de origem LRMM: 0..255 x 0..1023 */
  vr(16,0);
  { u16 n; for(n=0;n<768;n++)vp=palette[n]; }                     /* 256 entradas x (R,G,B) de 5 bits */
  vr(15,2);
  set_vram_write(8,0);                                            /* linha 512: início da área de texturas */
  for(i=0;i<NVRAM_BANKS;i++){ BANK(FIRST_VRAM_BANK+i); vtransfer16k(DATA); }
  geo_init();
  new_game(); keys=old_keys=0;
  for(page=0;page<2;page++){ fill(0,0,SCR_W,212,0); hud_static(); }
  page=0; sh_hp=-1; sh_kills=255; sh_msg=255; sh_alive=255; hud_update();
  vr(1,0x40);                                                     /* liga a tela */
  for(;;){
    read_keys();
    if(over){ if((keys&K_FIRE)&&!(old_keys&K_FIRE))new_game(); } else update();
    page^=1; render(); hud_update(); wait_ce();
    wait_vblank(); vr(2,0x1F|((u16)page<<5));                     /* exibe a página recém-desenhada */
    sound_tick();
  }
}
