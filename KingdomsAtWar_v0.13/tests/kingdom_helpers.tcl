source helpers.tcl
proc bd {id field} {
 set off [dict get {type 0 state 1 x 2 y 3 worker 4 remaining 5 training 7 time 8 rallyx 10 rallyy 11} $field]
 set at [expr {$id*12+$off}];set v [b buildings $at]
 if {$field in {remaining time}} {incr v [expr {256*[b buildings [expr {$at+1}]]}]}
 return $v
}
proc view_y {wy} {
 if {$wy<[w cam_y]+8 || $wy>[w cam_y]+136} {
  set target [expr {$wy-72}];if {$target<0} {set target 0};if {$target>352} {set target 352}
  if {[w cam_y]<$target} {camera_until cam_y $target 64} else {camera_until cam_y $target 32}
 }
}
proc world_point {x y} {view_y [expr {$y*16}];point [expr {$x*16+8-[w cam_x]}] [expr {$y*16+8-[w cam_y]}]}
proc choose {id} {
 for {set n 0} {$n<24} {incr n} {
  if {[b mine_phase $id]==2} {delay 0.2;continue}
  view_y [unit $id py]
  point [expr {[unit $id px]+8-[w cam_x]}] [expr {[unit $id py]-6-[w cam_y]}];tap 5 128
  if {[b selected]==$id} {return}
 }
 error "cannot select unit $id"
}
proc move_unit {id x y} {
 choose $id;world_point $x $y;tap 5 32
 await {[unit $id x]==$x && [unit $id y]==$y && [unit $id state]==3} 40
}
proc build {kind x y} {
 choose 0;camera_until cam_y 0 32;point [expr {24+($kind-1)*32}] 204;tap 5 128
 world_point $x $y
 if {![b place_valid]} {error "invalid build kind=$kind x=$x y=$y cursor=[b mouse_x],[b mouse_y]"}
 set id 1;while {$id<[b building_count] && [bd $id state]!=0} {incr id};tap 5 128
 await {[bd $id state]==2} 40
 return $id
}
proc choose_building {id} {
 world_point [bd $id x] [bd $id y];tap 5 128
 if {[b building_selected]!=$id+1} {error "cannot select building $id"}
}
proc materials {} {
 set ::initial_wood 0;set ::initial_gold 0
 for {set i 0} {$i<1024} {incr i} {
  if {[b terrain $i]==1} {incr ::initial_wood [b stock $i]}
  if {[b terrain $i]==4} {incr ::initial_gold [b stock $i]}
 }
}
proc conserved {} {
 set ww [w wood];set gg [w gold]
 for {set i 0} {$i<1024} {incr i} {if {[b terrain $i]==1} {incr ww [b stock $i]};if {[b terrain $i]==4} {incr gg [b stock $i]}}
 for {set i 0} {$i<[b unit_count]} {incr i} {
  if {[b cargo_kind $i]==1} {incr ww [b cargo $i]} else {incr gg [b cargo $i]}
  if {$i>=4} {set kind [b unit_kind $i];incr ww [b recruit_wood $kind];incr gg [b recruit_gold $kind]}
 }
 for {set i 1} {$i<[b building_count]} {incr i} {if {[bd $i state]} {incr ww [b build_wood [bd $i type]];incr gg [b build_gold [bd $i type]]}}
 for {set i 0} {$i<[b building_count]} {incr i} {
  set k [expr {[bd $i training]-1}]
  if {$k>=0 && $k<6} {incr ww [b recruit_wood $k];incr gg [b recruit_gold $k]}
  if {$k==6} {incr ww 16;incr gg 16}
 }
 if {[b research_level]} {incr ww 16;incr gg 16}
 return [expr {$ww==$::initial_wood && $gg==$::initial_gold}]
}
proc fund {wood_target gold_target} {
 choose 0;world_point 6 7;tap 5 32
 choose 1;world_point 9 7;tap 5 32
 await {[w wood]>=32} 100
 # Continue through nearby natural trees below the initial viewport.
 set roots {}
 for {set y 7} {$y<22} {incr y} {for {set x 0} {$x<11} {incr x} {if {[b terrain [expr {$y*32+$x}]]==1} {lappend roots [list $x $y]}}}
 foreach root $roots {
  if {[w wood]>=$wood_target} {break}
  lassign $root x y
  if {![b stock [expr {$y*32+$x}]]} {continue}
  set desired [expr {$y*16-96}];if {$desired<0} {set desired 0}
  camera_until cam_y 0 32;choose 0
  if {[w cam_y]<$desired} {camera_until cam_y $desired 64}
  point [expr {$x*16+8}] [expr {$y*16+8-[w cam_y]}];tap 5 32
  set bank [w wood]
  await {[w wood]>=$bank+32 || [b stock [expr {$y*32+$x}]]==0} 100
  await {[b cargo 0]==0 && [b worker_task]==0} 100
 }
 await {[w gold]>=$gold_target} 200
 camera_until cam_y 0 32
 # A normal group command also selects miners hidden inside the cave.
 tap 2 64;point 104 72;tap 5 32
 await {[b mine_phase 0]==0 && [b mine_phase 1]==0 && [unit 0 state]==3 && [unit 1 state]==3 && [unit 2 state]==3 && [unit 3 state]==3} 50

}

proc guard {cmd} {if {[catch {uplevel #0 $cmd} m opts]} {puts stderr "FAIL $m\n[dict get $opts -errorinfo]";puts stderr "TRACE mask=[b selection_mask] selected=[b selected] camera=[w cam_x],[w cam_y] job=[b job_unit]";for {set z 0} {$z<4} {incr z} {puts stderr "UNIT $z hp=[b unit_hp $z] x=[unit $z x] y=[unit $z y] tx=[unit $z tx] ty=[unit $z ty] state=[unit $z state] progress=[unit $z progress] req=[b requested $z] task=[b worker_task $z] target=[b combat_target $z] stance=[b stance $z]"};screenshot -raw -doublesize ../screenshots/$::env(KAW_VARIANT)/failure07.png;exit}}

proc fresh_tree {} {
 for {set y 7} {$y<30} {incr y} {for {set x 0} {$x<11} {incr x} {if {[b terrain [expr {$y*32+$x}]]==1 && [b stock [expr {$y*32+$x}]]>0} {return [list $x $y]}}}
 error "no fresh tree"
}
