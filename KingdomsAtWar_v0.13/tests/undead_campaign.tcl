# Campanha dos Insepultos: o menu escolhe o lado, a missao 1 vem da tabela deles, as unidades e os predios do jogador usam a arte dos mortos
# e a IA joga com a arte dos humanos. Rodar com KAW_MENU=1.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc uc_case {} {
 delay 38
 tapkey 7 128                       ;# titulo -> menu
 tapkey 7 128                       ;# NOVO JOGO -> escolha do lado
 shot undead-side-menu
 tapkey 8 64                        ;# INSEPULTOS
 tapkey 7 128
 ck side_undead {[b side]==1}
 delay 6;shot undead-briefing1
 tapkey 7 128;wait_game
 # missao 1 dos Insepultos: 60 de madeira, 40 de ouro, objetivo CONSTRUIR (4), mapa 64x64 (indice 1)
 ck mission1_params {[w wood]==60 && [w gold]==40 && [b mis 7]==4 && [b map_w]==64 && [b mission_n]==0}
 # a arte: o campones (unidade 0) usa o tipo de pose 7 (campones-mumia) e a unidade inimiga 8 o tipo 6->1 (espadachim humano)
 puts stderr "INFO pose keys: unit0=[debug read memory [expr {$::S(actor_key)+0}]] unit8=[debug read memory [expr {$::S(actor_key)+8}]]"
 delay 2;shot undead-game1
 # a primeira onda da IA (humanos) chega a base: guerreiros humanos contra os mortos
 debug write memory $::S(ai_launch) 0;debug write memory [expr {$::S(ai_launch)+1}] 0
 set t0 [machine_info time]
 while {[machine_info time]-$t0<120 && (abs([unit 8 x]-[b pb_x])+abs([unit 8 y]-[b pb_y]))>9} {delay 1}
 ck wave_arrives {(abs([unit 8 x]-[b pb_x])+abs([unit 8 y]-[b pb_y]))<=9}
 delay 1;shot undead-wave-arrives
 # castelo inimigo (humano) visivel: um soldado e posto junto dele (a visao revela o castelo e a IA)
 set ex [b eb_x];set ey [b eb_y]
 set u [expr {$::S(units)+16*2}];set sx [expr {$ex+2}];set sy [expr {$ey+5}]
 debug write memory [expr {$u+4}] $sx;debug write memory [expr {$u+5}] $sy;debug write memory [expr {$u+6}] $sx;debug write memory [expr {$u+7}] $sy;debug write memory [expr {$u+8}] $sx;debug write memory [expr {$u+9}] $sy
 debug write memory $u [expr {($sx*16)&255}];debug write memory [expr {$u+1}] [expr {($sx*16)>>8}];debug write memory [expr {$u+2}] [expr {($sy*16)&255}];debug write memory [expr {$u+3}] [expr {($sy*16)>>8}]
 debug write memory $::S(fog_refresh) 1;delay 4
 debug write memory [expr {$::S(cam_x)}] [expr {(($ex-6)*16)&255}];debug write memory [expr {$::S(cam_x)+1}] [expr {(($ex-6)*16)>>8}]
 debug write memory [expr {$::S(cam_y)}] [expr {(($ey-2)*16)&255}];debug write memory [expr {$::S(cam_y)+1}] [expr {(($ey-2)*16)>>8}]
 delay 1;shot undead-enemy-keep
 # icones do HUD dos mortos: selecao do castelo
 debug write memory $::S(building_selected) 1;debug write memory $::S(selected) 255;debug write memory $::S(hud_dirty) 3
 delay 1;shot undead-castle-hud
 puts stderr "RESULT failures=$::failures";exit
}
coroutine uc_run guard uc_case
after realtime 300 {puts stderr "FAIL undead watchdog";exit}
