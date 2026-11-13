# IA inimiga: onda 1 -> baixas -> treino e retorno com a composicao da onda 2 -> segunda onda.
set ::env(KAW_AI) 1
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc aw_case {} {
 delay 55
 set limit [expr {[machine_info time]+400}]
 while {[machine_info time]<$limit && ([b ai_step]<8 || [b ai_site]!=255)} {delay 1}
 ck expanded {[b ai_step]==8 && [b ai_site]==255}
 # onda 1 (os dois guardas esqueleto)
 wr ai_launch 0 0;wr ai_launch 1 0
 await {[b ai_wave]==1} 20
 ck wave1_launched {[b ai_state]==1 && [b ai_state 1]==1 && [b unit_kind 8]==6}
 # baixas
 wr unit_hp 8 0;wr unit_hp 9 0
 await {[b ai_state]>=2 && [b ai_state 1]>=2} 10
 ck casualties_detected {[b ai_state]>=2 && [b ai_state 1]>=2}
 await {[b unit_hp 8]>0 && [b unit_hp 9]>0 && [b ai_state]==0 && [b ai_state 1]==0} 120
 puts stderr "INFO respawn kinds=[b unit_kind 8],[b unit_kind 9] pos=[unit 8 x],[unit 8 y] / [unit 9 x],[unit 9 y] wood=[w ai_wood] gold=[w ai_gold]"
 ck respawn_wave2 {[b unit_kind 8]==2 && [b unit_kind 9]==3 && [unit 8 y]==23 && [unit 9 y]==23}
 await {[b ai_wave]==2} 120
 ck wave2_launched {[b ai_wave]==2 && [b ai_state]==1}
 # segundo ciclo: cavaleiro + espadachim
 wr unit_hp 8 0;wr unit_hp 9 0
 await {[b ai_state]>=2 && [b ai_state 1]>=2} 10
 await {[b unit_hp 8]>0 && [b unit_hp 9]>0 && [b ai_state]==0 && [b ai_state 1]==0} 200
 ck respawn_wave3 {[b unit_kind 8]==4 && [b unit_kind 9]==1}
 await {[b ai_wave]==3} 120
 ck wave3_launched {[b ai_wave]==3}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine aw_run guard aw_case
after realtime 1200 {puts stderr "FAIL aw watchdog";exit}
