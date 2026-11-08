/* Prototype: V9990 P1 tile scroll + 13 sprite actors (4 of 32x32, 9 of 48x48) + one pose upload per frame. */
#include "msxgl.h"
#include "v9990.h"
#define SGT 0x38000UL
#define NACT 13
u16 frames;u8 proto_mode,cmd_res;u16 scx,scy;
u8 pose32[2][512],pose48[2][1152];
u8 sprbuf[100*4];
i16 ax[NACT],ay[NACT];i8 vx[NACT],vy[NACT];
u8 cols[16][3]={{0,0,0},{20,40,20},{200,40,40},{40,200,40},{40,40,200},{200,200,40},{40,200,200},{200,40,200},{120,120,120},{255,128,0},{90,60,30},{60,90,30},{30,60,90},{220,220,220},{160,80,80},{255,255,255}};
static u8 pcolor(u8 x,u8 y,u8 w,u8 fill) {
 if(x<4&&y<4)return 0;
 if(x==0||y==0||x==w-1||y==w-1)return 15;
 if(x==y)return 9;
 if(x==w-1-y)return 13;
 return fill;
}
static void make_pose(u8* buf,u8 w,u8 fill) {
 u8 x,y;
 for(y=0;y<w;++y)for(x=0;x<w;x+=2)*buf++=(pcolor(x,y,w,fill)<<4)|pcolor(x+1,y,w,fill);
}
static void upload_pose(const u8* img,u8 w,u8 first) {
 u8 n=w>>4,sx,sy,k;
 for(sy=0;sy<n;++sy)for(sx=0;sx<n;++sx) {
  u8 p=first+sy*n+sx;u32 base=SGT+((u16)(p>>4)<<11)+((u16)(p&15)<<3);
  const u8* src=img+(u16)sy*16*(w>>1)+sx*8;
  for(k=0;k<16;++k) {V9_SetWriteAddress(base+((u16)k<<7));V9_WriteVRAM_CurrentAddr(src,8);src+=w>>1;}
 }
}
static const u8* up_src;static u8 up_pitch,up_dsthi,up_cnt;static u16 up_dst;
static void up_spr(void) __naked {
 __asm
 ld hl,(_up_src)
 ld de,(_up_dst)
 ld a,#16
 ld (_up_cnt),a
up_line:
 xor a,a
 out (#0x64),a
 ld a,e
 out (#0x63),a
 ld a,d
 out (#0x63),a
 ld a,(_up_dsthi)
 out (#0x63),a
 ld c,#0x60
 outi
 outi
 outi
 outi
 outi
 outi
 outi
 outi
 ld a,(_up_pitch)
 sub a,#8
 ld c,a
 ld b,#0
 add hl,bc
 ld a,e
 add a,#128
 ld e,a
 jr nc,up_nc
 inc d
up_nc:
 ld a,(_up_cnt)
 dec a
 ld (_up_cnt),a
 jr nz,up_line
 ret
 __endasm;
}
static void upload_pose_asm(const u8* img,u8 w,u8 first) {
 u8 n=w>>4,sx,sy;
 for(sy=0;sy<n;++sy)for(sx=0;sx<n;++sx) {
  u8 p=first+sy*n+sx;u32 base=SGT+((u16)(p>>4)<<11)+((u16)(p&15)<<3);
  up_src=img+(u16)sy*16*(w>>1)+sx*8;up_pitch=w>>1;up_dst=(u16)base;up_dsthi=(u8)(base>>16);
  up_spr();
 }
}
static void wait_cmd(void) {while(V9_IsCmdRunning());}
static void pose_copy(u16 w,u16 rows,u16 sy,u16 dy) {
 /* sheet lines alternate between the two halves of a 256-byte-pitch bitmap row */
 wait_cmd();V9_CommandLMMM(0,sy,0,dy,w,rows,0);
 wait_cmd();V9_CommandLMMM(256,sy,256,dy,w,rows,0);
}
static void cmd_test(void) {
 u8 a[8],b[8],k,c,i,r=0;
 V9_SetCommandWriteMask(0xFFFF);V9_SetCommandLogicalOp(0x0C);
 if(proto_mode==11){pose_copy(64,8,896,896+64);}else{wait_cmd();V9_CommandLMMM(0,1792,0,1792+128,64,16,0);}
 wait_cmd();
 for(i=0;i<4;++i) {
  k=i==0?0:i==1?1:i==2?5:15;
  for(c=0;c<2;++c) {
   u8 cell=c?3:0;u16 off=(u16)k*128+cell*8;
   V9_ReadVRAM(SGT+off,a,8);V9_ReadVRAM(SGT+16384UL+off,b,8);
   if(a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2]&&a[3]==b[3]&&a[4]==b[4]&&a[5]==b[5]&&a[6]==b[6]&&a[7]==b[7]&&(a[0]|a[1]|a[2]|a[3]))r|=1<<(i*2+c);
  }
 }
 cmd_res=r;
}
static u8 first_pat(u8 a) {return a<4?a*4:16+(a-4)*9;}
static u8 first_spr(u8 a) {return first_pat(a);}
static void build_attr(void) {
 u8 a,sx,sy,n,*o=sprbuf;
 for(a=0;a<NACT;++a) {
  n=a<4?2:3;u8 p=first_pat(a);i16 x=ax[a],y=ay[a];
  for(sy=0;sy<n;++sy)for(sx=0;sx<n;++sx) {
   u16 X=x+sx*16;
   *o++=y+sy*16;*o++=p++;*o++=X&255;*o++=(X>>8)&3;
  }
 }
}
static void init_actors(u8 stress) {
 u8 a;
 for(a=0;a<NACT;++a) {
  if(stress) {ax[a]=a*18;ay[a]=40;vx[a]=vy[a]=0;}
  else {ax[a]=10+a*17;ay[a]=10+(a&3)*40;vx[a]=(a&1)?2:-2;vy[a]=(a&2)?1:-1;}
 }
}
void main(void) {
 u16 i;u8 a,pose;
 V9_SetPort(V9_P07,2);V9_SetPort(V9_P07,0);
 V9_SetScreenMode(V9_MODE_P1);
 V9_SetDisplayEnable(FALSE);
 V9_SetInterrupt(V9_INT_NONE);
 V9_SetBackgroundColor(1);
 for(i=0;i<16;++i){u8 c5[3];c5[0]=cols[i][0]>>3;c5[1]=cols[i][1]>>3;c5[2]=cols[i][2]>>3;V9_SetPaletteEntry(i,c5);}
 V9_SetSpritePatternAddr(V9_P1_SGT_38000);
 V9_SetSpriteEnable(TRUE);
 /* tile patterns 1..4 (8x8, bitmap-style rows, 128 byte pitch) */
 for(a=0;a<5;++a) {
  u8 k,b;
  for(k=0;k<8;++k) {
   V9_SetWriteAddress(((u32)k<<7)+(u32)a*4);
   for(b=0;b<2;++b)V9_Poke16_CurrentAddr(0);
  }
 }
 for(a=1;a<5;++a) {
  u8 k;u8 c=(a==1)?0x11:(a==2)?0x22:(a==3)?0x33:0x44;
  for(k=0;k<8;++k) {
   u8 row[4];u8 b;for(b=0;b<4;++b)row[b]=(k==0||k==7)?0xAA:(b==0?0xA0|(c&15):(b==3?(c&0xF0)|0x0A:c));
   V9_SetWriteAddress(((u32)k<<7)+(u32)a*4);V9_WriteVRAM_CurrentAddr(row,4);
  }
 }
 V9_SetWriteAddress(V9_P1_PNT_A);
 for(i=0;i<64*64;++i) {u8 x=i&63,y=i>>6;V9_Poke_CurrentAddr(1+((x^y)&3));V9_Poke_CurrentAddr(0);}
 make_pose(&pose32[0][0],32,2);make_pose(&pose32[1][0],32,3);
 make_pose(&pose48[0][0],48,4);make_pose(&pose48[1][0],48,5);
 for(a=0;a<NACT;++a)upload_pose(a<4?&pose32[0][0]:&pose48[0][0],a<4?32:48,first_pat(a));
 /* sprite attributes: disable the unused ones */
 for(i=0;i<125;++i)V9_Poke(V9_P1_SPAT+i*4+3,0x10);
 init_actors(0);proto_mode=0;
 V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);

 for(;;) {
  if(proto_mode==2){init_actors(1);proto_mode=3;}
  if(proto_mode==0 || proto_mode==4) {
   for(a=0;a<NACT;++a) {
    ax[a]+=vx[a];ay[a]+=vy[a];
    if(ax[a]<0||ax[a]>200)vx[a]=-vx[a];
    if(ay[a]<0||ay[a]>150)vy[a]=-vy[a];
   }
   scx+=2;scy+=1;V9_SetScrollingX(scx&511);V9_SetScrollingY(scy&511);
  }
  if(proto_mode==0 || proto_mode==4 || (proto_mode==3 && frames==0)) {
   build_attr();
   V9_WriteVRAM(V9_P1_SPAT,sprbuf,388);
  }
  if(proto_mode==0) {
   a=(u8)(frames%NACT);pose=(frames/NACT)&1;
   upload_pose(a<4?&pose32[pose][0]:&pose48[pose][0],a<4?32:48,first_pat(a));
  }
  if(proto_mode==5)V9_WriteVRAM(V9_P1_SPAT,sprbuf,388);
  if(proto_mode==6)build_attr();
  if(proto_mode==7){scx+=2;scy+=1;V9_SetScrollingX(scx&511);V9_SetScrollingY(scy&511);}
  if(proto_mode==8)upload_pose(&pose48[0][0],48,first_pat(4));
  if(proto_mode==9)upload_pose_asm(&pose48[0][0],48,first_pat(4));
  if(proto_mode==10)upload_pose_asm(&pose32[0][0],32,first_pat(0));
  if(proto_mode==11||proto_mode==15){cmd_test();proto_mode=12;}
  if(proto_mode==13){V9_SetCommandWriteMask(0xFFFF);V9_SetCommandLogicalOp(0x0C);wait_cmd();V9_CommandLMMM(0,1792,0,1792+128,48,48,0);wait_cmd();}
  if(proto_mode==14){V9_SetCommandWriteMask(0xFFFF);V9_SetCommandLogicalOp(0x0C);wait_cmd();V9_CommandLMMM(0,1792,0,1792+128,32,32,0);wait_cmd();}
  if(proto_mode==16){V9_SetCommandWriteMask(0xFFFF);V9_SetCommandLogicalOp(0x0C);wait_cmd();V9_CommandLMMM(0,1792,0,1792+128,48,48,0);}
  ++frames;
 }
}
