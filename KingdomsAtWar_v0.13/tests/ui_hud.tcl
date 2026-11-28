# HUD de comandos com icones: capturas por contexto (campones, soldado, castelo) e dica ao passar o ponteiro. KAW_MENU=1 KAW_OBJ=1.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc mouse_to {x y} {
 debug write memory $::S(mouse_x) $x
 debug write memory $::S(mouse_y) $y
}
proc click {x y} {mouse_to $x $y;delay 0.3;keymatrixdown 5 128;delay 0.3;keymatrixup 5 128;delay 0.6}
proc sel {mask count selected bsel} {
 debug write memory $::S(selection_mask) $mask;debug write memory $::S(selection_count) $count
 debug write memory $::S(selected) $selected;debug write memory $::S(building_selected) $bsel
 debug write memory $::S(hud_dirty) 3
}
proc hud_case {} {
 delay 38
 tapkey 7 128;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 delay 2
 tapkey 7 8  ;# esconde o minimapa
 shot hud-none
 # campones (unidade 0)
 sel 1 1 0 0
 ck worker_selected {[b selection_mask]==1}
 mouse_to 100 100;delay 0.5
 shot hud-worker
 mouse_to 40 199;delay 0.6;shot hud-worker-tip-barracks
 # soldado (unidade 2)
 sel 4 1 2 0
 ck soldier_selected {[b selection_mask]==4}
 mouse_to 100 100;delay 0.5
 shot hud-soldier
 mouse_to 40 199;delay 0.6;shot hud-soldier-tip-hold
 # castelo
 sel 0 0 255 1
 ck castle_selected {[b building_selected]==1}
 mouse_to 100 100;delay 0.5
 shot hud-castle
 mouse_to 16 199;delay 0.6;shot hud-castle-tip
 # clique no icone do camponês: treina (ouro 40 -> 32)
 debug write memory [expr {$::S(unit_hp)+3}] 0   ;# um soldado a menos: ha vaga
 set g0 [w gold]
 for {set try 0} {$try<4 && [w gold]>=$g0} {incr try} {click 16 199}
 puts stderr "INFO train: gold $g0 -> [w gold] bsel=[b building_selected] st=[bd 0 state] tr=[bd 0 training] pop=[b unit_count] left=[b left_x],[b left_y] mm=[b hover_slot] hp3=[b unit_hp 3]"
 ck castle_trains_peasant {[w gold]<$g0}
 delay 1;shot hud-castle-training
 puts stderr "RESULT failures=$::failures";exit
}
coroutine hud_run guard hud_case
after realtime 300 {puts stderr "FAIL hud watchdog";exit}
