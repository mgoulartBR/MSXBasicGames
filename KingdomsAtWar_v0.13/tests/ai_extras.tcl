# IA com quatro soldados: os reforcos leves (atores 10 e 11) sao treinados, esperam no ponto de reuniao e marcham na onda.
set ::env(KAW_AI) 1
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc ax_case {} {
 delay 45
 set limit [expr {[machine_info time]+400}]
 while {[machine_info time]<$limit && ([b ai_step]<8 || [b ai_site]!=255)} {delay 1}
 ck expanded {[b ai_step]==8}
 await {[b unit_hp 10]>0 && [b unit_hp 11]>0 && [b ai_state 2]==0 && [b ai_state 3]==0} 150
 puts stderr "INFO extras kinds=[b unit_kind 10],[b unit_kind 11] hp=[b unit_hp 10],[b unit_hp 11] pos=[unit 10 x],[unit 10 y]/[unit 11 x],[unit 11 y] gold=[w ai_gold] wood=[w ai_wood] t=[w game_seconds]"
 ck extras_trained {[b unit_hp 10]>0 && [b unit_hp 11]>0}
 ck extras_light_kinds {([b unit_kind 10]==1 || [b unit_kind 10]==3) && ([b unit_kind 11]==1 || [b unit_kind 11]==3)}
 ck extras_at_muster {[unit 10 y]==23 && [unit 11 y]==23}
 wr ai_launch 0 0;wr ai_launch 1 0
 await {[b ai_wave]==1} 30
 ck four_in_the_wave {[b ai_state 0]==1 && [b ai_state 1]==1 && [b ai_state 2]==1 && [b ai_state 3]==1}
 await {[unit 10 y]<20 && [unit 11 y]<20} 90
 puts stderr "INFO march pos=[unit 8 x],[unit 8 y] [unit 9 x],[unit 9 y] [unit 10 x],[unit 10 y] [unit 11 x],[unit 11 y]"
 ck extras_march {[unit 10 y]<20 && [unit 11 y]<20}
 # morte de um reforco: volta a treinar (ainda leve)
 wr unit_hp 10 0
 await {[b ai_state 2]>=2} 10
 await {[b unit_hp 10]>0} 120
 ck extra_retrained {[b unit_hp 10]>0 && ([b unit_kind 10]==1 || [b unit_kind 10]==3)}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine ax_run guard ax_case
after realtime 900 {puts stderr "FAIL ax watchdog";exit}
