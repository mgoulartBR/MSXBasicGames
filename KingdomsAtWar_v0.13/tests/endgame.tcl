# Vitoria (castelo inimigo cai), derrota (castelo humano cai), prioridade da derrota e congelamento da simulacao.
set ::env(KAW_END) 1
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc hud_line {} {set s "";for {set i 0} {$i<29} {incr i} {append s [format %c [b hud_context [expr {$i}]]]};return $s}
proc eg_case {} {
 delay 65
 ck playing {[b game_over]==0}
 # vitoria: o castelo inimigo (predio 1) e destruido
 wr building_hp 1 0;wr buildings [expr {12*1+1}] 0
 await {[b game_over]!=0} 5
 ck victory {[b game_over]==1}
 set t1 [w sim_ticks];set s1 [w game_seconds];delay 3
 ck simulation_frozen {[w sim_ticks]==$t1 && [w game_seconds]==$s1}
 delay 1;shot endgame-victory
 puts stderr "INFO hud=[hud_line]"
 # derrota: castelo humano destruido, o inimigo restaurado (e a derrota vence o empate)
 wr game_over 0 0;wr building_hp 0 0;wr buildings 1 0
 await {[b game_over]!=0} 5
 ck defeat_wins_tie {[b game_over]==2}
 delay 1;shot endgame-defeat
 # sem fim: com end_enabled=0 nada acontece
 wr game_over 0 0;wr end_enabled 0 0;delay 3
 ck disabled_does_nothing {[b game_over]==0}
 # reinicio: derrota de novo, Enter reinicia o jogo do zero
 wr end_enabled 0 1;wr game_over 0 2
 delay 1;puts stderr "INFO hud=[hud_line]"
 wr building_hp 0 0;wr buildings 1 0
 await {[b game_over]==2} 5
 set sec [w game_seconds]
 keymatrixdown 7 128;delay 0.5;delay 0.5;keymatrixup 7 128
 # a carga inicial leva ~45 s emulados
 await {[b video_enabled]==0} 20
 await {[b video_enabled]==1 && [w sim_ticks]>100} 100
 ck restarted {[b game_over]==0 && [w game_seconds]<20}
 ck new_game_running {[w sim_ticks]>100 && [b building_hp 0]==240 && [b building_hp 1]==180 && [bd 0 state]==2 && [bd 1 state]==2 && [b game_over]==0}
 shot endgame-restarted
 puts stderr "RESULT failures=$::failures";exit
}
coroutine eg_run guard eg_case
after realtime 400 {puts stderr "FAIL eg watchdog";exit}
