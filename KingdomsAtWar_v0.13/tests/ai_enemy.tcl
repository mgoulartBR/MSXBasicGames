# IA inimiga: expansao (predios), treino e onda de ataque ate a base humana.
set ::env(KAW_AI) 1
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc ai_case {} {
 delay 55
 ck ai_start {[b ai_wave]==0 && [b unit_kind 8]==6}
 # acelera o relogio do jogo: 120 s de simulacao em tempo curto (os ticks continuam normais)
 set t0 [w game_seconds]
 await {[w game_seconds]>=$t0+4} 40
 ck ai_economy {[w ai_wood]!=60 || [b ai_step]>0}
 # espera a expansao completa (7 predios) rodando o jogo em velocidade maxima
 set limit [expr {[machine_info time]+360}]
 while {[machine_info time]<$limit && ([b ai_step]<8 || [b ai_site]!=255)} {delay 1}
 puts stderr "INFO seconds=[w game_seconds] step=[b ai_step] wood=[w ai_wood] gold=[w ai_gold] count=[b building_count]"
 set types {}
 for {set k 1} {$k<[b building_count]} {incr k} {if {[b building_owner $k]} {lappend types "$k:[bd $k type]/[bd $k state]"}}
 puts stderr "INFO enemy_buildings $types"
 ck ai_expanded {[b ai_step]==8}
 # mostra a base inimiga (revela com aliados) e captura
 set cells {{4 24} {9 24} {2 28} {9 29}}
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];lassign [lindex $cells $k] cx cy;set a [expr {$::S(units)+16*$i}]
  foreach {o v} [list 0 [expr {$cx*16&255}] 1 [expr {$cx*16>>8}] 2 [expr {$cy*16&255}] 3 [expr {$cy*16>>8}] 4 $cx 5 $cy 6 $cx 7 $cy 8 $cx 9 $cy 10 0 11 0 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
  wr combat_target $i 255;wr unit_build $i 255;wr unit_kind $i 1;wr unit_hp $i 48
 }
 wr unit_count 0 8;wr sprites_dirty 0 3;wr fog_refresh 0 1
 view_y [expr {19*16+72}];delay 3
 shot ai-base
 # aliados de volta perto da base humana (alvos da onda)
 set home {{4 12} {6 12} {4 13} {6 13}}
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];lassign [lindex $home $k] cx cy;set a [expr {$::S(units)+16*$i}]
  foreach {o v} [list 0 [expr {$cx*16&255}] 1 [expr {$cx*16>>8}] 2 [expr {$cy*16&255}] 3 [expr {$cy*16>>8}] 4 $cx 5 $cy 6 $cx 7 $cy 8 $cx 9 $cy] {debug write memory [expr {$a+$o}] $v}
 }
 wr sprites_dirty 0 3
 # forca a onda: lanca ja e acompanha ate chegar na base humana
 wr ai_launch 0 0;wr ai_launch 1 0
 set limit [expr {[machine_info time]+120}]
 while {[machine_info time]<$limit && [b ai_wave]==0} {delay 1}
 ck ai_wave_launched {[b ai_wave]>=1}
 for {set n 0} {$n<9 && [unit 8 y]>=16 && [unit 9 y]>=16} {incr n} {delay 10;puts stderr "INFO t=[w game_seconds] u8=[unit 8 x],[unit 8 y] st=[unit 8 state] hp=[b unit_hp 8] tgt=[b combat_target 8] u9=[unit 9 x],[unit 9 y] hp=[b unit_hp 9] tgt=[b combat_target 9] aist=[b ai_state],[b ai_state 1] stuck=[b ai_stuck],[b ai_stuck 1] wave=[b ai_wave]"}
 view_y [unit 8 py];delay 8;shot ai-march
 puts stderr "INFO wave pos=[unit 8 x],[unit 8 y] / [unit 9 x],[unit 9 y] state=[b ai_state]"
 ck ai_marching {[unit 8 y]<20 || [unit 9 y]<20}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine ai_run guard ai_case
after realtime 900 {puts stderr "FAIL ai watchdog";exit}
