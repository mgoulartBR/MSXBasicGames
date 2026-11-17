#include "castle.h"
#include "boobus.h"
#include "swamp.h"
#include "bay.h"
#include "hotcode.h"
#include "grove.h"
#include "apple.h"
#include "city.h"
#include "pass.h"
#include "rapids.h"
#include "hw_tomato.h"
// SPDX-License-Identifier: GPL-2.0-or-later
// Keen Dreams MSX 0.5.1: movement and original tile collision prototype.
#include "msxgl.h"
#include "rom_mapper.h"
#include "font/font_mgl_sample6.h"
#include "scene_data.h"
#include "game.h"
#include "world.h"
#include "combat_assets.h"
#include "combat.h"
#include "tomato.h"
#include "bridge.h"
#include "hill.h"
#include "mines.h"
#include "campaign.h"
#include "benchmark.h"

// Faster banked-call trampoline (replaces the SDCC library one; same contract, e=bank hl=function):
// get_bank/set_bank are inlined (3 call/ret pairs less per banked call). The ASCII16 register write and the
// current-bank shadow at 0xC004 are exactly crt0's set_bank.
void BankTrampoline(void) __naked
{
__asm
 .globl ___sdcc_bcall_ehl
___sdcc_bcall_ehl::
 ld a,(0xC004)
 push af
 inc sp
 call bcall_go
 dec sp
 pop bc
 push af
 ld a,b
 ld (0x77FF),a
 ld (0xC004),a
 pop af
 ret
bcall_go:
 ld a,e
 ld (0x77FF),a
 ld (0xC004),a
 jp (hl)
__endasm;
}

// Shared by the Apple/Bay/Castle/Grove static-object lists. ca_x = table (records start with u16 x,y), ca_wx = record
// size, ca_wy = address of the 32-bit `taken` mask (bonus_taken or pickup_taken), ca_r = entry count (<= 40). Writes to idx[] (ascending) the indices of the not-yet-taken bonuses
// (bit i of bonus_taken for i < 32, bonus_high for 32..39) that can be on screen, with a generous
// superset of the exact sprite test (-382 < dx < 256, -382 < dy < 192); returns the count.
extern u16 ca_x,ca_r,ca_wx,ca_wy;
u8 BonusList(u8* idx) __naked
{
 idx;
 __asm
 push ix
 push iy
 push hl
 pop iy
 push iy
 ld ix,(_ca_x)
 ld b,#0
bl_loop:
 ld a,b
 and #7
 ld c,a
 ld a,b
 rrca
 rrca
 rrca
 and #31
 cp #4
 jr nc,bl_hi
 ld hl,(_ca_wy)
 add a,l
 ld l,a
 jr nc,bl_ld
 inc h
bl_ld:
 ld a,(hl)
 jr bl_t
bl_hi:
 ld a,(_world_collision+1151)
bl_t:
 inc c
bl_sh:
 dec c
 jr z,bl_tz
 rrca
 jr bl_sh
bl_tz:
 and #1
 jr nz,bl_next
 ld l,(ix+0)
 ld h,(ix+1)
 ld de,(_camera_px)
 or a
 sbc hl,de
 ld de,#382
 add hl,de
 ld de,#638
 or a
 sbc hl,de
 jr nc,bl_next
 ld l,(ix+2)
 ld h,(ix+3)
 ld de,(_camera_py)
 or a
 sbc hl,de
 ld de,#382
 add hl,de
 ld de,#574
 or a
 sbc hl,de
 jr nc,bl_next
 ld (iy+0),b
 inc iy
bl_next:
 ld a,(_ca_wx)
 ld e,a
 ld d,#0
 add ix,de
 inc b
 ld a,(_ca_r)
 cp b
 jr nz,bl_loop
 pop de
 push iy
 pop hl
 or a
 sbc hl,de
 ld a,l
 pop iy
 pop ix
 ret
 __endasm;
}
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count);

