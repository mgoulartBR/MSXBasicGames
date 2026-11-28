#include "../shared.h"
#include "../mapdef.h"
#include "../text.h"
extern u8 music_cur;
/* Interface extras: minimap overlay (TAB; click = jump the camera, right click = send the selection there), control groups
 * (CTRL+1..5 saves, 1..5 recalls, twice = centre the camera), B = camera to the castle, P = sound on/off.
 * The minimap is written straight into the layer A bitmap (lines 109..158, right-hand corner of the 256x160 view): the base
 * picture (terrain + explored flag) is rebuilt one row at a time into mm_buf, and every 12 frames the rows are written to
 * VRAM with units, buildings in sight and the camera rectangle on top. 1 px = map_w/48 cells. */
__sfr __at(0x60) io_vram;
__sfr __at(0x63) io_reg;
__sfr __at(0x64) io_sel;
__sfr __at(0x65) io_stat;
#define MM_N 48
#define MM_X 208
#define MM_Y 110
u8 mm_cint,mm_bint;static u8 mm_on,mm_dn,mm_odn,mm_sub,mm_boost,mm_full,mm_ox0,mm_ox1,mm_oy0,mm_oy1;
static u16 mm_last;
static u8 mm_buf[MM_N*24],mm_col[MM_N],mm_ym[MM_N],mm_dx[24],mm_dy[24],mm_dc[24],mm_odx[24],mm_ody[24];
static u8 mm_wm[6],mm_om[6];         /* rows to rewrite (rebuilt) / rows that carried an overlay when last drawn */
static u8 mm_rowt[128],mm_rowf[128];
static u8 ui_prev,grp_mask[5],grp_last;static u16 grp_time;
static const u8 mm_tcol[10]={2,3,4,10,11,15,12,12,12,12};
/* Z80: read vr_n bytes of VRAM (address vr_a) into vr_dst with one INIR. */
static u8 vr_a0,vr_a1,vr_a2,vr_n;static u8* vr_dst;
static void vread_asm(void) __naked {
 __asm
 di
 ld a,#3
 out (#0x64),a
 ld a,(_vr_a0)
 out (#0x63),a
 ld a,(_vr_a1)
 out (#0x63),a
 ld a,(_vr_a2)
 out (#0x63),a
 ld hl,(_vr_dst)
 ld a,(_vr_n)
 ld b,a
 ld c,#0x60
 inir
 ei
 ret
 __endasm;
}
static void vread(u32 a,u8* dst,u8 n) {vr_a0=(u8)a;vr_a1=(u8)(a>>8);vr_a2=(u8)(a>>16);vr_dst=dst;vr_n=n;vread_asm();}
/* write n bytes of a bitmap line (4 bpp, 128 bytes per line) */
static void bwrite(u8 y,u8 xb,const u8* src,u8 n) {
 u16 a=(u16)y*128+xb;u8 k;
 __asm di __endasm;
 while(io_stat&1);
 io_sel=0;io_reg=(u8)a;io_reg=(u8)(a>>8);io_reg=0;
 for(k=0;k<n;++k)io_vram=src[k];
 __asm ei __endasm;
}
static void setpix(u8* t,u8 x,u8 c) {
 u8 b=x>>1;
 if(x&1)t[b]=(t[b]&0xF0)|c;else t[b]=(t[b]&0x0F)|(u8)(c<<4);
}
static void bset(u8* m,u8 r) {m[r>>3]|=(u8)(1<<(r&7));}
static u8 bget(const u8* m,u8 r) {return m[r>>3]&(u8)(1<<(r&7));}
static void mm_erase(void) {
 u8 z[25],k,y;
 for(k=0;k<25;++k)z[k]=0;
 for(y=MM_Y-1;y<MM_Y+MM_N+1;++y)bwrite(y,103,z,25);
}
void minimap_init(void) __banked {
 u8 i;u16 j;
 mm_cint=30;mm_bint=6;mm_on=1;mm_dn=mm_odn=mm_sub=0;mm_boost=120;mm_full=1;mm_last=0;ui_prev=0;grp_last=255;grp_time=0;
 mm_ox0=mm_ox1=mm_oy0=mm_oy1=0;
 for(i=0;i<5;++i)grp_mask[i]=0;
 for(i=0;i<6;++i)mm_wm[i]=mm_om[i]=0;
 for(i=0;i<16;++i)mm_dirty[i]=255;
 for(i=0;i<MM_N;++i) {mm_col[i]=(u8)((u16)i*map_w/MM_N);mm_ym[i]=(u8)((u16)i*map_h/MM_N);}
 for(j=0;j<MM_N*24;++j)mm_buf[j]=0x11;
}
static u8 mm_color(u8 cell) {
 u8 t=mm_rowt[cell];
 if(!mm_rowf[cell])return 1;
 if(t>=10)return building_owner[t-10]?9:15;
 return mm_tcol[t>9?9:t];
}
/* One minimap row from the terrain and explored-flag rows of the map: unexplored = dark. */
static void mm_base_row(u8 r) {
 u8 ym=mm_ym[r],x,*o=mm_buf+(u16)r*24;
 vread(MAP_T+((u32)ym<<7),mm_rowt,map_w);
 vread(MAP_FOGX+((u32)ym<<7),mm_rowf,map_w);
 for(x=0;x<MM_N;x+=2)*o++=(u8)((mm_color(mm_col[x])<<4)|mm_color(mm_col[x+1]));
 bset(mm_wm,r);
}
static void mm_dots(void) {
 u8 i;mm_dn=0;
 for(i=0;i<RN && mm_dn<24;++i) {
  if(!unit_hp[i])continue;
  if(i>=N && fog_at(units[i].x,units[i].y)!=2)continue;
  mm_dx[mm_dn]=(u8)((u16)units[i].x*MM_N/map_w);mm_dy[mm_dn]=(u8)((u16)units[i].y*MM_N/map_h);mm_dc[mm_dn]=i>=N?9:15;++mm_dn;
 }
}
/* Rewrite only the rows whose picture changed: rebuilt rows, rows that carried an overlay, rows that carry one now. */
static void mm_compose(void) {
 u8 r,i,x0,x1,y0,y1,t[25],nm[6],rows[6],same;
 const u8* src;
 mm_dots();
 x0=(u8)((u16)(cam_x>>4)*MM_N/map_w);x1=(u8)((u16)((cam_x+255)>>4)*MM_N/map_w);
 y0=(u8)((u16)(cam_y>>4)*MM_N/map_h);y1=(u8)((u16)((cam_y+159)>>4)*MM_N/map_h);
 if(x1>=MM_N)x1=MM_N-1;if(y1>=MM_N)y1=MM_N-1;
 same=!mm_full && mm_dn==mm_odn && x0==mm_ox0 && x1==mm_ox1 && y0==mm_oy0 && y1==mm_oy1;
 for(i=0;same && i<mm_dn;++i)if(mm_dx[i]!=mm_odx[i] || mm_dy[i]!=mm_ody[i])same=0;
 for(i=0;same && i<6;++i)if(mm_wm[i])same=0;
 if(same)return;
 for(i=0;i<6;++i)nm[i]=0;
 for(i=0;i<mm_dn;++i)bset(nm,mm_dy[i]);
 for(i=0;i<6;++i)rows[i]=mm_full?255:(u8)(mm_wm[i]|mm_om[i]|nm[i]);
 /* the rectangle's rows only matter when it moved (old rows to erase, new rows to draw) */
 if(x0!=mm_ox0 || x1!=mm_ox1 || y0!=mm_oy0 || y1!=mm_oy1) {
  for(r=mm_oy0;r<=mm_oy1;++r)bset(rows,r);
  for(r=y0;r<=y1;++r)bset(rows,r);
 }
 if(mm_full) {
  t[0]=0x0A;for(i=1;i<25;++i)t[i]=0xAA;
  bwrite(MM_Y-1,103,t,25);bwrite(MM_Y+MM_N,103,t,25);
 }
 t[0]=0x0A;
 for(r=0;r<MM_N;++r) {
  u8* row=t+1,k;
  if(!bget(rows,r))continue;
  src=mm_buf+(u16)r*24;
  Mem_Copy(src,row,24);
  for(k=0;k<mm_dn;++k)if(mm_dy[k]==r)setpix(row,mm_dx[k],mm_dc[k]);
  if(r==y0 || r==y1) {for(k=x0;k<=x1;++k)setpix(row,k,11);}
  else if(r>y0 && r<y1) {setpix(row,x0,11);setpix(row,x1,11);}
  bwrite(MM_Y+r,103,t,25);
 }
 for(i=0;i<6;++i) {mm_om[i]=nm[i];mm_wm[i]=0;}
 for(i=0;i<mm_dn;++i) {mm_odx[i]=mm_dx[i];mm_ody[i]=mm_dy[i];}
 mm_odn=mm_dn;mm_ox0=x0;mm_ox1=x1;mm_oy0=y0;mm_oy1=y1;mm_full=0;
}
/* Lowest map row flagged dirty (flag cleared), 255 if none. */
static u8 mm_next_dirty(void) {
 u8 i,b,k;
 for(i=0;i<16;++i) {
  b=mm_dirty[i];
  if(b) {
   for(k=0;!(b&1);b>>=1)++k;
   mm_dirty[i]&=(u8)~(1<<k);return (u8)((i<<3)|k);
  }
 }
 return 255;
}
void minimap_tick(void) __banked {
 u16 now;u8 m,tries,y,r;
 if(!mm_on)return;
 if(mm_boost || ++mm_sub>=mm_bint) {
  mm_sub=0;m=mm_boost?3:1;if(mm_boost)--mm_boost;
  /* rebuild up to m rows whose map row changed since they were last sampled (map rows no minimap row samples are skipped) */
  for(tries=0;tries<24 && m;++tries) {
   y=mm_next_dirty();if(y==255)break;
   r=(u8)(((u16)y*MM_N+map_h-1)/map_h);
   if(r<MM_N && mm_ym[r]==y) {mm_base_row(r);--m;}
  }
 }
 now=video_clock();
 if((u16)(now-mm_last)>=mm_cint) {mm_compose();mm_last=now;}
}
u8 mm_hit(u8 x,u8 y) __banked {return mm_on && x>=MM_X && y>=MM_Y && y<MM_Y+MM_N;}
static void cam_center(u8 cx,u8 cy) {
 i16 x=(i16)cx*16+8-128,y=(i16)cy*16+8-80,mx=(i16)map_w*16-256,my=(i16)map_h*16-160;
 if(x<0)x=0;if(x>mx)x=mx;if(y<0)y=0;if(y>my)y=my;
 cam_x=x;cam_y=y;
 if(placement_mode) {place_x=(cam_x+mouse_x)>>4;place_y=(cam_y+mouse_y)>>4;place_valid=mouse_y<160 && placement_valid(place_x,place_y);}
 video_camera();sprites_dirty|=3;mm_last=0;
}
void mm_click(u8 x,u8 y,u8 right) __banked {
 u8 cx=(u8)((u16)(x-MM_X)*map_w/MM_N),cy=(u8)((u16)(y-MM_Y)*map_h/MM_N);
 if(cx>=map_w)cx=map_w-1;if(cy>=map_h)cy=map_h-1;
 if(right)order_at(cx,cy);else cam_center(cx,cy);
}
static void say(u8 id) {ui_note_id=id;ui_note_t=3;hud_dirty=3;}
void ui_keys(void) __banked {
 u8 cur,edge,i;
 /* straight from the keyboard matrix (a bit is 0 when pressed): digits 1..5 = row 0 bits 1..5, TAB = row 7 bit 3, B = row 2 bit 7, P = row 4 bit 5 */
 cur=(u8)((~kbrow[0]>>1)&0x1F)<<3;
 if(!(kbrow[7]&8))cur|=1;
 if(!(kbrow[2]&128))cur|=2;
 if(!(kbrow[4]&32))cur|=4;
 edge=cur&~ui_prev;ui_prev=cur;
 if(!edge)return;
 if(edge&1) {
  mm_on^=1;
  if(mm_on) {mm_last=0;mm_full=1;} else mm_erase();
 }
 if(edge&2)cam_center(buildings[0].x+2,buildings[0].y+2);
 if(edge&4) {
  if(!fm_found)say(TXT_NOTE_NO_FM);
  else if(opt_music || opt_sfx) {music_stop();opt_music=opt_sfx=0;say(TXT_NOTE_SOUND_OFF);}
  else {opt_music=opt_sfx=1;music_cur=255;music_play(SONG_CALM);say(TXT_NOTE_SOUND_ON);}
 }
 for(i=0;i<5;++i)if(edge&(u8)(8<<i)) {
  if(KP(KEY_CTRL)) {grp_mask[i]=selection_mask;sfx_play(SFX_CONFIRM);say(TXT_NOTE_GROUP);}
  else if(grp_mask[i]) {
   u16 now=video_clock();
   if(selection_mask==grp_mask[i] && grp_last==i && (u16)(now-grp_time)<30 && selected!=NONE)cam_center(units[selected].x,units[selected].y);
   else {selection_mask=grp_mask[i];building_selected=0;enemy_selected=NONE;selection_update();sfx_play(SFX_CLICK);}
   grp_last=i;grp_time=now;
  }
 }
}
