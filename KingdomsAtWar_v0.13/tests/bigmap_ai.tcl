# IA inimiga em mapa grande (missao 2, 64x64): a onda segue a rota de waypoints ate a base humana e a IA constroi na base inimiga.
# Rodar com KAW_MENU=1 KAW_OBJ=1.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc quick_win {} {debug write memory [expr {$::S(mis)+7}] 1;debug write memory [expr {$::S(mis)+8}] 3;debug write memory [expr {$::S(mis)+9}] 0}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc dist8 {} {return [expr {abs([unit 8 x]-[b pb_x])+abs([unit 8 y]-[b pb_y])}]}
proc ba_case {} {
 delay 38
 tapkey 7 128;tapkey 7 128;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 quick_win;await {[b game_over]==1} 15
 delay 4;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 # a missao 2 nao termina por tempo curto: restaura o objetivo (sobreviver 360 s)
 debug write memory [expr {$::S(mis)+8}] 104;debug write memory [expr {$::S(mis)+9}] 1
 ck ai_on {[b ai_enabled]==1 && [b eb_x]==46 && [b eb_y]==18 && [b ai_route_n]>0}
 ck enemy_keep_placed {[bd 1 x]==46 && [bd 1 y]==18 && [b unit_hp 8]>0}
 ck enemy_spawn_near_keep {[unit 8 x]==46 && [unit 8 y]==14}
 # a IA constroi: espera o primeiro predio inimigo
 set t0 [machine_info time]
 while {[b building_count]<3 && [machine_info time]-$t0<120} {delay 1}
 delay 3
 puts stderr "INFO ai buildings=[b building_count] step=[b ai_step] seconds=[w game_seconds]"
 set found 0;for {set k 2} {$k<[b building_count]} {incr k} {if {[b building_owner $k] && [bd $k state]>0 && abs([bd $k x]-46)<=10 && abs([bd $k y]-18)<=14} {set found 1}}
 ck ai_builds_at_keep {$found==1}
 # lanca a onda ja: os dois soldados pesados seguem a rota
 debug write memory $::S(ai_launch) 0;debug write memory [expr {$::S(ai_launch)+1}] 0
 set d0 [dist8];set t0 [machine_info time];set best $d0
 while {[machine_info time]-$t0<220 && [dist8]>16 && [b unit_hp 8]>0} {delay 2;set d [dist8];if {$d<$best} {set best $d}}
 puts stderr "INFO wave unit8 at [unit 8 x],[unit 8 y] dist $d0 -> [dist8] best $best hp=[b unit_hp 8] after [expr {[machine_info time]-$t0}]s"
 ck wave_marches_to_base {$best<=16 || [b unit_hp 8]==0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine ba_run guard ba_case
after realtime 900 {puts stderr "FAIL bigmap_ai watchdog";exit}