extern u8 g_CurrentSegment;
void HudInit(void) __banked;
void CpuAuto(void) __banked;
void WorldMap(u8 completed) __banked;
extern volatile u8 hud_debug;
volatile u8 scene_ready;
volatile u8 hardware_scroll,render_commit;
// Vblank flip request (6 bytes, replaces the old profiling counters): the ISR applies it.
// flip_flag: bit0 pending, bit1 page (R#2), bit2 scroll (R#23/26/27 from drawn_camera_*)
volatile u8 flip_flag,flip_r2,flip_r5,flip_r6,flip_r8,flip_r11;
volatile u8 camera_x, camera_y;
volatile u16 render_count;
volatile u8 front_page;
u8 page_cy[2];
u16 page_px[2];
volatile u16 camera_py,drawn_camera_py;
u8 RingStable(u8 page) __banked;
void RingScene(u8 page) __banked;
void RingReset(void) __banked;
volatile u16 camera_px,drawn_camera_px,scroll_copies;
volatile u16 full_redraws;
volatile u16 max_backlog;
volatile u16 drawn_x,drawn_y;
volatile u8 drawn_frame,drawn_cx,drawn_cy;

void TimpConvert(void) __banked;
static void LoadAtlas(u8 world)
{
    u8 bank;
    HWHide();
    if(!world && active_level==12){CastleLoad();hot_entry=(u16)CastleMapTile;}else if(!world && active_level==11){BoobusLoad();hot_entry=hot_enabled?(u16)hot_code:(u16)MapTileKernel;}else if(!world && active_level==10){SwampLoad();hot_entry=hot_enabled?(u16)hot_code:(u16)MapTileKernel;}else if(!world && active_level==9){BayLoad();hot_entry=hot_enabled?(u16)hot_code:(u16)MapTileKernel;}else if(!world && active_level==8){GroveLoad();hot_entry=hot_enabled?(u16)hot_code:(u16)MapTileKernel;}else if(!world && active_level==7){
        static const u8 banks[8]={43,47,42,26,51,52,59,21};
        static const u16 offsets[8]={0x9000,0x8800,0x9000,0xa500,0xab00,0xa600,0xb500,0xb600};
        static const u16 lengths[8]={12288,10240,11776,6912,5376,6656,2816,1280};
        u16 dest=8192;
        SET_BANK_SEGMENT(1,1);VPF((const u8*)0x8000,0,1,8192);
        for(bank=0;bank<8;++bank){SET_BANK_SEGMENT(1,banks[bank]);VPF((const u8*)offsets[bank],dest,1,lengths[bank]);dest+=lengths[bank];}
        for(bank=0;bank<4;++bank){SET_BANK_SEGMENT(1,22+bank);Mem_Copy((const void*)0xb800,world_map+(u16)bank*2048,2048);}
        SET_BANK_SEGMENT(1,31);Mem_Copy((const void*)0xbc00,world_map+8192,716);
        SET_BANK_SEGMENT(1,21);g_CurrentSegment=21;Mem_Copy((const void*)0xbb00,world_collision,1168);
        hot_entry=hot_enabled?(u16)hot_code:(u16)MapTileKernel;
    }else if(!world && active_level==6){
        static const u8 city_banks[6]={30,41,49,45,44,47};
        for(bank=0;bank<6;++bank){SET_BANK_SEGMENT(1,city_banks[bank]);VPF((const u8*)(bank==5?0xb000:0x9000),(u16)bank*12288,1,bank==5?4096:12288);}
        SET_BANK_SEGMENT(1,29);g_CurrentSegment=29;
        Mem_Copy((const void*)0x9000,world_map,8726);Mem_Copy((const void*)0xb300,world_collision,1168);
        hot_entry=(u16)CityMapTile;
    }else{
    for(bank=1; bank<=4; ++bank)
    {
        SET_BANK_SEGMENT(1, world?bank+8:(active_level==5?bank+59:active_level==4?bank+53:active_level==3?bank+31:active_level==2?bank+15:bank));
        VPF((const u8*)0x8000, (u16)(bank-1)*0x4000, 1, 0x4000);
    }
    SET_BANK_SEGMENT(1, world?13:(active_level==5?40:active_level==4?58:active_level==3?36:active_level==2?20:5));
    g_CurrentSegment=world?13:(active_level==5?40:active_level==4?58:active_level==3?36:active_level==2?20:5);
    Mem_Copy((const void*)(active_level==5 && !world?0x9400:0x8000),world_map,world?6312:(active_level==5?8910:active_level==4?3830:active_level==3?4534:active_level==2?9835:6876));
    if(!world && active_level==4)Mem_Copy((const void*)0xb500,world_map+5000,1088);
    if(!world)Mem_Copy((const void*)(active_level==5?0xb800:0xb000),world_collision,1168);
        hot_entry=hot_enabled?(u16)hot_code:(u16)MapTileKernel;
    }
    cart_cache_valid=0;
    HWHide();HudInit();
    if(!world)TimpConvert();
}

