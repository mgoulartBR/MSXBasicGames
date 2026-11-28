# Campanha: NOVO JOGO -> briefing da missao 1 -> jogo (recursos da missao) -> vitoria -> missao 2 -> ... -> fim da campanha -> titulo.
# Rodar com KAW_MENU=1 KAW_OBJ=1 (menu e objetivos reais).
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc quick_win {} {debug write memory [expr {$::S(mis)+7}] 1;debug write memory [expr {$::S(mis)+8}] 3;debug write memory [expr {$::S(mis)+9}] 0}
# a carga de uma missao (mundo, fog, HUD) leva alguns segundos emulados
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc cf_case {} {
 delay 38
 tapkey 7 128;tapkey 7 128;tapkey 7 128;delay 6;shot camp-brief1
 tapkey 7 128;wait_game
 ck mission1_started {[b mission_n]==0 && [b map_w]==64 && [w wood]==60 && [w gold]==40 && [b game_over]==0}
 ck mission1_objective_build {[b mis 7]==4}
 quick_win;await {[b game_over]==1} 15
 delay 4;shot camp-victory1
 tapkey 7 128;delay 6;shot camp-brief2
 ck mission2_briefing {[b mission_n]==1}
 tapkey 7 128;wait_game
 ck mission2_started {[b mission_n]==1 && [w wood]==40 && [w gold]==30 && [b game_over]==0 && [w game_seconds]<10 && [b mis 7]==1}
 shot camp-game2
 # ultima missao: vitoria leva ao fim da campanha e ao titulo
 debug write memory $::S(mission_n) 7;quick_win;await {[b game_over]==1} 15
 delay 4;tapkey 7 128;delay 5;shot camp-end
 tapkey 7 128;delay 3;shot camp-back-to-title
 ck back_to_start {[b mission_n]==0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine cf_run guard cf_case
after realtime 500 {puts stderr "FAIL campaign watchdog";exit}
