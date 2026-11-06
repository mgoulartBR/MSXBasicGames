source helpers011.tcl
proc defense011 {} {
 delay 45;materials;fund 128 32
 ck collected_for_realm {[w wood]>=128 && [w gold]>=32 && [conserved]}
 move_unit 0 1 24
 move_unit 2 1 22
 set damaged [build 1 3 24]
 await {[b building_hp $damaged]<96} 30
 ck defender_damages_structure {[bd $damaged state]==2 && [b building_hp $damaged]>0}
 set before [w wood]
 tap 5 32
 ck repair_order_assigned {[b repair_target]==$damaged}
 shot repair011-damaged
 await {[w repair_spent]>=2} 30
 ck repair_uses_wood {[w wood]<=$before-1 && [w repair_healed]>=8 && [conserved]}
 # repair_tick limpa e reativa repair_active a cada tick; amostrar no meio dele da falso negativo.
 set seen 0;for {set k 0} {$k<60 && !$seen} {incr k} {if {([b repair_active]&1)!=0} {set seen 1} else {delay 0.011}}
 ck repair_animates {$seen}
 shot repair011-working
 tap 2 64
 view_y [unit 8 py];point [expr {[unit 8 px]+8-[w cam_x]}] [expr {[unit 8 py]-6-[w cam_y]}];tap 5 32
 ck attack_group_preserves_repair {[b repair_target]==$damaged && [b combat_target 2]==8}
 await {[b unit_hp 8]==0} 140
 await {[b building_hp $damaged]==96 && [b repair_target]==255} 80
 ck repair_completes {[b building_hp $damaged]==96 && [conserved]}
 shot repair011-complete
 move_unit 2 2 23

 set saw [build 3 4 14]
 move_unit 0 11 25;move_unit 0 11 20;move_unit 0 3 20
 set tower [build 6 4 19]
 ck tower_ready {[bd $tower state]==2 && [fog_state 8 23]==2 && [conserved]}
 await {[w tower_shots]>0 && [b unit_hp 9]<48} 30
 ck tower_auto_acquires {[w tower_shots]>0 && [b unit_hp 9]<48}
 shot tower011-firing
 await {[b unit_hp 9]==0} 40
 ck tower_defeats_defender {[w combat_kills]==2 && [b building_hp $tower]>0 && [conserved]}
 ck no_hud_clears {[w hud_erase_count]==0}
 shot defense011-finished
 puts stderr "DEFENSE tower_shots=[w tower_shots] repair_spent=[w repair_spent] healed=[w repair_healed]"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine defense_case011 guard defense011
after realtime 800 {puts stderr "FAIL defense011 watchdog";exit}
