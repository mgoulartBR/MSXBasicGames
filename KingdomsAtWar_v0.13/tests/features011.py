"""Compile and exercise actual military C functions with controlled video/navigation stubs."""
from pathlib import Path
import subprocess,tempfile,json,hashlib,os
R=Path(__file__).resolve().parents[1]
def fn(src,name):
 import re
 m=re.search(r'^(?:static )?(?:void|u8|u16) '+name+r'\([^\n]*?\)\s*(?:__banked\s*)?(?:__naked\s*)?\{',src,re.M);assert m,name
 a=m.start();i=m.end();depth=1
 while depth:
  depth+=(src[i]=='{')-(src[i]=='}');i+=1
 return src[a:i].replace('__banked','').replace('__naked','')
main=(R/'src/main.c').read_text();mil=(R/'src/banks/art_s26_b3.c').read_text();tower=(R/'src/banks/art_s11_b3.c').read_text();fog=(R/'src/banks/art_s12_b3.c').read_text();repair=(R/'src/banks/art_s13_b3.c').read_text();proj=(R/'src/banks/art_s25_b3.c').read_text()
preamble=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef int16_t i16;typedef int8_t i8;
#define NONE 255
#define N 8
#define RN 10
#define BMAX 16
#define IDLE 0
#define MOVING 2
#define ARRIVED 3
typedef struct {u16 px,py;u8 x,y,tx,ty,nx,ny,owner,len,pos,progress,phase,state;} Unit;
typedef struct {u8 type,state,x,y,worker;u16 remaining;u8 training;} Building;
Unit units[RN];Building buildings[BMAX];
u8 unit_hp[RN],unit_kind[RN],unit_facing[RN],combat_target[RN],attack_flash[RN],hurt_flash[RN],attack_cooldown[RN],stance[N];
u8 unit_count,building_count,unit_build[N],requested[N],retry_wait[N],worker_task[N],cargo[N],cargo_kind[N],mine_phase[N],selection_mask,enemy_selected,builder_working,hud_dirty,sprites_dirty,combat_active,job_unit;
u8 building_hp[BMAX],building_owner[BMAX],terrain[1024];u16 sim_ticks,combat_hits,combat_buildings,combat_kills;
u16 shot_x[RN],shot_y[RN],shot_active,combat_shots; i8 shot_dx[RN],shot_dy[RN];u8 shot_time[RN],shot_target[RN],shot_damage[RN],shot_owner[RN];
const u16 render_bit[RN]={1,2,4,8,16,32,64,128,256,512};
const u8 unit_range[]={0,1,2,4,1,1,1},unit_damage[]={0,8,7,5,12,36,4},unit_cooldown[]={0,24,30,36,24,60,30};
u8 combat_alive(u8 i){return i<RN && unit_hp[i] && (i<unit_count || i>=N);}
u8 building_size(u8 t){return t==0?4:2;}
u8 tile(u8 x,u8 y){return terrain[y*32+x];}
void work_stop(u8 i){requested[i]=0;units[i].len=units[i].pos=0;units[i].state=units[i].progress?MOVING:ARRIVED;if(job_unit==i)job_unit=NONE;}
void video_known_tile(u8 x,u8 y){}void selection_update(void){}void building_refresh(u8 i){}extern u8 fog_memory[1024],art_map[1024];u8 fog_at(u8 x,u8 y);void video_tile(u8 x,u8 y){if(fog_at(x,y)==2)fog_memory[y*32+x]=art_map[y*32+x];}
void order_one(u8 i,u8 x,u8 y){requested[i]=1;units[i].tx=x;units[i].ty=y;}
u8 combat_hit_test(u16 x,u16 y){return (u8)x;}
'''
preamble=preamble.replace('#define RN 10','#define RN 10\n#define PS 16').replace('#define BMAX 16','#define BMAX 12').replace('shot_x[RN],shot_y[RN]','shot_x[PS],shot_y[PS]').replace('shot_dx[RN],shot_dy[RN]','shot_dx[PS],shot_dy[PS]').replace('shot_time[RN],shot_target[RN],shot_damage[RN],shot_owner[RN]','shot_time[PS],shot_target[PS],shot_damage[PS],shot_owner[PS]')
preamble=preamble.replace('return t==0?4:2;','return t==1?3:4;')
preamble+="""
u8 fog_seen[128],fog_visible[128],fog_next[128],fog_changed[2][128],fog_memory[1024],art_map[1024],fog_refresh,video_enabled;
u8 fog_static[128],fog_build_dirty,repair_pending,fog_need[128],fog_delta[128];u8 fog_trail[128],fog_pending[2];u8* fog_dest=fog_next;u16 fog_epoch,fog_checked_epoch[2],fog_checked_x[2],fog_checked_y[2];u16 fog_sources[N];u8 repair_target[N],repair_timer[N],repair_active,tower_cooldown[BMAX],work_timer[N],research_level,selected;u16 wood,tower_shots,repair_spent,repair_healed,fog_updates;
#define WORKER(i) ((i)<unit_count && unit_hp[i] && unit_kind[i]==0)
void poll_input(void){}
u8 fog_at(u8 x,u8 y);u8 fog_target(u8 t);void fog_update(void);u8 structure_max_hp(u8 id);u8 military_damage(u8 kind,u8 owner);void structure_dirty(u8 id);u8 line_clear(u8 x,u8 y,u8 tx,u8 ty,u8 water,u8 ignore);
u8 work_route(u8 i,u8 x,u8 y){requested[i]=1;return 1;}
u8 work_adjacent(u8 i,u8 x,u8 y){u8 n=2;for(int j=0;j<building_count;++j)if(buildings[j].x==x && buildings[j].y==y){n=building_size(buildings[j].type);break;}return (units[i].x>=x && units[i].x<x+n && (units[i].y+1==y || units[i].y==y+n)) || (units[i].y>=y && units[i].y<y+n && (units[i].x+1==x || units[i].x==x+n));}
"""
import re
stamp=(R/'src/banks/art_s15_b3.c').read_text();preamble+=stamp[stamp.index('static const'):stamp.index('void fog_stamp')]+fn(stamp,'fog_stamp')+'\n'
preamble+='\n#define __builtin_memcpy memcpy\n#define __builtin_memset memset\n'
code='\n'.join([fn(main,n) for n in ['fog_at','fog_art']]+[fn(tower,n) for n in ['line_clear','military_damage','structure_max_hp','structure_dirty','tower_tick']]+[fn(fog,n) for n in ['fog_merge','sight','fog_traverse','fog_target','fog_init','fog_update']]+[fn(mil,n) for n in ['target_box','distance','combat_position','military_selected','face','die','combat_impact','strike','combat_click','combat_stop','combat_tick']]+[fn(proj,'projectile_tick')]+[fn(repair,n) for n in ['repair_order','repair_tick']])

checks=r"""
int checks=0;
#define CHECK(e) do{++checks;if(!(e)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#e);exit(1);}}while(0)
void reset(void){
 memset(units,0,sizeof units);memset(buildings,0,sizeof buildings);memset(unit_hp,0,sizeof unit_hp);memset(unit_kind,0,sizeof unit_kind);memset(unit_build,255,sizeof unit_build);memset(combat_target,255,sizeof combat_target);memset(terrain,0,sizeof terrain);memset(art_map,0,sizeof art_map);memset(shot_time,0,sizeof shot_time);memset(requested,0,sizeof requested);memset(mine_phase,0,sizeof mine_phase);memset(stance,0,sizeof stance);memset(attack_cooldown,0,sizeof attack_cooldown);memset(retry_wait,0,sizeof retry_wait);
 unit_count=2;building_count=2;unit_hp[0]=32;unit_hp[1]=48;unit_kind[1]=1;units[0].x=3;units[0].y=4;units[1].x=8;units[1].y=4;units[8].x=20;units[8].y=23;units[8].owner=1;unit_kind[8]=6;unit_hp[8]=48;
 buildings[0].x=4;buildings[0].y=4;buildings[0].state=2;buildings[0].type=0;building_hp[0]=240;building_owner[0]=0;building_owner[1]=1;building_hp[1]=0;sim_ticks=0;research_level=0;job_unit=NONE;video_enabled=0;combat_hits=combat_kills=combat_buildings=0;selection_mask=1;wood=20;fog_init();
}
int main(void){
 for(int r=3;r<=7;++r)for(int y=0;y<32;++y)for(int x=0;x<32;++x){
  u8 expected[128]={0};memset(fog_next,0,sizeof fog_next);fog_dest=fog_next;sight(x,y,r);
  for(int yy=0;yy<32;++yy)for(int xx=0;xx<32;++xx){int dx=abs(xx-x),dy=abs(yy-y);if(dx<=r && dy<=r && dx+dy<=r+r/2){int c=yy*32+xx;expected[c>>3]|=1<<(c&7);}}
  CHECK(!memcmp(expected,fog_next,128));
 }

 reset();units[1].x=20;units[1].y=20;fog_traverse(1);units[1].x=8;units[1].y=4;fog_update();CHECK(fog_at(20,20)==1 && fog_at(8,4)==2);
 reset();CHECK(fog_at(4,4)==2);CHECK(fog_at(20,23)==0);CHECK(!fog_target(8));CHECK(fog_at(32,0)==0);
 units[1].x=20;units[1].y=20;fog_update();CHECK(fog_target(8));CHECK(fog_at(20,23)==2);CHECK(fog_seen[(23*32+20)>>3]&(1<<((23*32+20)&7)));
 art_map[23*32+20]=15;video_tile(20,23);fog_refresh=1;fog_update();CHECK(fog_art(20,23)==15);
 units[1].x=8;units[1].y=4;fog_update();CHECK(fog_at(20,23)==1 && !fog_target(8));art_map[23*32+20]=7;CHECK(fog_art(20,23)==15);
 int updates=fog_updates;fog_update();CHECK(fog_updates==updates);units[1].x=20;units[1].y=20;fog_update();CHECK(fog_art(20,23)==7);
 unit_hp[1]=0;fog_update();CHECK(fog_at(20,23)==1);units[0].x=20;units[0].y=20;fog_update();CHECK(fog_target(8));mine_phase[0]=2;fog_update();CHECK(!fog_target(8));
 reset();CHECK(military_damage(1,0)==8 && military_damage(5,0)==36);research_level=1;CHECK(military_damage(1,0)==10 && military_damage(2,0)==9 && military_damage(3,0)==7 && military_damage(4,0)==14 && military_damage(5,0)==38);CHECK(military_damage(0,0)==0 && military_damage(6,1)==4);
 combat_impact(1,4,1);CHECK(unit_hp[1]==45);combat_impact(0,4,1);CHECK(unit_hp[0]==28);combat_impact(1,1,1);CHECK(unit_hp[1]==44);combat_impact(128,4,1);CHECK(building_hp[0]==236);
 reset();buildings[0].type=6;buildings[0].x=0;buildings[0].y=0;building_hp[0]=96;units[8].x=6;units[8].y=2;memset(fog_visible,255,sizeof fog_visible);tower_tick();CHECK(tower_shots==1 && shot_time[10]==16 && shot_damage[10]==6);CHECK(unit_hp[8]==48);for(int i=0;i<16;++i)projectile_tick();CHECK(unit_hp[8]==42);CHECK(shot_owner[10]==0 && shot_target[10]==8);tower_tick();CHECK(tower_shots==1);
 tower_cooldown[0]=0;research_level=1;tower_tick();CHECK(shot_damage[10]==8);for(int i=0;i<16;++i)projectile_tick();CHECK(unit_hp[8]==34);
 tower_cooldown[0]=0;terrain[2*32+4]=1;tower_tick();CHECK(tower_shots==2);terrain[2*32+4]=0;memset(fog_visible,0,sizeof fog_visible);tower_tick();CHECK(tower_shots==2);memset(fog_visible,255,sizeof fog_visible);buildings[0].state=1;tower_tick();CHECK(tower_shots==2);buildings[0].state=2;for(int i=10;i<16;++i)shot_time[i]=1;tower_tick();CHECK(tower_shots==2);
 reset();building_hp[0]=230;CHECK(repair_order(0));CHECK(repair_target[0]==0 && worker_task[0]==0);requested[0]=0;units[0].state=ARRIVED;units[0].x=3;units[0].y=4;
 for(int i=0;i<31;++i)repair_tick();CHECK(building_hp[0]==234 && wood==19 && repair_spent==1 && repair_healed==4);
 for(int i=0;i<30;++i)repair_tick();CHECK(building_hp[0]==238 && wood==18);for(int i=0;i<31;++i)repair_tick();CHECK(building_hp[0]==240 && wood==17 && repair_healed==10 && repair_target[0]==NONE);
 CHECK(!repair_order(0));building_owner[0]=1;building_hp[0]=100;CHECK(!repair_order(0));building_owner[0]=0;building_hp[0]=230;CHECK(repair_order(0));requested[0]=0;wood=0;for(int i=0;i<100;++i)repair_tick();CHECK(building_hp[0]==230 && repair_spent==3 && repair_active==0);wood=1;repair_timer[0]=0;repair_tick();CHECK(building_hp[0]==234 && wood==0);buildings[0].state=0;repair_tick();CHECK(repair_target[0]==NONE && repair_active==0);
 reset();unit_hp[0]=0;CHECK(repair_order(0)==0); /* full building rejects before worker scan */
 reset();memset(fog_visible,255,sizeof fog_visible);unit_kind[0]=2;unit_kind[8]=4;units[8].x=5;units[8].y=4;research_level=1;CHECK(combat_position(0,3,4,8));strike(0,8);CHECK(unit_hp[8]==30);
 reset();memset(fog_visible,255,sizeof fog_visible);unit_kind[0]=3;units[0].px=48;units[0].py=64;units[8].x=7;units[8].y=4;CHECK(combat_position(0,3,4,8));strike(0,8);CHECK(shot_time[0]==16 && unit_hp[8]==48);combat_stop(0);for(int i=0;i<16;++i)projectile_tick();CHECK(unit_hp[8]==43 && shot_x[0]==120 && shot_y[0]==68);
 printf("PASS %d fog/tower/research/repair checks\n",checks);return 0;
}
"""
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(preamble+code+checks);subprocess.run(['gcc','-std=c99','-O1','-fsanitize=undefined,address',str(p/'test.c'),'-o',str(p/'test')],check=True);out=subprocess.check_output([str(p/'test')],text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));print(out)
paths=['src/main.c','src/banks/art_s11_b3.c','src/banks/art_s12_b3.c','src/banks/art_s13_b3.c','src/banks/art_s25_b3.c','src/banks/art_s26_b3.c']
report={'passed':True,'output':out.strip(),'source_sha256':{n:hashlib.sha256((R/n).read_bytes()).hexdigest() for n in paths}}
(R/'tests/features011.json').write_text(json.dumps(report,indent=2)+'\n')
