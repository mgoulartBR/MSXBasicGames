source mouse_helpers.tcl
proc press091 {} {set start [w loops];host B 1 1;delay 0.12;await {(([w loops]-$start)&65535)>=2} 8}
proc release091 {} {set start [w loops];host B 1 0;delay 0.12;await {(([w loops]-$start)&65535)>=2} 8}
proc drag091 {x1 y1 x2 y2} {mp $x1 $y1;press091;mp $x2 $y2;release091}
proc dump091 {name} {
 set variant $::env(KAW_VARIANT)
 set device [expr {$variant eq "V9990"?"Sunrise GFX9000 VRAM":"VRAM"}]
 set size [expr {$variant eq "V9990"?524288:131072}]
 set f [open $name-$variant.bin wb];fconfigure $f -translation binary;puts -nonewline $f [debug read_block $device 0 $size];close $f
 set f [open $name-$variant.txt w];puts $f "0 [w cam_x] [w cam_y]";close $f
}
proc fast_host011 {command a b} {puts $::mf "$command $a $b";flush $::mf;delay 0.04}
proc box091 {} {
 delay 55;mouse_open
 mp 34 20;press091;mp 116 68
 ck rectangle_live {[b drag_down] && [b drag_moved] && [b selection_mask]==3 && [b selection_count]==2}
 shot box091-held;dump091 box091-held
 set ::throttle on;set ::speed 100;delay 0.5
 release091
 ck rectangle_release {[b drag_down]==0 && [b selection_mask]==3}
 shot box091-released;dump091 box091-released
 set ::throttle on;set ::speed 100;delay 0.5
 keymatrixdown 6 1;drag091 185 84 118 38;keymatrixup 6 1
 ck shift_adds_swords {[b selection_mask]==15 && [b selection_count]==4}
 drag091 116 68 34 20
 ck reverse_selects_workers {[b selection_mask]==3}
 # Press/move/release without waiting for the main loop, during a camera redraw.
 mp 220 142;mc 1;mp 34 20
 keymatrixdown 8 64;delay 0.04;keymatrixup 8 64
 fast_host011 B 1 1;fast_host011 M 164 96;fast_host011 B 1 0
 set start [w loops];delay 0.12;await {(([w loops]-$start)&65535)>=2} 10
 ck fast_drag_keeps_press_origin {[b selection_mask]==3 && [b selection_count]==2}
 camera_until cam_y 0 32
 mp 104 104;mc 3
 await {[b worker_kind]==1 && [b worker_kind 1]==1} 10
 ck both_workers_receive_wood {[b worker_task]>0 && [b worker_task 1]>0}
 set seen 0;set start [machine_info time]
 while {[w wood]<16 || $seen!=3} {
  if {[b worker_task]==2} {set seen [expr {$seen|1}]}
  if {[b worker_task 1]==2} {set seen [expr {$seen|2}]}
  delay 0.033333;if {[machine_info time]-$start>80} {error "both wood workers did not harvest"}
 }
 ck both_workers_harvested {$seen==3}
 ck wood_delivered {[w wood]>=16}
 mp 152 120;mc 3
 await {[b worker_kind]==2 && [b worker_kind 1]==2} 10
 ck both_workers_receive_gold {[b worker_task]>0 && [b worker_task 1]>0}
 set seen 0;set start [machine_info time]
 while {[w gold]<16 || $seen!=3} {
  if {[b mine_phase]==2} {set seen [expr {$seen|1}]}
  if {[b mine_phase 1]==2} {set seen [expr {$seen|2}]}
  delay 0.033333;if {[machine_info time]-$start>80} {error "both miners did not enter"}
 }
 ck both_workers_mined {$seen==3}
 ck gold_delivered {[w gold]>=16}
 drag091 118 38 185 84
 ck selects_two_swords {[b selection_mask]==12 && [b selection_count]==2}
 camera_until cam_y 288 64
 mp 24 [expr {312-[w cam_y]}];mc 3
 await {[unit 2 x]==1 && [unit 2 y]==19 && [unit 2 state]==3 && [unit 3 state]==3 && ([b fog_visible [expr {(23*32+3)>>3}]]&(1<<((23*32+3)&7)))!=0} 100
 camera_until cam_y 304 64
 mp 56 [expr {362-[w cam_y]}];mc 3
 ck both_swords_attack {[b combat_target 2]==8 && [b combat_target 3]==8}
 ck approaches_distinct {[unit 2 tx]!=[unit 3 tx] || [unit 2 ty]!=[unit 3 ty] || [b requested 2] || [b requested 3]}
 await {[b unit_hp 8]==0} 100
 ck group_defeats_defender {[b unit_hp 2]>0 && [b unit_hp 3]>0}
 shot box091-group-combat;dump091 box091-group
 set ::throttle on;set ::speed 100;delay 0.5
 mp [expr {[unit 2 px]+8-[w cam_x]}] [expr {[unit 2 py]-6-[w cam_y]}];mc 1
 ck single_click_preserved {[b selection_mask]==4 && [b selected]==2}
 drag091 118 [expr {345-[w cam_y]}] 156 [expr {385-[w cam_y]}]
 ck enemies_not_selected {[b selection_mask]==0 && [b enemy_selected]==255}
 # Select a friendly actor at the current southern camera position.
 mp [expr {[unit 2 px]+8-[w cam_x]}] [expr {[unit 2 py]-6-[w cam_y]}];mc 1
 camera_until cam_y 0 32;mp 8 8;mc 3
 await {[unit 2 x]==0 && [unit 2 y]==0 && [unit 2 state]==3} 100
 drag091 1 1 27 28
 ck corner_drag_selects_clipped_unit {[b selection_mask]==4}
 mp 220 142;mc 1;mp 8 4;mc 1
 ck corner_click_selects_clipped_unit {[b selection_mask]==4 && [b selected]==2}
 shot box091-corner
 puts stderr "RESULT failures=$::failures";exit
}
coroutine box_case091 guard box091
after realtime 300 {puts stderr "FAIL box091 watchdog";exit}
