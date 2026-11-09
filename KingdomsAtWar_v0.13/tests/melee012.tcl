# Corpo a corpo parado (sintetico): 8 unidades paradas trocam golpes (gesto de ataque + clarao de dano) a cada 0,6 s, por 12 s.
# O depurador escreve os contadores de golpe e dano; o desenho, a pose e as contas sao os do jogo.
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
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc melee012 {} {
 delay 45;materials
 debug set_watchpoint write_mem $::S(rendered_frames) {$::perf_running} {lappend ::perf_times [machine_info time]}
 set cells {{4 5} {5 6} {7 6} {9 6} {6 4} {8 4}}
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];lassign [lindex $cells $k] cx cy;set a [expr {$::S(units)+16*$i}]
  foreach {o v} [list 0 [expr {$cx*16&255}] 1 [expr {$cx*16>>8}] 2 [expr {$cy*16&255}] 3 [expr {$cy*16>>8}] 4 $cx 5 $cy 6 $cx 7 $cy 8 $cx 9 $cy 10 0 11 0 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
  wr combat_target $i 255;wr unit_build $i 255
 }
 wr unit_count 0 8
 foreach {i kind} {4 4 5 3 6 4 7 5} {wr unit_kind $i $kind;wr unit_hp $i [lindex {32 48 48 32 72 96 48} $kind]}
 wr sprites_dirty 0 3
 delay 2
 proc strike_all {} {for {set i 0} {$i<8} {incr i} {wr attack_flash $i 8;wr hurt_flash $i 8};wr combat_active 0 1;wr sprites_dirty 0 3}
 perf_start;for {set r 0} {$r<10} {incr r} {strike_all;delay 0.6}
 if {[info exists ::env(KAW_SHOT)]} {shot melee-$::env(KAW_SHOT)}
 for {set r 0} {$r<10} {incr r} {strike_all;delay 0.6}
 perf_end melee
 puts stderr "RESULT failures=$::failures";exit
}
coroutine melee012_case guard melee012
after realtime 400 {puts stderr "FAIL melee watchdog";exit}
