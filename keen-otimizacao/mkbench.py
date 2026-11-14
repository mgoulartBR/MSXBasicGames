# uso: mkbench.py <projeto> <saida.tcl>
import sys,re
from pathlib import Path
r=Path(sys.argv[1]);out=Path(sys.argv[2])
# cabeçalho: endereços do mapa + procs de teste (gerado por tests/make_keydoor_test.py)
s=(r/'build/keydoor-test.tcl').read_text().split('after time 10 {')[0]
d={n:int(a,16)for a,n in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+',(r/'build/keen_msx.map').read_text(),re.M)}
for n in ['render_count','active_level','scene_ready','world_active','world_x','world_y','defeats']:
    assert n in d,n
s+='\n'+'\n'.join(f'set {n} {d[n]}' for n in ['render_count','active_level','scene_ready','world_active','defeats'] )+'\n'
s+=r'''
set ::bench_out $::env(BENCH_OUT)
proc logline {t} {set f [open $::bench_out a];puts $f $t;close $f}
set phase -1
set ::vbusy 0.0;set ::vt0 0;set ::vcmds 0
debug probe set_bp VDP.commandExecuting {} {
 if {[debug probe read VDP.commandExecuting]} {set ::vt0 [machine_info time];incr ::vcmds} else {set ::vbusy [expr {$::vbusy+[machine_info time]-$::vt0}]}
}
proc motion {} {
 if {$::phase<0 || $::phase>=2} {return}
 set offset [expr {int(([machine_info time]-$::start)*40)%16}]
 if {$offset>8} {set offset [expr {16-$offset}]}
 place $::bx [expr {$::by+$offset}]
 debug write memory $::death_ticks 0
 debug write memory $::combat_dirty 1
 after time 0.02 motion
}
proc next_case {} {
 controls 0
 if {$::phase>=0} {
  set n [expr {[word $::render_count]-$::rc0}]
  set dt [expr {[machine_info time]-$::start}]
  logline "case $::phase renders=$n seconds=[format %.3f $dt] fps=[format %.2f [expr {$n/$dt}]] vdp_busy_ms=[format %.0f [expr {1000*($::vbusy-$::vb0)}]] vdp_cmds=[expr {$::vcmds-$::vc0}] busy_ms_per_render=[format %.1f [expr {$n?1000*($::vbusy-$::vb0)/$n:0}]]"
  if {[info exists ::env(BENCH_SHOT)]} {}
 }
 incr ::phase
 if {$::phase>=2} {exit;return}
 foreach {::bx ::by} [lindex {{784 64} {1240 192}} $::phase] {}
 place $::bx $::by
 after time 1.5 [list begin_case]
}
proc begin_case {} {
 set ::start [machine_info time];set ::rc0 [word $::render_count];set ::vb0 $::vbusy;set ::vc0 $::vcmds
 proc hop {} {if {$::phase<0} {return};keymatrixdown 8 1;after time 0.5 {keymatrixup 8 1};after time 1.3 hop}
 controls 1;hop
 if {[info exists ::env(BENCH_SHOT)]} {after time 3 [list screenshot $::env(BENCH_SHOT)_$::phase.png]}
 after time 8 next_case
}
proc go_level {} {
 if {[byte $::world_active]!=1} {after time 0.2 go_level;return}
 after time 1 {
  putword $::world_x 608;putword $::world_y 448;keymatrixdown 8 1
  after time 0.3 {keymatrixup 8 1;waitfor {[byte $::active_level]==2 && [byte $::scene_ready]==165} lvl_ready}
 }
}
proc lvl_ready {} {after time 2 next_case}
proc press {} {if {[byte $::world_active]==1} {return};keymatrixdown 8 1;after time 0.3 {keymatrixup 8 1};after time 1.5 press}
after time 6 press
go_level
'''
out.write_text(s)
