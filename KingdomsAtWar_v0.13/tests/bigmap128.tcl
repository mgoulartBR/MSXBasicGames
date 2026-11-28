# IA inimiga em mapa grande (missao 4, 128x128): a onda segue a rota de waypoints ate a base humana e a IA constroi na base inimiga.
# Rodar com KAW_MENU=1 KAW_OBJ=1.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc quick_win {} {debug write memory [expr {$::S(mis)+7}] 1;debug write memory [expr {$::S(mis)+8}] 3;debug write memory [expr {$::S(mis)+9}] 0}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc dist8 {} {return [expr {abs([unit 8 x]-[b pb_x])+abs([unit 8 y]-[b pb_y])}]}
proc ba_case {} {
 delay 38
 tapkey 7 128;tapkey 7 128;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 for {set m 0} {$m<4} {incr m} {
  quick_win;await {[b game_over]==1} 15
  delay 4;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 }
 debug write memory [expr {$::S(mis)+8}] 104;debug write memory [expr {$::S(mis)+9}] 1
 ck map128 {[b map_w]==128 && [b map_h]==128 && [b mission_n]==4}
 puts stderr "INFO base=[b pb_x],[b pb_y] enemy=[b eb_x],[b eb_y] route=[b ai_route_n] cam=[w cam_x],[w cam_y] fps_ticks=[w sim_ticks]"
 ck ai_on {[b ai_enabled]==1 && [b ai_route_n]>0}
 ck camera_at_base {[w cam_y]>=([b pb_y]-8)*16-16 && [w cam_x]<=([b pb_x]*16)}
 ck enemy_keep {[bd 1 x]==[b eb_x] && [bd 1 y]==[b eb_y] && [b unit_hp 8]>0}
 shot bigmap128-start
 set t0 [w sim_ticks];set s0 [machine_info time];delay 20
 puts stderr "INFO ticks/s [expr {([w sim_ticks]-$t0)/([machine_info time]-$s0)}]"
 debug write memory $::S(ai_launch) 0;debug write memory [expr {$::S(ai_launch)+1}] 0
 set d0 [dist8];set t0 [machine_info time];set best $d0
 while {[machine_info time]-$t0<400 && [dist8]>16 && [b unit_hp 8]>0} {delay 2;set d [dist8];if {$d<$best} {set best $d}}
 puts stderr "INFO wave unit8 at [unit 8 x],[unit 8 y] dist $d0 -> [dist8] best $best hp=[b unit_hp 8] after [expr {[machine_info time]-$t0}]s"
 ck wave_marches_to_base {$best<=16 || [b unit_hp 8]==0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine ba_run guard ba_case
after realtime 1300 {puts stderr "FAIL bigmap128 watchdog";exit}
