# Entrada e saida da mina: o trabalhador some pela porta (linhas de cima cortadas). Captura um quadro por passo.
source helpers011.tcl
set ::cap 0
proc mine_watch {} {
 set ph [b mine_phase 1]
 if {($ph==1 || $ph==3) && $::cap<60} {
  incr ::cap
  screenshot -raw -doublesize ../screenshots/$::env(KAW_VARIANT)/minecrop-[format %02d $::cap]-p${ph}-s[b mine_step 1].png
 }
}
proc mine_case {} {
 delay 55;materials
 debug set_watchpoint write_mem $::S(rendered_frames) {} mine_watch
 choose 1;world_point 9 7;tap 5 32
 await {[b mine_phase 1]==1} 60
 await {[b mine_phase 1]==2} 20
 ck worker_hidden_inside {[b mine_phase 1]==2}
 await {[b mine_phase 1]==3} 120
 await {[b mine_phase 1]==0} 20
 ck worker_out {[b mine_phase 1]==0}
 ck captured_steps {$::cap>=7}
 puts stderr "CAPTURES $::cap"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine mine_run guard mine_case
after realtime 300 {puts stderr "FAIL mine watchdog";exit}
