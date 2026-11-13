# Tempo de carga inicial (segundos emulados ate a simulacao comecar). ROM anterior ao canteiro: 54,5 s; com o canteiro dos mortos: 58,0 s.
source kingdom_helpers.tcl
proc fp_case {} {
 delay 5;set t0 [machine_info time]
 while {[w sim_ticks]==0 && [machine_info time]-$t0<150} {delay 0.5}
 puts stderr "INFO boot_seconds=[expr {[machine_info time]-$t0+5}]"
 puts stderr "RESULT failures=0";exit
}
coroutine fp_run guard fp_case
after realtime 300 {puts stderr "FAIL boot_time watchdog";exit}
