# Grava cada musica e efeito em WAV (openMSX, audio emulado) para audicao. Saida em ../musica/.
source helpers011.tcl
proc play_stream {bank} {
 set a $::S(ms)
 debug write memory $a 0;debug write memory [expr {$a+1}] 0;debug write memory [expr {$a+2}] $bank
 debug write memory [expr {$a+3}] $bank;debug write memory [expr {$a+4}] 0;debug write memory [expr {$a+5}] 0xA0
 debug write memory [expr {$a+6}] 0;debug write memory [expr {$a+7}] 0xA0;debug write memory $a 1
}
proc rec {name secs} {
 record start -audioonly ../../musica/$name.wav
 delay $secs
 record stop
}
proc mr_case {} {
 delay 36
 set songs [list [list titulo 63 50] [list calma 64 50] [list batalha 65 26] [list vitoria 66 6] [list derrota 67 17] [list fim_da_campanha 68 20]]
 foreach s $songs {
  lassign $s name bank secs
  # silencia, reinicia
  debug write memory $::S(ms) 0
  delay 0.5
  play_stream $bank
  # os fluxos que nao repetem terminam sozinhos
  rec $name $secs
 }
 # efeitos em sequencia (1,2 s cada), na ordem de tools/music.py
 debug write memory $::S(ms) 0
 set tbl [regexp -inline {sfx_ptr\[\]=\{([0-9,]+)\}} [exec cat ../src/music_data.h]]
 set ptrs [split [lindex $tbl 1] ,];set bank [lindex [regexp -inline {SFX_BANK ([0-9]+)} [exec cat ../src/music_data.h]] 1]
 record start -audioonly ../../musica/efeitos.wav
 foreach p $ptrs {
  set a $::S(sx)
  debug write memory $a 0;debug write memory [expr {$a+1}] 0;debug write memory [expr {$a+2}] $bank
  debug write memory [expr {$a+4}] [expr {$p&255}];debug write memory [expr {$a+5}] [expr {$p>>8}];debug write memory $a 1
  delay 1.2
 }
 record stop
 puts stderr "RESULT failures=0";exit
}
coroutine mr_run guard mr_case
after realtime 1100 {puts stderr "FAIL render watchdog";exit}
