source kingdom_helpers.tcl
proc fog_state {x y} {
 set cell [expr {$y*32+$x}];set mask [expr {1<<($cell&7)}];set off [expr {$cell>>3}]
 return [expr {([b fog_visible $off]&$mask)?2:([b fog_seen $off]&$mask)?1:0}]
}
proc dump_fog011 {state} {
 set pre fog011-$::env(KAW_VARIANT)-$state
 set device [expr {($::env(KAW_VARIANT) eq "V9990" || $::env(KAW_VARIANT) eq "V9990P1")?"Sunrise GFX9000 VRAM":"VRAM"}]
 set size [expr {($::env(KAW_VARIANT) eq "V9990" || $::env(KAW_VARIANT) eq "V9990P1")?524288:131072}]
 delay 0.7
 foreach {suffix dev start length} [list map memory $::S(art_map) 1024 memory memory $::S(fog_memory) 1024 seen memory $::S(fog_seen) 128 visible memory $::S(fog_visible) 128 vram $device 0 $size] {
  set f [open $pre-$suffix.bin wb];fconfigure $f -translation binary;puts -nonewline $f [debug read_block $dev $start $length];close $f
 }
 set f [open $pre-meta.txt w];puts $f "0 [w cam_x] [w cam_y]";close $f
 shot $pre
}
proc fog_case011 {} {
 delay 65
 ck home_visible {[fog_state 4 8]==2 && [fog_state 8 4]==2}
 ck enemy_unexplored {[fog_state 3 23]==0 && [fog_state 8 23]==0 && [fog_state 3 28]==0}
 camera_until cam_y 288 64
 ck camera_does_not_reveal {[fog_state 3 18]==0}
 dump_fog011 unknown
 # A hidden defender cannot be inspected, and a right-click is only a ground order.
 world_point 3 23;tap 5 128
 ck hidden_enemy_not_selected {[b enemy_selected]==255 && [b building_selected]==0}
 choose 2;world_point 3 23;tap 5 32
 ck hidden_enemy_not_targeted {[b combat_target 2]==255}
 tap 5 1
 move_unit 2 0 18
 ck scout_reveals {[fog_state 3 18]==2}
 camera_until cam_y 288 64
 dump_fog011 visible
 move_unit 2 8 4
 camera_until cam_y 288 64
 ck exploration_remembered {[fog_state 3 18]==1 && [fog_state 3 23]!=2}
 dump_fog011 remembered
 set count [w fog_updates];delay 1
 ck stationary_no_fog_rebuild {[w fog_updates]==$count}
 choose 0;point 24 204;tap 5 128;world_point 0 26
 ck unseen_placement_denied {![b place_valid]};tap 7 4
 ck hud_preserved {[w hud_erase_count]==0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine fog011 guard fog_case011
after realtime 500 {puts stderr "FAIL fog011 watchdog";exit}