void LogicStep2(u8 input) __banked;
void LevelSelect(void) __banked;
void DrawScene(void) __banked;
void DetectClock(u8 hz) __banked;
void FollowCamera(void) __banked;
u8 ReadInput(void) __banked;

void TitleScreen(void) __banked;
// Runs from the fixed bank while the asset window is switched.
void TitleLoad(void)
{
    u8 bank,prior=g_CurrentSegment;
    for(bank=0;bank<4;++bank){
        SET_BANK_SEGMENT(1,37+bank);
        VPF((const u8*)0x8000,(u16)bank*0x4000,0,bank==3?5120:0x4000);
    }
    SET_BANK_SEGMENT(1,prior);
}

void HardwareBoot(void) __banked;
void Phase2Run(void) __banked;
void Phase3Run(void) __banked;
void Core15Run(void) __banked;
void Core15Call(u8 action){u8 prior=g_CurrentSegment;SET_BANK_SEGMENT(1,48);((void(*)(u8))0x8000)(action);SET_BANK_SEGMENT(1,prior);}
#ifdef KEEN_DIAGNOSTICS
// Sampled bank fingerprint, executed from the fixed ROM window.
u16 BootBankHash(u8 bank)
{
    u8 prior=g_CurrentSegment;u16 h=0x4b44;const u8* p;
    SET_BANK_SEGMENT(1,bank);
    for(p=(const u8*)0x8000;p<(const u8*)0xc000;p+=64)h=((h<<1)|(h>>15))^*p;
    SET_BANK_SEGMENT(1,prior);return h;
}

// Read the mapped copy of bank zero while executing in the fixed bank.
u8 BootProbe(u16 offset,u8 bank)
{
 u8 prior=g_CurrentSegment,value;
 SET_BANK_SEGMENT(1,bank);value=*((const u8*)(0x8000+offset));
 SET_BANK_SEGMENT(1,prior);return value;
}

#endif

// Paced VRAM write (same signature as VDP_WriteVRAM_128K). Plain OTIR (21 T/byte) overruns the
// V9938/V9958 data port at 7.16 MHz and bytes are dropped; with vdp_fast (set by DetectClock) each
// byte takes 32 T. Lives in the fixed bank so the caller's 0x8000 window stays mapped.
extern u8 vdp_fast,bar_ok;
void VPF(const u8* src,u16 destLow,u8 destHigh,u16 count)
{
 src;destLow;destHigh;count;
 __asm
  ld iy,#2
  add iy,sp
  ld a,0 (iy)
  add a,a
  add a,a
  ld c,a
  ld a,d
  rlca
  rlca
  and #3
  add a,c
  di
  out (0x99),a
  ld a,#0x8E
  out (0x99),a
  ld a,e
  out (0x99),a
  ld a,d
  and #0x3F
  or #0x40
  ei
  out (0x99),a
  ld c,#0x98
  ld e,1 (iy)
  ld d,2 (iy)
  ld a,d
  or e
  jr z,vpf_done
  ld a,(_vdp_fast)
  or a
  jr z,vpf_fast
vpf_slow:
  outi
  nop
  dec de
  ld a,d
  or e
  jr nz,vpf_slow
  jr vpf_done
vpf_fast:
  ld b,e
  ld a,e
  or a
  jr z,vpf_hi
  otir
vpf_hi:
  ld a,d
  or a
  jr z,vpf_done
vpf_pg:
  ld b,#0
  otir
  dec a
  jr nz,vpf_pg
vpf_done:
 __endasm;
}

