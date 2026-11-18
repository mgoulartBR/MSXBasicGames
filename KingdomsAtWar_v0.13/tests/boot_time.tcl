# Tempo de carga inicial (segundos emulados ate a simulacao comecar). Historico: 54,5 s antes dos canteiros; 58,0 com o canteiro dos mortos; 60,0 com os dois; 31,5 apos a rotina Z80 de escrita na VRAM, o decodificador LZ mais rapido e os ajustes do carregador.
source kingdom_helpers.tcl
proc fp_case {} {
 delay 5;set t0 [machine_info time]
 while {[w sim_ticks]==0 && [machine_info time]-$t0<150} {delay 0.5}
 puts stderr "INFO boot_seconds=[expr {[machine_info time]-$t0+5}]"
 puts stderr "RESULT failures=0";exit
}
coroutine fp_run guard fp_case
after realtime 300 {puts stderr "FAIL boot_time watchdog";exit}
