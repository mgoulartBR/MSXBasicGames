// SPDX-License-Identifier: GPL-2.0-or-later
#include "msxgl.h"
#include "pickups.h"
#include "world.h"
#include "keydoor.h"
#include "hw_tomato.h"
#include "campaign.h"
#include "hud_panel.h"
#include "hud_cache.h"
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);
extern volatile u16 camera_px,camera_py;
extern volatile u8 hardware_scroll;
u8 hud_keys[2];u16 hud_ammo[2];volatile u8 hud_debug;
static u8 dirty[2],cache_ready;
static u32 cached_score;
static u16 cached_ammo;
static i8 cached_lives;
static const u8 font[10][7]={{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
static const u8 small[10][7]={{7,5,5,5,5,5,7},{2,6,2,2,2,2,7},{7,1,1,7,4,4,7},{7,1,1,7,1,1,7},{5,5,5,7,1,1,1},{7,4,4,7,1,1,7},{7,4,4,7,5,5,7},{7,1,1,2,2,2,2},{7,5,5,7,5,5,7},{7,5,5,7,1,1,7}};
static u8 HudKey(void){return (active_level==4 || active_level==6 || active_level>=7)?(key_owned+1)|((boobus_bombs>9?9:boobus_bombs)<<2):(active_level==2 || active_level==5)?key_owned+1:0;}
void HudInit(void) __banked {cache_ready=0;dirty[0]=dirty[1]=7;cached_score=0xffffffffUL;cached_ammo=65535;cached_lives=-128;hud_debug=0;}
void HudDamage(u8 p) __banked {dirty[p]=7;}
u8 HudDirty(u8 p) __banked {return dirty[p] || cached_score!=game_score || cached_lives!=game_lives || cached_ammo!=(active_level==11?boobus_bombs:flower_ammo);}
static u8 Number(u8* digits,u32 value)
{
 static const u32 divisors[7]={1000000UL,100000UL,10000UL,1000UL,100UL,10UL,1UL};
 u8 i,n=0,d;
 for(i=0;i<7;++i){
  d=0;while(value>=divisors[i]){value-=divisors[i];++d;}
  if(d || n || i==6)digits[n++]=d;
 }
 return n;
}

// Z80 fast path for HudDraw's per-frame cache copies (see HudDraw). Parameters travel on the
// caller's stack (persistent RAM is full). Returns 1 if every chunk was issued, 0 if any chunk
// wraps the 256x256 ring (the generic loop in HudDraw then does the work).
typedef struct {const HudChunk* p;u8 n,hx,hy,mlo,mhi,page;} HudFastArgs;
static u8 HudFast(HudFastArgs* args) __sdcccall(1) __naked
{
 args;
 __asm
  push ix
  push iy
  push hl
  pop iy
  ld l,0 (iy)
  ld h,1 (iy)
  push hl
  pop ix
  ld b,2 (iy)
hf_loop:
  ld a,4 (ix)
  cp #16
  ld a,5 (iy)
  jr c,hf_chk
  ld a,6 (iy)
hf_chk:
  or a
  jp z,hf_next
  ld a,3 (iy)
  add a,3 (ix)
  ld e,a
  ld a,5 (ix)
  add a,e
  jr nc,hf_xok
  or a
  jp nz,hf_fail
hf_xok:
  ld a,4 (iy)
  add a,4 (ix)
  ld d,a
  add a,6 (ix)
  jr nc,hf_yok
  or a
  jp nz,hf_fail
hf_yok:
  call _VDP_CommandWait
  di
  ld a,#32
  out (0x99),a
  ld a,#0x91
  out (0x99),a
  ld a,0 (ix)
  out (0x9B),a
  xor a
  out (0x9B),a
  ld a,1 (ix)
  out (0x9B),a
  ld a,2 (ix)
  out (0x9B),a
  ld a,e
  out (0x9B),a
  xor a
  out (0x9B),a
  ld a,d
  out (0x9B),a
  ld a,7 (iy)
  out (0x9B),a
  ld a,5 (ix)
  out (0x9B),a
  xor a
  out (0x9B),a
  ld a,6 (ix)
  out (0x9B),a
  xor a
  out (0x9B),a
  out (0x9B),a
  out (0x9B),a
  ld a,e
  or 5 (ix)
  and #1
  ld a,#0xD0
  jr z,hf_cmd
  ld a,#0x90
hf_cmd:
  ei
  out (0x9B),a
hf_next:
  ld de,#7
  add ix,de
  dec b
  jp nz,hf_loop
  ld a,#1
  jr hf_done
hf_fail:
  xor a
hf_done:
  pop iy
  pop ix
  ret
 __endasm;
}
// Compose atlas pixels only on inventory changes. Per-frame redraws are VDP copies.
void HudDraw(u8 page) __banked
{
 u8 line[22],digits[7],length,start,pitch,left,right,top,lo,hi;
 const HudChunk* chunk;
 u8 count=hud_chunks_count[LEVEL_INDEX];
 u8 changed=0,k,row,i,col,f,bit,px,offset,x,y,sx,sy,w,h,dx,dy;
 u16 ammo=active_level==11?boobus_bombs:flower_ammo,base=(u16)page*256;
 if(!cache_ready || cached_score!=game_score){cached_score=game_score;changed|=1;dirty[0]|=1;dirty[1]|=1;}
 if(!cache_ready || cached_lives!=game_lives){cached_lives=game_lives;changed|=2;dirty[0]|=6;dirty[1]|=6;}
 if(!cache_ready || cached_ammo!=ammo){cached_ammo=ammo;changed|=4;dirty[0]|=6;dirty[1]|=6;}
 if(!cache_ready){
  VDP_CommandWait();
  for(k=0,chunk=hud_chunks[LEVEL_INDEX];k<count;++k,++chunk)
   for(row=0;row<chunk->h;++row)
    VPF(hud_panel+(u16)(chunk->dy+row)*36+chunk->dx/2,(chunk->sy+row-512)*128+chunk->sx/2,1,chunk->w/2);
  cache_ready=1;
 }
 if(changed){
  VDP_CommandWait();
  for(f=0;f<3;++f){
   if(!(changed&(1<<f)))continue;
   length=Number(digits,f==0?(game_score>9999999UL?9999999UL:game_score):f==1?(game_lives<0?0:game_lives):ammo);
   pitch=f==2 && ammo>999?4:6;
   start=f==0?68-length*6:f==1?32-length*6:69-length*pitch;
   left=f==0?26:f==1?20:48;right=f==0?68:f==1?32:70;top=f?20:4;
   for(row=0;row<7;++row){
    for(i=0;i<(right-left)/2;++i)line[i]=0x33;
    for(i=0;i<length;++i){
     bit=pitch==4?small[digits[i]][row]:font[digits[i]][row];
     for(col=0;col<pitch-1;++col)if(bit&(1<<(pitch-2-col))){
      px=start+i*pitch+col-left;line[px>>1]|=(px&1)?15:240;
     }
    }
    y=top+row;
    for(k=0,chunk=hud_chunks[LEVEL_INDEX];k<count;++k,++chunk){
     if(y<chunk->dy || y>=chunk->dy+chunk->h)continue;
     lo=left>chunk->dx?left:chunk->dx;hi=right<chunk->dx+chunk->w?right:chunk->dx+chunk->w;
     if(lo>=hi)continue;
     VPF(line+(lo-left)/2,(chunk->sy+y-chunk->dy-512)*128+(chunk->sx+lo-chunk->dx)/2,1,(hi-lo)/2);
    }
   }
  }
 }
 // Fast path (Z80): one prepared VDP command per chunk, no clipping. Falls back to the
 // generic loop below when any chunk wraps the 256x256 ring.
 {
  HudFastArgs fa;
  fa.p=hud_chunks[LEVEL_INDEX];fa.n=count;
  fa.hx=hardware_scroll?(u8)(camera_px+168):168;fa.hy=hardware_scroll?(u8)(camera_py+4):20;
  fa.mlo=dirty[page]&1;fa.mhi=dirty[page]&6;fa.page=page;
  if(HudFast(&fa)){dirty[page]=0;hud_keys[page]=HudKey();hud_ammo[page]=ammo;return;}
 }
 for(k=0,chunk=hud_chunks[LEVEL_INDEX];k<count;++k,++chunk){
  sy=chunk->dy;
  if(!(dirty[page]&(sy<16?1:6)))continue;
  dx=chunk->dx;
  x=hardware_scroll?(camera_px+168+dx)&255:168+dx;
  y=hardware_scroll?(camera_py+4+sy)&255:20+sy;
  for(sy=0;sy<chunk->h;sy+=h){
   h=chunk->h-sy;if((u16)y+h>256)h=256-y;
   dx=hardware_scroll?(camera_px+168+chunk->dx)&255:168+chunk->dx;
   for(sx=0;sx<chunk->w;sx+=w){
    w=chunk->w-sx;if((u16)dx+w>256)w=256-dx;
    g_VDP_Command.SX=chunk->sx+sx;
    g_VDP_Command.SY=chunk->sy+sy;
    g_VDP_Command.DX=dx;g_VDP_Command.DY=base+y;
    g_VDP_Command.NX=w;g_VDP_Command.NY=h;g_VDP_Command.ARG=0;
    g_VDP_Command.CMD=((dx|w|sx)&1)?VDP_CMD_LMMM+VDP_OP_IMP:VDP_CMD_HMMM;
    VDP_CommandSetupR32();dx=0;
   }
   y=0;
  }
 }
 dirty[page]=0;hud_keys[page]=HudKey();hud_ammo[page]=ammo;
}
