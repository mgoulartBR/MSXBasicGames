# uso: mkprof.py <projeto> <saida.tcl> : perfil inclusivo por função durante uma caminhada, com VDP ocupado
import sys,re
from pathlib import Path
r=Path(sys.argv[1]);out=Path(sys.argv[2])
s=(r/'build/keydoor-test.tcl').read_text().split('after time 10 {')[0]
d={n:int(a,16)for a,n in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+',(r/'build/keen_msx.map').read_text(),re.M)}
import glob,os
def local_syms():
    out=r/'src/out';gabs={}
    for a,n,mod in re.findall(r'^\s+([0-9A-F]{8})\s+(_\w+)\s+(\S+)\s*$',(out/'keen_msx.map').read_text(),re.M):gabs[(mod,n)]=int(a,16)
    res={}
    for f in glob.glob(str(out/'*.sym')):
        mod=os.path.basename(f)[:-4]
        if os.environ.get('BENCH_MOD') and mod!=os.environ['BENCH_MOD']:continue
        ent=[(int(m[1]),m[2],int(m[3],16)) for m in re.finditer(r'^\s+(\d+) (_\w+)\s+([0-9A-F]{8}) [A-Z]*R',open(f).read(),re.M)]
        base=None
        for ar,n,rel in ent:
            if (mod,n) in gabs and not re.search(r'_\d{4,}_\d+$',n):base=gabs[(mod,n)]-rel;break
        if base is None:continue
        for ar,n,rel in ent:
            if not re.search(r'_\d{4,}_\d+$',n):res.setdefault(n[1:],base+rel)
    return res
_loc=local_syms()
for _k,_v in _loc.items():
    if os.environ.get('BENCH_MOD'):d[_k]=_v
    else:d.setdefault(_k,_v)
import os
FUNCS=os.environ.get('BENCH_FUNCS','DrawScene,ActorsPrepare,RingScene,ActorsDraw,ActorDraw,HudDraw,Game_Tick,CombatTick,FollowCamera,ReadInput').split(',')
s+='\n'+'\n'.join(f'set {n} {d[n]}' for n in ['render_count','active_level','scene_ready','world_active','g_CurrentSegment','enemy_mode','player_x','player_y','simulation_ticks','stable_x','stable_y','stable_ticks','defeats','game_lives'])+'\n'
spec=' '.join('%s %d %d'%(f,d[f]&0xFFFF,(d[f]>>16)&0xFF) for f in FUNCS if f in d)
s+='set ::funcs {%s}\n'%spec
s+=r'''
set ::bench_out $::env(BENCH_OUT)
if {[info exists ::env(BENCH_Z80)]} {set z80_freq_locked false;set z80_freq $::env(BENCH_Z80)}
proc logline {t} {set f [open $::bench_out a];puts $f $t;close $f}
set ::cur_seg_addr $::g_CurrentSegment
foreach {nm addr seg} $::funcs {
 set ::n_$nm 0;set ::t_$nm 0.0;set ::t0_$nm 0;set ::bp_$nm {}
 debug set_bp $addr "\[debug read memory \$::cur_seg_addr\]==$seg" [list prof_enter $nm]
}
proc prof_enter {nm} {
 set sp [reg SP];set ::t0_$nm [machine_info time]
 set ::bp_$nm [debug set_bp [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}] "\[reg SP\]==$sp+2" [list prof_exit $nm]]
}
proc prof_exit {nm} {
 upvar #0 bp_$nm bp t0_$nm t0 n_$nm n t_$nm t
 catch {debug remove_bp $bp}
 incr n;set t [expr {$t+[machine_info time]-$t0}]
}
set ::vbusy 0.0;set ::vt0 0;set ::vcmds 0
debug probe set_bp VDP.commandExecuting {} {
 if {[debug probe read VDP.commandExecuting]} {set ::vt0 [machine_info time];incr ::vcmds} else {set ::vbusy [expr {$::vbusy+[machine_info time]-$::vt0}]}
}
proc sweep {} {
 if {$::phase<0} {return}
 lassign $::env(BENCH_SWEEP) x0 y0 step period
 incr ::sk
 place [expr {$x0+($::sk*$step)%1200}] $y0
 after time $period sweep
}
set ::sk 0
proc walk {} {if {$::phase<0} {return};set ::dir [expr {1-$::dir}];controls [expr {$::dir?1:2}];after time $::env(BENCH_LEG) walk}
set ::dir 0;set ::phase -1
proc begin {} {
 set ::phase 0;set ::start [machine_info time];set ::rc0 [word $::render_count];set ::vb0 $::vbusy;set ::vc0 $::vcmds
 foreach {nm a s} $::funcs {set ::s_n_$nm [set ::n_$nm];set ::s_t_$nm [set ::t_$nm]}
 if {[info exists ::env(BENCH_PLACE)]} {place {*}$::env(BENCH_PLACE)}
 if {[info exists ::env(BENCH_ENEMY)]} {debug write memory $::enemy_mode $::env(BENCH_ENEMY)}
 if {[info exists ::env(BENCH_SWEEP)]} {sweep} elseif {![info exists ::env(BENCH_IDLE)]} {walk}
 if {[info exists ::env(BENCH_PC)]} {sample}
 if {[info exists ::env(BENCH_SHOT)]} {after time 10 {screenshot $::env(BENCH_SHOT)}}
 after time $::env(BENCH_SECS) finish
}
set ::pcs {}
proc sample {} {
 if {$::phase<0} {return}
 set pc [reg PC];set seg [debug read memory $::cur_seg_addr]
 lappend ::pcs "$pc,$seg"
 after time 0.0007 sample
}
proc finish {} {
 set f [open $::env(BENCH_OUT).pc w];puts $f [join $::pcs \n];close $f
 controls 0
 set dt [expr {[machine_info time]-$::start}];set n [expr {[word $::render_count]-$::rc0}]
 logline "defeats=[word $::defeats] lives=[byte $::game_lives]"
 logline "player: x=[word $::stable_x] y=[word $::stable_y] at_tick=[word $::stable_ticks]"
 logline "window=[format %.1f $dt]s renders=$n fps=[format %.2f [expr {$n/$dt}]] vdp_busy=[format %.0f [expr {100*($::vbusy-$::vb0)/$dt}]]% vdp_cmds=[expr {$::vcmds-$::vc0}]"
 foreach {nm a s} $::funcs {
  set c [expr {[set ::n_$nm]-[set ::s_n_$nm]}];set t [expr {[set ::t_$nm]-[set ::s_t_$nm]}]
  logline [format "  %-14s calls=%-5d total=%6.0f ms (%4.1f%% of window)  mean=%6.2f ms" $nm $c [expr {1000*$t}] [expr {100*$t/$dt}] [expr {$c?1000*$t/$c:0}]]
 }
 exit
}
proc go_level {} {
 if {[byte $::world_active]!=1} {after time 0.2 go_level;return}
 after time 1 {
  putword $::world_x [lindex $::env(BENCH_WORLD) 0];putword $::world_y [lindex $::env(BENCH_WORLD) 1];keymatrixdown 8 1
  after time 0.3 {keymatrixup 8 1;waitfor {[byte $::active_level]==$::env(BENCH_LEVEL) && [byte $::scene_ready]==165 && ![byte $::world_active]} lvl_ready}
 }
}
proc lvl_ready {} {after time $::env(BENCH_LOADWAIT) begin}
proc press {} {if {[byte $::world_active]==1} {return};keymatrixdown 8 1;after time 0.3 {keymatrixup 8 1};after time 1.5 press}
after time 6 press
go_level
'''
out.write_text(s)
