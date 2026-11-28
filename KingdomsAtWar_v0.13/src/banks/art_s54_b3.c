#include "../shared.h"
#include "../vdefs.h"
#include "../text.h"
#include "../screens.h"
#include "../enemy_defs.h"
/* Boot loader of the V9990 renderer (moved out of the fixed ROM, which is full): resets the chip, writes the HUD patterns,
 * fonts, sprite cells, world tiles and poses, and finally initialises the world (the map lives in VRAM). */
__sfr __at(0x60) io_vram;
__sfr __at(0x63) io_reg;
__sfr __at(0x64) io_sel;
extern const Asset static_assets[],font_asset,poses[],enemy_tiles_asset,enemy_tiles_asset2,font_ext_asset,icons_asset;
extern const u8 kind_sizes[],pose_count[],pose_base[],palette9[],enemy_palette[],cursor[],ring_pixels[];
static void io_vset(u32 a) {io_sel=0;io_reg=(u8)a;io_reg=(u8)(a>>8);io_reg=(u8)(a>>16);}
/* ---- pattern memory ---- */
/* Loader helper (the load screen is dominated by VRAM writes): vr_rows rows of vr_w bytes go to the V9990 at address
 * vr_hi:vr_lo, 128 bytes apart (one row of the 256 px wide bitmap view); the source advances vr_s bytes per row.
 * One call replaces vr_rows V9_WriteVRAM calls and their 32-bit address arithmetic. Runs with interrupts off. */
