source kingdom_helpers.tcl
proc fog_state {x y} {
 set c [expr {$y*32+$x}];set m [expr {1<<($c&7)}];set off [expr {$c>>3}]
 return [expr {([b fog_visible $off]&$m)?2:([b fog_seen $off]&$m)?1:0}]
}
# Keep the normal collection/construction controls, exploring before unknown targets.
rename fund fund010
proc fund {wt gt} {
 set body [info body fund010]
 set needle {  set desired [expr {$y*16-96}]}
 set body [string map [list $needle {  if {[fog_state $x $y]==0} {move_unit 0 [expr {$x+2}] $y}
  set desired [expr {$y*16-96}]}] $body]
 proc fund011_impl {wood_target gold_target} $body
 fund011_impl $wt $gt
}
rename build build010
proc build {kind x y} {
 set n [expr {$kind==1?3:4}];set need 0
 for {set yy $y} {$yy<$y+$n} {incr yy} {for {set xx $x} {$xx<$x+$n} {incr xx} {if {[fog_state $xx $yy]!=2} {set need 1}}}
 if {$need} {
  set sx [expr {$x+$n}];set sy [expr {$y+$n/2}]
  if {$sx>=12} {set sx [expr {$x-1}]}
  move_unit 3 $sx $sy
 }
 return [build010 $kind $x $y]
}
rename conserved conserved010
proc conserved {} {
 set body [info body conserved010]
 set body [string map [list {$ww==$::initial_wood} {$ww+[w repair_spent]==$::initial_wood}] $body]
 proc conserved011_impl {} $body
 return [conserved011_impl]
}
proc attack011 {id target} {
 choose $id
 if {$target<128} {view_y [unit $target py];point [expr {[unit $target px]+8-[w cam_x]}] [expr {[unit $target py]-6-[w cam_y]}]} else {world_point [bd [expr {$target&127}] x] [bd [expr {$target&127}] y]}
 tap 5 32
}

# Units touching the top edge still have visible pixels inside the field.
rename choose choose010
set choose_body011 [info body choose010]
set choose_body011 [string map [list {[unit $id py]-6-[w cam_y]} {[unit $id py]-6-[w cam_y]<1?1:[unit $id py]-6-[w cam_y]}] $choose_body011]
proc choose {id} $choose_body011