void main(void)
{
    u8 in,now,last,elapsed,hz,oldframe;
    u16 credit=0,oldx,oldy;
    
    bar_ok=0;vdp_fast=0;   // RAM is not cleared at boot; DetectClock decides after the title/map
    flip_flag=0;   // RAM is not cleared at boot: the vblank ISR must not see garbage
    HardwareBoot();
    shots_fired=flower_hits=flower_returns=defeats=0;
    pickups_collected=seeds_recovered=empty_throws=0;HudInit();
    enemy_mode=0;
    scene_ready=0; render_count=0; front_page=0;full_redraws=0;max_backlog=0;
    simulation_ticks=0;wall_hits=0;ceiling_hits=0;slope_hits=0;respawns=0;
    camera_x=2;camera_y=2;camera_px=32;scroll_copies=0;
    page_px[0]=page_px[1]=65535;page_cy[0]=page_cy[1]=255;
    SoundInit();
    TitleScreen();
    VDP_SetMode(VDP_MODE_SCREEN5);VDP_SetColor(0);VDP_EnableVBlank(TRUE);
    VDP_EnableSprite(FALSE);VDP_EnableTransparency(FALSE);VDP_ClearVRAM();
    VDP_SetPalette(scene_palette);
    hardware_scroll=Sys_GetMSXVersion()>=2;
    camera_py=(u16)camera_y*16;
    if(hardware_scroll)
    {
        VDP_SetLineCount(VDP_LINE_192);
        
        camera_y=camera_py>>4;
        VDP_RegWrite(25,2);VDP_RegWrite(26,0);VDP_RegWrite(27,0);
    }
    VDP_CommandHMMM(0,0,0,256,256,212);VDP_CommandWait();
    CampaignNew();active_level=1;
    if(!bench_requested){LoadAtlas(1);WorldMap(0);}
    DetectClock(VDP_GetFrequency()?50:60);   // before the first scene: it selects the fixed HUD bar
    LoadAtlas(0);RingReset();Game_Reset();Game_Tick(0);CombatReset();FollowCamera();DrawScene();scene_ready=0xa5;
    if(bench_requested==4)Core15Run();
    if(bench_requested==3)Phase3Run();
    if(bench_requested==2)Phase2Run();
    if(bench_requested)BenchmarkRun();
    hz=VDP_GetFrequency()?50:60;
    last=*((volatile u8*)M_JIFFY);
    for(;;)
    {
        now=*((volatile u8*)M_JIFFY);elapsed=now-last;last=now;
        credit+=(u16)elapsed*70;
        if(credit>max_backlog)max_backlog=credit;
        if(credit<hz) {Halt();continue;}
        in=ReadInput();oldx=player_x>>4;oldy=player_y>>4;oldframe=player_frame;
        LevelSelect();
        while(credit>=hz && !level_done && !restart_pending)
        {
            if(credit>=hz+hz && !death_ticks && !(in&32)){credit-=hz+hz;LogicStep2(in);continue;}
            credit-=hz;
            if(!death_ticks)Game_Tick(in);else ++simulation_ticks;
            if(!level_done)CombatTick(in);
        }
        if(restart_pending || level_done){
            HWHide();
            if(restart_pending==2){GameOver();CampaignNew();LoadAtlas(1);WorldMap(0);}
            else if(level_done){
                if(active_level==11){GameFinale();CampaignNew();LoadAtlas(1);WorldMap(0);}
                else {LevelComplete();LoadAtlas(1);WorldMap(active_level);}
            }
            restart_pending=0;LoadAtlas(0);Game_Reset();CombatReset();RingReset();
            page_px[0]=page_px[1]=65535;page_cy[0]=page_cy[1]=255;
            FollowCamera();DrawScene();credit=0;last=*((volatile u8*)M_JIFFY);continue;
        }
        FollowCamera();
        if(combat_dirty || oldx!=(player_x>>4) || oldy!=(player_y>>4) || oldframe!=player_frame || (in&32))
            DrawScene();
    }
}

