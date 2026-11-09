# Cenarios pesados de desempenho (somente medicao): 8 unidades grandes andando e luta em grupo.
# O depurador escreve ouro/madeira, o tipo das unidades e o foco da luta; o resto usa os controles normais.
source helpers011.tcl
set perf_running 0
proc perf_start {} {set ::perf_times {};set ::perf_running 1;
 set ::start [machine_info time];set ::start_frames [w rendered_frames]}
proc perf_end {phase} {
 set ::perf_running 0
 puts stderr "METRIC $phase seconds=[expr {[machine_info time]-$::start}] frames=[expr {([w rendered_frames]-$::start_frames)&65535}]"
 puts stderr "CADENCE $phase $::perf_times"
 puts stderr "CALLS $phase"
}
proc poke16 {name v} {debug write memory $::S($name) [expr {$v&255}];debug write memory [expr {$::S($name)+1}] [expr {$v>>8}]}
proc heavy012 {} {
 delay 45;materials
 debug set_watchpoint write_mem $::S(rendered_frames) {$::perf_running} {lappend ::perf_times [machine_info time]}
 perf_start;delay 2;perf_end idle
 set cells {{4 5} {5 6} {7 6} {9 6}}
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];lassign [lindex $cells $k] cx cy;set a [expr {$::S(units)+16*$i}]
  foreach {o v} [list 0 [expr {$cx*16&255}] 1 [expr {$cx*16>>8}] 2 [expr {$cy*16&255}] 3 [expr {$cy*16>>8}] 4 $cx 5 $cy 6 $cx 7 $cy 8 $cx 9 $cy 10 0 11 0 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
  debug write memory [expr {$::S(combat_target)+$i}] 255;debug write memory [expr {$::S(unit_build)+$i}] 255
 }
 debug write memory $::S(unit_count) 8
 foreach {i kind} {0 4 1 3 2 4 3 5 4 4 5 3 6 4 7 5} {debug write memory [expr {$::S(unit_kind)+$i}] $kind;debug write memory [expr {$::S(unit_hp)+$i}] 40}
 ck eight_units {[b unit_count]==8}
 delay 1
 # todos selecionados caminham para o canto: 8 atores grandes, Fog mudando
 tap 2 64;world_point 1 3
 perf_start;tap 5 32;delay 14;perf_end eight
 ck eight_moved {[unit 0 x]<8}
 # luta: grupo ataca o defensor inimigo (unidade 8) e depois a fortaleza
 tap 2 64;view_y [unit 8 py];point [expr {[unit 8 px]+8-[w cam_x]}] [expr {[unit 8 py]-6-[w cam_y]}]
 if {[info exists ::env(KAW_DRAW)]} {debug write memory $::S(draw_opt) $::env(KAW_DRAW)}
 perf_start;tap 5 32;delay 7
 if {[info exists ::env(KAW_SHOT)]} {view_y [expr {[bd 1 y]*16-40}];delay 1;shot $::env(KAW_SHOT)}
 delay 13;perf_end fight
 puts stderr "RESULT failures=$::failures";exit
}
coroutine heavy012_case guard heavy012
after realtime 400 {puts stderr "FAIL heavy watchdog";exit}
