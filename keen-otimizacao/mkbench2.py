# uso: mkbench.py <projeto> <saida.tcl>
import sys,re
from pathlib import Path
r=Path(sys.argv[1]);out=Path(sys.argv[2])
# cabeçalho: endereços do mapa + procs de teste (gerado por tests/make_keydoor_test.py)
s=(r/'build/keydoor-test.tcl').read_text().split('after time 10 {')[0]
d={n:int(a,16)for a,n in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+',(r/'build/keen_msx.map').read_text(),re.M)}
for n in ['render_count','active_level','scene_ready','world_active','world_x','world_y','defeats','g_CurrentSegment','DrawScene','ActorsDraw','ActorDraw','HeroBitmap0','HeroBitmap1']:
    assert n in d,n
s+='\n'+'\n'.join(f'set {n} {d[n]}' for n in ['render_count','active_level','scene_ready','world_active','defeats','g_CurrentSegment'] )+'\n'
s+=r'''
set ::bench_out $::env(BENCH_OUT)
proc logline {t} {set f [open $::bench_out a];puts $f $t;close $f}
set phase -1
set ::cur_seg_addr $::g_CurrentSegment
foreach {nm addr seg} {%s} {
 set ::${nm}_n 0;set ::${nm}_t 0.0;set ::${nm}_max 0.0;set ::${nm}_t0 0;set ::${nm}_bp {}
 debug set_bp $addr "\[debug read memory \$::cur_seg_addr\]==$seg" [list prof_enter $nm]
}
proc prof_enter {nm} {
 set sp [reg SP];set ::${nm}_t0 [machine_info time]
 set ::${nm}_bp [debug set_bp [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}] "\[reg SP\]==$sp+2" [list prof_exit $nm]]
}
proc prof_exit {nm} {
 upvar #0 ${nm}_bp bp ${nm}_t0 t0 ${nm}_n n ${nm}_t t ${nm}_max mx
 catch {debug remove_bp $bp}
 set d [expr {[machine_info time]-$t0}];incr n;set t [expr {$t+$d}];if {$d>$mx} {set mx $d}
}
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
  logline "case $::phase renders=$n seconds=[format %.3f $dt] fps=[format %.2f [expr {$n/$dt}]] vdp_busy_ms=[format %.0f [expr {1000*($::vbusy-$::vb0)}]] vdp_cmds=[expr {$::vcmds-$::vc0}]  drawscene_calls=[expr {$::ds_n-$::dn0}] drawscene_mean_ms=[format %.1f [expr {$::ds_n>$::dn0?1000*($::ds_t-$::dt0)/($::ds_n-$::dn0):0}]] drawscene_max_ms=[format %.0f [expr {1000*$::ds_max}]] actor_calls=[expr {$::ac_n-$::cn0}] actor_mean_ms=[format %.2f [expr {$::ac_n>$::cn0?1000*($::ac_t-$::ct0)/($::ac_n-$::cn0):0}]] hero_calls=[expr {$::hb_n+$::h1_n-$::hn0}] hero_mean_ms=[format %.2f [expr {($::hb_n+$::h1_n)>$::hn0?1000*($::hb_t+$::h1_t-$::ht0)/($::hb_n+$::h1_n-$::hn0):0}]] actorsdraw_mean_ms=[format %.1f [expr {$::ad_n>$::an0?1000*($::ad_t-$::at0)/($::ad_n-$::an0):0}]] busy_ms_per_render=[format %.1f [expr {$n?1000*($::vbusy-$::vb0)/$n:0}]]"
  if {[info exists ::env(BENCH_SHOT)]} {}
 }
 incr ::phase
 if {$::phase>=2} {exit;return}
 foreach {::bx ::by} [lindex {{784 64} {1240 192} {1120 1296} {560 1360}} $::phase] {}
 after time 1.5 [list begin_case]
}
proc begin_case {} {
 set ::start [machine_info time];set ::rc0 [word $::render_count];set ::vb0 $::vbusy;set ::dn0 $::ds_n;set ::dt0 $::ds_t;set ::ds_max 0.0;set ::an0 $::ad_n;set ::at0 $::ad_t;set ::cn0 $::ac_n;set ::ct0 $::ac_t;set ::hn0 [expr {$::hb_n+$::h1_n}];set ::ht0 [expr {$::hb_t+$::h1_t}];set ::vc0 $::vcmds
 proc walk {} {if {$::phase<0} {return};set ::dir [expr {1-$::dir}];controls [expr {$::dir?1:2}];after time 1.0 walk}
set ::dir 0
 walk
 if {[info exists ::env(BENCH_SHOT)]} {after time 3 [list screenshot $::env(BENCH_SHOT)_$::phase.png]}
 after time 12 next_case
}
proc go_level {} {
 if {[byte $::world_active]!=1} {after time 0.2 go_level;return}
 after time 1 {
  putword $::world_x [lindex $::env(BENCH_WORLD) 0];putword $::world_y [lindex $::env(BENCH_WORLD) 1];keymatrixdown 8 1
  after time 0.3 {keymatrixup 8 1;waitfor {[byte $::active_level]==$::env(BENCH_LEVEL) && [byte $::scene_ready]==165} lvl_ready}
 }
}
proc lvl_ready {} {after time 30 next_case}
proc press {} {if {[byte $::world_active]==1} {return};keymatrixdown 8 1;after time 0.3 {keymatrixup 8 1};after time 1.5 press}
after time 6 press
go_level
'''
s=s.replace('{%s}','{ds %d %d ad %d %d ac %d %d hb %d %d h1 %d %d}'%tuple(v for k in ('DrawScene','ActorsDraw','ActorDraw','HeroBitmap0','HeroBitmap1') for v in (d[k]&0xFFFF,(d[k]>>16)&0xFF)),1)
out.write_text(s)
