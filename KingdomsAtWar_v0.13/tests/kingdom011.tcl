source helpers011.tcl
proc kingdom011 {} {
 delay 65;materials;fund 320 192
 ck collected_for_realm {[w wood]>=320 && [w gold]>=192 && [conserved]}
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
 ck repair_animates {([b repair_active]&1)!=0}
 shot repair011-working
 tap 2 64
 view_y [unit 8 py];point [expr {[unit 8 px]+8-[w cam_x]}] [expr {[unit 8 py]-6-[w cam_y]}];tap 5 32
 ck attack_group_preserves_repair {[b repair_target]==$damaged && [b combat_target 2]==8}
 await {[b unit_hp 8]==0} 140
 await {[b building_hp $damaged]==96 && [b repair_target]==255} 80
 ck repair_completes {[b building_hp $damaged]==96 && [conserved]}
 shot repair011-complete
 move_unit 2 2 23
 move_unit 0 6 5;move_unit 1 7 6;move_unit 3 11 5
 set h [build 1 0 0]
 set bar [build 2 7 0];set saw [build 3 8 10];set forge [build 4 0 10];set stable [build 5 4 15];set siege [build 7 7 26]
 choose_building $bar;point 112 204;tap 5 128;await {[b unit_count]==5} 25;move_unit 4 8 9
 choose_building $bar;point 192 204;tap 5 128;await {[b unit_count]==6} 25;move_unit 5 10 5
 choose_building $stable;tap 4 128;await {[b unit_count]==7} 25;move_unit 6 9 18
 choose_building $siege;tap 4 128;await {[b unit_count]==8} 25
 ck all_classes_recruited {[b unit_kind 4]==2 && [b unit_kind 5]==3 && [b unit_kind 6]==4 && [b unit_kind 7]==5 && [conserved]}
 puts stderr "FORGE wood=[w wood] gold=[w gold] repaired=[w repair_spent]"
 choose_building $forge;tap 4 128;await {[b research_level]==1} 15
 choose 6;delay 1
 ck research_applied {[b research_level]==1 && [conserved]}
 shot research011-knight
 move_unit 0 11 25;move_unit 0 11 20;move_unit 0 3 20
 set tower [build 6 4 19]
 ck tower_ready {[bd $tower state]==2 && [fog_state 8 23]==2 && [conserved]}
 await {[w tower_shots]>0 && [b unit_hp 9]<48} 30
 ck tower_auto_acquires {[w tower_shots]>0 && [b unit_hp 9]<48}
 shot tower011-firing
 await {[b unit_hp 9]==0} 40
 ck tower_defeats_defender {[w combat_kills]==2 && [b building_hp $tower]>0}
 attack011 7 129
 await {[b building_hp 1]<180} 140
 ck upgraded_ram_damage {[b building_hp 1]<=142}
 await {[bd 1 state]==0 && [w combat_buildings]==1} 40
 ck castle_destroyed {[b building_hp 1]==0 && [b building_hp]==240}
 ck all_buildings_and_units {[b building_count]==10 && [b unit_count]==8 && [conserved]}
 ck no_hud_clears {[w hud_erase_count]==0}
 shot kingdom011-finished
 puts stderr "KINGDOM tower_shots=[w tower_shots] repair_spent=[w repair_spent] healed=[w repair_healed] hits=[w combat_hits]"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine kingdom_case011 guard kingdom011
after realtime 1150 {puts stderr "FAIL kingdom011 watchdog";exit}