static u16 vr_lo;static u8 vr_hi,vr_w,vr_s,vr_rows;static const u8* vr_src;
static void vram_rows_run(void) __naked {
 __asm
 di
vr_loop:
 xor a,a
 out (#0x64),a
 ld a,(_vr_lo)
 out (#0x63),a
 ld a,(_vr_lo+1)
 out (#0x63),a
 ld a,(_vr_hi)
 out (#0x63),a
 ld hl,(_vr_src)
 push hl
 ld a,(_vr_w)
 ld b,a
 ld c,#0x60
 otir
 pop hl
 ld a,(_vr_s)
 ld e,a
 ld d,#0
 add hl,de
 ld (_vr_src),hl
 ld hl,(_vr_lo)
 ld de,#128
 add hl,de
 ld (_vr_lo),hl
 jr nc,vr_nc
 ld hl,#_vr_hi
 inc (hl)
vr_nc:
 ld hl,#_vr_rows
 dec (hl)
 jr nz,vr_loop
 ei
 ret
 __endasm;
}
static void vram_rows(u32 a,const u8* src,u8 w,u8 s,u8 rows) {vr_lo=(u16)a;vr_hi=(u8)(a>>16);vr_src=src;vr_w=w;vr_s=s;vr_rows=rows;vram_rows_run();}
/* tile_buffer fix-ups for the loader (Z80): transparent nibbles become the grass colour 3 for the opaque-tile patterns;
 * tile_dither() overlays the remembered-fog checker (colour 1 on alternate pixels, per row parity). */
static void tile_fix(void) __naked {
 __asm
 ld hl,#_tile_buffer
 ld b,#128
tf_lp:
 ld a,(hl)
 ld c,a
 and a,#0xF0
 jr nz,tf_1
 ld a,c
 or a,#0x30
 ld c,a
tf_1:
 ld a,c
 and a,#0x0F
 jr nz,tf_2
 ld a,c
 or a,#0x03
 ld c,a
tf_2:
 ld (hl),c
 inc hl
 djnz tf_lp
 ret
 __endasm;
}
static void tile_dither(void) __naked {
 __asm
 ld hl,#_tile_buffer
 ld c,#8
td_pair:
 ld b,#8
td_even:
 ld a,(hl)
 and a,#0x0F
 or a,#0x10
 ld (hl),a
 inc hl
 djnz td_even
 ld b,#8
td_odd:
 ld a,(hl)
 and a,#0xF0
 or a,#0x01
 ld (hl),a
 inc hl
 djnz td_odd
 dec c
 jr nz,td_pair
 ret
 __endasm;
}
static void pat_write(u32 base,u16 n,const u8* src) {
 vram_rows(base+((u32)(n>>5)<<10)+((n&31)<<2),src,4,4,8);
}
/* A 16x16 tile (16 rows of 8 bytes) as four 8x8 patterns: TL, TR, BL, BR. */
static void tile_patterns(u32 base,u16 n,const u8* t) {
 u8 q;
 for(q=0;q<4;++q) {
  u16 pn=n+q;
  vram_rows(base+((u32)(pn>>5)<<10)+((pn&31)<<2),t+(u16)((q>>1)<<6)+((q&1)<<2),4,8,8);
 }
}

static void px4(u8* buf,u8 x,u8 y,u8 c) {u8* p=buf+((u16)y<<3)+(x>>1);if(x&1)*p=(*p&0xF0)|c;else *p=(*p&0x0F)|(c<<4);}

static void cell_write(u8 cell,const u8* src16rows) {
 u8 k;for(k=0;k<16;++k)V9_WriteVRAM(P1_SGT+((u32)((cell>>4)<<4)+k)*128+((cell&15)<<3),src16rows+(k<<3),8);
}

static void load_progress(u8 v) {u8 i;if(v>16)v=16;for(i=0;i<v;++i)hud_name(8+i,24,PA_SOLID+10);}

/* Look for an MSX-MUSIC (YM2413) in every slot: the internal one shows APRLOPLL at 4018h, the FM-PAC cartridge PAC2OPLL (enabled through 7FF6h). */
static u8 opll_ident(u8 slot) {
 static const u8 id[4]={'O','P','L','L'};u8 i;
 for(i=0;i<4;++i)if(BIOS_InterSlotRead(slot,0x401C+i)!=id[i])return 0;
 if(BIOS_InterSlotRead(slot,0x4018)!='A')BIOS_InterSlotWrite(slot,0x7FF6,BIOS_InterSlotRead(slot,0x7FF6)|1);
 return 1;
}
u8 music_detect(void) __banked {
 u8 s,ss,n;
 for(s=0;s<4;++s) {
  u8 exp=(*(volatile u8*)(0xFCC1+s))&0x80;n=exp?4:1;
  for(ss=0;ss<n;++ss)if(opll_ident(exp?(u8)(0x80|s|(ss<<2)):s))return 1;
 }
 return 0;
}
/* V9990 reset and P1 mode (once, before the world is written to VRAM). */
void video_boot(void) __banked {
 VDP_SetMode(VDP_MODE_SCREEN5);VDP_SetFrequency(VDP_FREQ_60HZ);VDP_EnableVBlank(TRUE);VDP_EnableDisplay(FALSE);
 V9_SetPort(V9_P07,2);V9_SetPort(V9_P07,0);
 V9_SetScreenMode(V9_MODE_P1);V9_SetDisplayEnable(FALSE);V9_SetInterrupt(V9_INT_NONE);
 V9_SetCursorEnable(FALSE);V9_SetSpriteEnable(TRUE);V9_SetSpritePatternAddr(V9_P1_SGT_38000);
 V9_SetBackgroundColor(0);V9_SetCommandWriteMask(0xFFFF);V9_SetScrolling(0,0);V9_SetScrollingB(0,0);
}
void video_init(void) __banked {
 u8 i,j,q;u16 y;
 rf_cx=rf_cy=0;

 for(i=0;i<64;++i)V9_SetPaletteEntry(i,palette9+(i&15)*3);
 /* sprite palette group 1 belongs to the enemy kingdom (its poses use their own 15 colours) */
 for(i=0;i<16;++i)V9_SetPaletteEntry(16+i,enemy_palette+i*3);
 cache_tick=frame_uploads=rendered_frames=0;
 reset_caches();
 p1_ready=0;rw_n=rc_n=rw_dirty=rw_lock=0;V9_FillVRAM(P1_SGT,0,32768); for(i=0;i<125;++i)V9_Poke(SPAT+(u16)i*4+3,0x10);
 sused=0;ord_n=0;
 /* clear both name tables and the HUD pattern table head */
 V9_FillVRAM(PNT_A,0,8192);V9_FillVRAM(PNT_B,0,8192);
 {u16 row[32];u8 cy,cx;for(cy=0;cy<20;++cy) {for(cx=0;cx<32;++cx)row[cx]=(u16)cy*32+cx;V9_WriteVRAM(PNT_A+((u16)cy<<7),(const u8*)row,64);}}
V9_FillVRAM(0x00000UL,0,30720);V9_FillVRAM(P1_PGT_B_BASE,0,1024);
 /* layer A patterns: solid colours 1..16 (pattern 1+c, c>=1 only), HUD top line */
 for(i=0;i<16;++i) {u8 pat[32];for(j=0;j<32;++j)pat[j]=i*17;pat_write(0,PA_SOLID+i,pat);}
 {u8 pat[32];for(j=0;j<32;++j)pat[j]=(j<8)?0xAA:0x11;pat_write(0,PA_HUDTOP,pat);}
 /* font: 64 chars as 256x16 bitmap strip -> two pattern rows; colour 0 would be transparent, use 1 */
 asset_begin(&font_asset);
 for(i=0;i<16;++i) {u8 b;asset_read(tile_buffer,128);for(b=0;b<128;++b){u8 v=tile_buffer[b];if(!(v&0xF0))v|=0x10;if(!(v&0x0F))v|=0x01;tile_buffer[b]=v;}
  V9_WriteVRAM((u32)((PA_FONT>>5)*8+i)*128,tile_buffer,128);}
 /* extended glyphs (accents, kana): codes 96..255 = patterns 736..895, lines 184..223 */
 asset_begin(&font_ext_asset);
 for(i=0;i<40;++i) {asset_read(tile_buffer,128);V9_WriteVRAM((u32)(184+i)*128,tile_buffer,128);}
 /* command icons: a 256x16 strip, VRAM lines 224..239 (layer A patterns 896..959) */
 asset_begin(&icons_asset);
 for(i=0;i<16;++i) {asset_read(tile_buffer,128);V9_WriteVRAM((u32)(224+i)*128,tile_buffer,128);}
 screen_show(SCR_TITLE);
 {u8 r;for(r=0;r<2;++r){hud_draw=r;video_fill(0,160,256,52,1);video_fill(0,160,256,2,10);}hud_draw=0;}
 V9_SetDisplayEnable(TRUE);V9_SetPort(V9_P15,0x10);
 /* sprite cells: pointer and selection ring */
 {u8 c[128];
  for(j=0;j<128;++j)c[j]=0;
  for(i=0;i<16;++i)for(q=0;q<8;++q)if(cursor[i]&(128>>q))c[i*8+(q>>1)]|=(q&1)?0x0F:0xF0;
  cell_write(CELL_PTR,c);
  cell_write(CELL_RING,ring_pixels);
  {u8 v;static const u8 vc[3]={15,10,7},vcell[3]={CELL_VLINE15,CELL_VLINE10,CELL_VLINE7};
   for(v=0;v<3;++v){u8 k;for(k=0;k<128;++k)tile_buffer[k]=0;for(k=0;k<16;++k)tile_buffer[(u16)k<<3]=(u8)(vc[v]<<4);cell_write(vcell[v],tile_buffer);}}
 }
  {u8 c2[128],w,k,col,x,q2;
   for(col=0;col<2;++col)for(w=0;w<=8;++w) {
    for(j=0;j<128;++j)c2[j]=0;
    for(k=0;k<2;++k)for(x=0;x<16;++x)px4(c2,x,k,x<w*2?(col?9:5):12);
    cell_write((col?CELL_BAR_ENEMY:CELL_BAR_ALLY)+w,c2);
   }
   for(w=0;w<=8;++w) {
    for(j=0;j<128;++j)c2[j]=0;
    for(k=0;k<2;++k)for(x=0;x<16;++x)px4(c2,x,k,x<w*2?15:12);
    cell_write(CELL_BAR_WHITE+w,c2);
   }
   for(j=0;j<128;++j)c2[j]=0;
   for(x=6;x<11;++x)px4(c2,x,12,15);
   for(k=10;k<15;++k)px4(c2,8,k,15);
   cell_write(CELL_CROSS,c2);
   for(q2=0;q2<4;++q2) {
    for(j=0;j<128;++j)c2[j]=0;
    if(q2<2) {
     for(x=6;x<10;++x)px4(c2,x,8,11);
     for(k=7;k<10;++k)px4(c2,q2?6:10,k,15);
    } else {
     for(k=6;k<10;++k)px4(c2,8,k,11);
     for(x=7;x<10;++x)px4(c2,x,q2==2?10:6,15);
    }
    cell_write(CELL_ARROW+q2,c2);
   }
  }
 /* world patterns: fog, plain, dithered */
 {u8 pat[32];for(j=0;j<32;++j)pat[j]=0x11;pat_write(P1_PGT_B_BASE,1,pat);}
 for(q=0;q<3;++q) {
  asset_begin(static_assets+q);
  for(i=0;i<64;++i) {
   u8 t=q*64+i,b;u16 phy=t;
   asset_read(tile_buffer,128);
   if(phy>=50) {
    u16 ov=phy-50,bx=(ov&15)<<4,by=OV_Y+((ov>>4)<<4);u8 k;
    vram_rows((u32)by*128+(bx>>1),tile_buffer,8,8,16);
   }
   if(phy>=50)tile_fix();
   tile_patterns(P1_PGT_B_BASE,PB_PLAIN+phy*4,tile_buffer);
   tile_dither();
   tile_patterns(P1_PGT_B_BASE,PB_DITH+phy*4,tile_buffer);
  }
 }
 {u8 part,e=0;
  for(part=0;part<2;++part) {
   u8 cnt=part?(u8)(ENEMY_TILES-ENEMY_TILES1):(u8)ENEMY_TILES1;
   if(!cnt)continue;
   asset_begin(part?&enemy_tiles_asset2:&enemy_tiles_asset);
   for(i=0;i<cnt;++i,++e) {
    u8 b;
    asset_read(tile_buffer,128);
    {u16 bx=(u16)(e&15)<<4,by=EOV_Y+((u16)(e>>4)<<4);vram_rows((u32)by*128+(bx>>1),tile_buffer,8,8,16);}
    tile_fix();
    tile_patterns(P1_PGT_B_BASE,ENEMY_PAT+(u16)e*4,tile_buffer);
   }
  }
 }
 for(i=0;i<12;++i) {
  u8 f,n=kind_sizes[i]>>4;
  for(f=0;f<pose_count[i];++f) {
   u8 key=pose_base[i]+f;
   if(!poses[key].bank)continue;
   asset_begin(poses+key);
   {u16 px,py;u8 qx=0,qy=0,nn=n*n;pose_pos(key,kind_sizes[i],&px,&py);
   for(q=0;q<nn;++q) {
    u16 x,yy;x=px+((u16)qx<<4);yy=py+((u16)qy<<4);if(++qx==n){qx=0;++qy;}
    asset_read(tile_buffer,128);
    vram_rows((u32)yy*128+(x>>1),tile_buffer,8,8,16);
   }}
   ++boot_poses;load_progress((u8)((u16)boot_poses*16/208));
  }
 }
 wv_x0=wv_y0=wv_x1=wv_y1=0;video_scroll_update();
 for(i=0;i<16;++i)hud_name(8+i,24,PA_SOLID+1); /* the load bar (row 24) disappears */
 hud_draw=0;
}