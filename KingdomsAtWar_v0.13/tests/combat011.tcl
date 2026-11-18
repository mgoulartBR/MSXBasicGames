source helpers011.tcl
proc enemy_point09 {id} {
 view_y [expr {[unit $id py]-6}]
 point [expr {[unit $id px]+8-[w cam_x]}] [expr {[unit $id py]-6-[w cam_y]}]
}
proc attack09 {id target} {
 if {$target<128 && [fog_state [unit $target x] [unit $target y]]!=2} {move_unit $id 1 19}
 choose $id
 if {$target<128} {enemy_point09 $target} else {set bid [expr {$target&127}];world_point [bd $bid x] [bd $bid y]}
 tap 5 32
}
proc combat09 {} {
 delay 45;materials
 ck two_kingdoms {[b unit_count]==4 && [b unit_hp 8]==48 && [b unit_hp 9]==48 && [b building_owner 1]==1 && [b building_hp 1]==180}
 choose 1;world_point 9 7;tap 5 32
 await {[w gold]>=8} 90
 attack09 2 8
 ck explicit_attack {[b combat_target 2]==8}
 await {[b unit_hp 8]<48 && [b unit_hp 2]<48} 140
 ck melee_reached {[unit 2 progress]==0 && abs([unit 2 x]-[unit 8 x])+abs([unit 2 y]-[unit 8 y])==1}
 shot combat09-duel
 await {[b unit_hp 8]==0 && [w combat_kills]>=1} 30
 ck defender_dies {[b unit_hp 2]>0 && [w combat_kills]==1}
 move_unit 2 3 23
 ck dead_tile_released {[unit 2 x]==3 && [unit 2 y]==23}
 attack09 2 129
 tap 5 1
 await {[unit 2 progress]==0 && [b requested 2]==0} 10
 ck stop_cancels_attack {[b combat_target 2]==255}
 set keep [b building_hp 1];delay 2
 ck no_remote_damage {[b building_hp 1]==$keep}
 set hold_x [unit 2 x];set hold_y [unit 2 y]
 tap 3 32;delay 1
 ck hold_keeps_position {[b stance 2]==1 && [unit 2 x]==$hold_x && [unit 2 y]==$hold_y && [b building_hp 1]==$keep}
 choose 0;world_point 9 23;tap 5 32
 ck worker_move_not_attack {[b combat_target]==255 && [unit 0 tx]==9 && [unit 0 ty]==23}
 await {[b unit_hp 0]==0 && [w combat_kills]>=2} 140
 ck allied_death {[w combat_kills]==2 && ([b selection_mask]&1)==0}
 choose_building 0;tap 4 128
 await {[b unit_hp 0]>0} 30
 ck dead_slot_recruited {[b unit_count]==4 && [b unit_kind]==0 && [b unit_hp]==32}
 move_unit 3 9 23;tap 3 32
 await {[b unit_hp 9]<48} 140
 await {[b unit_hp 9]==0 && [w combat_kills]>=3} 30
 ck hold_retaliates {[b stance 3]==1}
 ck second_defender_dies {[b unit_hp 3]>0 && [w combat_kills]==3}
 choose 0;world_point 3 27;tap 5 32;delay 1
 ck worker_cannot_attack {[b building_hp 1]==$keep && [b combat_target]==255}
 attack09 2 129
 await {[b building_hp 1]<180} 80
 ck structure_takes_damage {[b combat_target 2]==129 && [bd 1 state]==2}
 shot combat09-siege
 await {[bd 1 state]==0 && [w combat_buildings]==1} 50
 ck structure_destroyed {[b building_hp 1]==0 && [w combat_buildings]==1}
 ck footprint_released {[b terrain [expr {27*32+3}]]==0 && [b terrain [expr {30*32+6}]]==0}
 move_unit 2 3 28
 ck ruined_site_walkable {[unit 2 x]==3 && [unit 2 y]==28}
 ck own_keep_intact {[b building_hp]==240 && [bd 0 state]==2}
 shot combat09-cleared
 puts stderr "COMBAT hits=[w combat_hits] kills=[w combat_kills] buildings=[w combat_buildings] hp2=[b unit_hp 2] hp3=[b unit_hp 3]"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine combat_case09 guard combat09
after realtime 480 {puts stderr "FAIL combat09 watchdog";exit}
