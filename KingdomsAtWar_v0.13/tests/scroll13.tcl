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
 delay 45;choose 2;world_point 0 0
 camera_until cam_x 0 32
 perf_start;keymatrixdown 8 0x80;delay 2.5;keymatrixup 8 0x80;perf_end idle
 puts stderr "CAM [w cam_x],[w cam_y]"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine perf012_case guard perf012
after realtime 300 {puts stderr "FAIL perf watchdog";exit}