// Fixed bank transfer: the caller's code bank remains selected after upload.
void AppleUpload(u8 bank,const u8* src,u16 dst,u16 length)
{
 u8 prior=g_CurrentSegment;SET_BANK_SEGMENT(1,bank);VPF(src,dst,0,length);SET_BANK_SEGMENT(1,prior);
}

// Loading only: use map RAM as a 4 KiB LZSS window, then load the map.
void GroveDecode(u8 bank,const u8* source,u16 y,u16 rows)
{
 u8 prior=g_CurrentSegment,flags=0,bits=0,lo,hi,n,value;
 u16 head=(y*128)&4095,stop=y+rows,back;
 SET_BANK_SEGMENT(1,bank);
 while(y<stop){
  if(!bits){flags=*source++;bits=8;}
  if(flags&1){value=*source++;n=1;back=0;}else{lo=*source++;hi=*source++;back=(head-(((u16)(hi&240)<<4)|lo)-1)&4095;n=(hi&15)+3;}
  if(back || !(flags&1)){while(n--){value=world_map[back];back=(back+1)&4095;world_map[head]=value;head=(head+1)&4095;if(!(head&127)){VPF(world_map+((head-128)&4095),y*128,1,128);++y;}}}
  else{world_map[head]=value;head=(head+1)&4095;if(!(head&127)){VPF(world_map+((head-128)&4095),y*128,1,128);++y;}}
  flags>>=1;--bits;
 }
 SET_BANK_SEGMENT(1,prior);
}
void GroveCopy(u8 bank,const u8* source,u8* dest,u16 count)
{
 u8 prior=g_CurrentSegment;SET_BANK_SEGMENT(1,bank);Mem_Copy(source,dest,count);SET_BANK_SEGMENT(1,prior);
}

static void ActorAtlas(u8 bank,u8 base){u8 prior=g_CurrentSegment;SET_BANK_SEGMENT(1,bank);VPF((const u8*)0x8000,(u16)(bank-base)*16384,1,16384);SET_BANK_SEGMENT(1,prior);}
void BayAtlas(u8 bank){ActorAtlas(bank,64);}
void SwampAtlas(u8 bank){ActorAtlas(bank,70);}
void BoobusAtlas(u8 bank){ActorAtlas(bank,76);}
void BoobusRows(u8 frame,u8 first,u8 rows,u8 width){u8 row,prior=g_CurrentSegment;const u8* p=(const u8*)0x8000+(u16)first*width;SET_BANK_SEGMENT(1,82+frame-108);VDP_CommandWait();for(row=0;row<rows;++row)VPF(p+(u16)row*width,(u16)(BOOBUS_STREAM_ROW+row)*128,1,width);SET_BANK_SEGMENT(1,prior);}

void CastleAtlas(u8 bank){ActorAtlas(bank,92);}

// Two 52-line halves fit side by side in the V9958 page's hidden 64 rows.
// Rows remain original packed mask+color bytes, with no new RAM allocation.
void BoobusTexture(u8 frame,u8 page,u8 texture_y,u8 width,u8 height)
{
 u8 row,prior=g_CurrentSegment;const u8* p=(const u8*)0x8000;
 u16 start=(u16)page*32768+(u16)texture_y*128,dst=start;
 SET_BANK_SEGMENT(1,82+frame-108);VDP_CommandWait();
 for(row=0;row<height;++row){
  if(row==52)dst=start+64;
  VPF(p,dst,0,width);p+=width;
  dst=(dst&0x8000)|((dst+128)&0x7fff);
 }
 SET_BANK_SEGMENT(1,prior);
}

#include "sprite_color_pass.c"

#include "sound_irq.c"
