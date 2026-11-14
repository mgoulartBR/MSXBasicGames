#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "msxgl.h"
u8 world_map[9835],world_collision[1168],active_level;
volatile u8 enemy_mode,enemy_probe[4],hero_frame,hero_visible,death_ticks,throw_ticks,combat_dirty;
volatile u16 shots_fired,flower_hits,flower_returns,defeats;
u16 MapTile(u16 i){u16 n=i+(i>>3);return (((u16)world_map[n]|((u16)world_map[n+1]<<8))>>(i&7))&511;}
void SoundPlay(u8 s){(void)s;}
void KeyDoorTick(void){}
void KeyDoorReset(void){}
#include "game_t.c"
static unsigned rng=12345;static unsigned R(void){rng=rng*1103515245u+12345u;return (rng>>16)&0x7fff;}
typedef struct{u16 x,y;i16 vx,vy;u8 g,f,ft,cl,pc,emb;}St;
static u8 embedded(void){int top=(player_y+48)>>8,bottom=((player_y+496)>>8);if(floor_type>1)bottom--;int mxl=(player_x+64)>>8,mxr=(player_x+304)>>8;for(int y=top;y<=bottom;++y){if(Wall(mxr,y,3)&&Wall(mxr,y,1))return 1;if(Wall(mxl,y,1)&&Wall(mxl,y,3))return 1;}return 0;}
static St snap(void){St s={player_x,player_y,player_vx,player_vy,grounded,player_frame,floor_type,climbing,pole_cooldown,embedded()};return s;}
int main(int argc,char**argv)
{
 FILE*f=fopen(argv[1],"rb");int n=fread(world_map,1,sizeof(world_map),f);(void)n;fclose(f);
 f=fopen(argv[2],"rb");assert(fread(world_collision,1,1168,f)==1168);fclose(f);
 active_level=atoi(argv[3]);int segs=atoi(argv[4]);rng=atoi(argv[5]);
 static St A[200000],B[200000];int embA=0,embB=0,total=0,exact=0,frame_mis=0,gr_mis=0;int maxdx=0,maxdy=0;long sumdx=0;int diverged=0,reset_when_diverged=0,shown=0;
 for(int sg=0;sg<segs;++sg){
  unsigned sr=rng;
  // gera roteiro: segmentos de 2*k ticks com entrada fixa
  u8 script[400];int len=0;
  for(int k=0;k<120;++k){u8 in=0;int d=R()%4;if(d==1)in|=1;if(d==2)in|=2;if(d==3)in|=2;if(R()%5==0)in|=16;if(R()%25==0)in|=4;if(R()%40==0)in|=8;int dur=2+2*(R()%12);for(int t=0;t<dur/2&&len<400;++t)script[len++]=in;}
  Game_Reset();player_x=player_x;int m=len;
  for(int i=0;i<m;++i){Game_Tick(script[i]);Game_Tick(script[i]);A[i]=snap();}
  Game_Reset();
  for(int i=0;i<m;++i){Game_Tick2(script[i]);B[i]=snap();}
  for(int i=0;i<m;++i){
   embA+=A[i].emb;embB+=B[i].emb;++total;int ddx=abs((int)A[i].x-(int)B[i].x),ddy=abs((int)A[i].y-(int)B[i].y);
   if(!ddx&&!ddy&&A[i].g==B[i].g&&A[i].f==B[i].f)++exact;
   if(A[i].f!=B[i].f)++frame_mis;if(A[i].g!=B[i].g)++gr_mis;
   if(ddx>maxdx)maxdx=ddx;if(ddy>maxdy)maxdy=ddy;sumdx+=ddx;
   if(getenv("FIRST")&&(ddx>48||ddy>32)&&shown<4){shown++;printf("--- first big deviation, script step %d\n",i);for(int j=(i>6?i-6:0);j<=i;++j)printf("  i=%d in=%u A: x%u y%u vx%d vy%d g%u f%u c%u | B: x%u y%u vx%d vy%d g%u f%u c%u\n",j,script[j],A[j].x,A[j].y,A[j].vx,A[j].vy,A[j].g,A[j].f,A[j].cl,B[j].x,B[j].y,B[j].vx,B[j].vy,B[j].g,B[j].f,B[j].cl);}
   if(ddx>400||ddy>400){diverged++;if(getenv("DBG")&&diverged==1){for(int j=(i>8?i-8:0);j<=i;++j)printf("  i=%d in=%u A: x%u y%u vx%d vy%d g%u f%u c%u pc%u | B: x%u y%u vx%d vy%d g%u f%u c%u pc%u\n",j,script[j],A[j].x,A[j].y,A[j].vx,A[j].vy,A[j].g,A[j].f,A[j].cl,A[j].pc,B[j].x,B[j].y,B[j].vx,B[j].vy,B[j].g,B[j].f,B[j].cl,B[j].pc);}break;}   // > 25 px: trajetórias separadas, não compara mais
  }
 }
 printf("level %d: steps compared %d, exact %d (%.1f%%), grounded mismatches %d, frame mismatches %d, max |dx| %.2f px, max |dy| %.2f px, diverged scripts %d/%d, embedded states A=%d B=%d\n",active_level,total,exact,100.0*exact/total,gr_mis,frame_mis,maxdx/16.0,maxdy/16.0,diverged,segs,embA,embB);
 return 0;
}
