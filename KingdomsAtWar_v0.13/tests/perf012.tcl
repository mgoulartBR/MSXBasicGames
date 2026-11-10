source kingdom_helpers.tcl
set perf_running 0
set counts [dict create]
proc perf_start {} {
 set ::perf_times {};set ::counts [dict create];set ::perf_running 1
 set ::start [machine_info time];set ::start_frames [w rendered_frames];set ::start_ticks [w sim_ticks]
}
proc perf_end {phase} {
 set ::perf_running 0
 puts stderr "METRIC $phase seconds=[expr {[machine_info time]-$::start}] frames=[expr {([w rendered_frames]-$::start_frames)&65535}] ticks=[expr {([w sim_ticks]-$::start_ticks)&65535}]"
 puts stderr "CADENCE $phase $::perf_times"
 puts stderr "CALLS $phase $::counts"
}
proc perf012 {} {
 delay 55;choose 2;world_point 0 0
 debug set_watchpoint write_mem $::S(rendered_frames) {$::perf_running} {lappend ::perf_times [machine_info time]}
 set variant $::env(KAW_VARIANT);set name [dict get {V9990 KAW9990 V9990P1 KAW9990P1 PREVIEW KAW68PV V9968 KAW9968} $variant]
 set f [open ../build/$variant/$name.map];set text [read $f];close $f
 foreach name {fog_update fog_stamp fog_merge projectile_tick repair_tick video_sprites} {
  if {[regexp "(\[0-9A-F\]{8})\\s+_$name\\s" $text whole address]} {
   scan $address %x address;set bank [expr {$address>>16}];set pc [expr {$address&65535}]
   set condition [expr {$bank?"\$::perf_running && \[debug read memory 0xC006\]==$bank":"\$::perf_running"}]
   debug set_bp $pc $condition "dict incr ::counts $name"
  }
 }
 perf_start;delay 2;perf_end idle
 perf_start;tap 5 32;await {[unit 2 state]==3 && [unit 2 x]==0 && [unit 2 y]==0} 120;perf_end one
 ck one_unit_arrives {[unit 2 x]==0 && [unit 2 y]==0}
 tap 2 64;world_point 0 0;ck four_selected {[b selection_mask]==15}
 perf_start;tap 5 32
 await {[unit 0 state]==3 && [unit 1 state]==3 && [unit 2 state]==3 && [unit 3 state]==3 && [unit 0 progress]==0 && [unit 1 progress]==0 && [unit 2 progress]==0 && [unit 3 progress]==0} 120
 perf_end four
 set goals {};for {set i 0} {$i<4} {incr i} {lappend goals [list [unit $i x] [unit $i y]]}
 ck four_distinct_goals {[llength [lsort -unique $goals]]==4}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine perf012_case guard perf012
after realtime 300 {puts stderr "FAIL perf watchdog";exit}
