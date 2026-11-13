# Desempenho com a IA inimiga: 8 unidades humanas na base, base inimiga em obra (ai_idle), onda de dois soldados
# marchando (march) e a luta na base com defesa automatica (fight). Rodar com KAW_AI=1 KAW_DEF=1.
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
proc wave_perf {} {
 delay 65;materials
 debug set_watchpoint write_mem $::S(rendered_frames) {$::perf_running} {lappend ::perf_times [machine_info time]}
 set cells {{4 5} {5 6} {7 6} {9 6}}
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];lassign [lindex $cells $k] cx cy;set a [expr {$::S(units)+16*$i}]
  foreach {o v} [list 0 [expr {$cx*16&255}] 1 [expr {$cx*16>>8}] 2 [expr {$cy*16&255}] 3 [expr {$cy*16>>8}] 4 $cx 5 $cy 6 $cx 7 $cy 8 $cx 9 $cy 10 0 11 0 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
  debug write memory [expr {$::S(combat_target)+$i}] 255;debug write memory [expr {$::S(unit_build)+$i}] 255
 }
 debug write memory $::S(unit_count) 8
 foreach {i kind} {0 4 1 3 2 4 3 5 4 4 5 3 6 4 7 5} {debug write memory [expr {$::S(unit_kind)+$i}] $kind;debug write memory [expr {$::S(unit_hp)+$i}] [lindex {32 48 48 32 72 96 48} $kind]}
 debug write memory $::S(sprites_dirty) 3;debug write memory $::S(fog_refresh) 1
 view_y 60;delay 2
 perf_start;delay 2;perf_end idle
 # onda forte: cavaleiro e arqueiro saem agora
  debug write memory [expr {$::S(unit_kind)+8}] 4;debug write memory [expr {$::S(unit_hp)+8}] 72
 debug write memory [expr {$::S(unit_kind)+9}] 3;debug write memory [expr {$::S(unit_hp)+9}] 32
 poke16 ai_launch 0
 # marcha sem medir ate o primeiro contato (inimigo a menos de 16 linhas)
 for {set n 0} {$n<40 && [unit 8 y]>=14} {incr n} {delay 1}
 perf_start;delay 6;perf_end melee
 if {[info exists ::env(KAW_SHOT)]} {shot wave-melee-$::env(KAW_SHOT)}
 perf_start;delay 10;perf_end late
 puts stderr "INFO wave=[b ai_wave] enemy hp=[b unit_hp 8],[b unit_hp 9] pos=[unit 8 x],[unit 8 y]/[unit 9 x],[unit 9 y] kills=[w combat_kills]"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine wp_run guard wave_perf
after realtime 900 {puts stderr "FAIL wave_perf watchdog";exit}
