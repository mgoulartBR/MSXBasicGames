# Cada tipo de unidade do reino inimigo (pose propria da facção): troca o tipo das unidades 8 e 9 e captura a cena.
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc ek_case {} {
 delay 55;materials
 set ex [unit 8 x];set ey [unit 8 y]
 # aliados perto para revelar a area (4 a 7), parados, sem combate
 set fx [unit 9 x];set fy [unit 9 y];set cells [list [list [expr {$ex+2}] [expr {$ey-2}]] [list [expr {$fx+2}] [expr {$fy-2}]] [list [expr {$ex+2}] [expr {$ey+2}]] [list [expr {$fx+2}] [expr {$fy+2}]]]
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];lassign [lindex $cells $k] cx cy;set a [expr {$::S(units)+16*$i}]
  foreach {o v} [list 0 [expr {$cx*16&255}] 1 [expr {$cx*16>>8}] 2 [expr {$cy*16&255}] 3 [expr {$cy*16>>8}] 4 $cx 5 $cy 6 $cx 7 $cy 8 $cx 9 $cy 10 0 11 0 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
  wr combat_target $i 255;wr unit_build $i 255
 }
 wr unit_count 0 8
 foreach {i kind} {4 4 5 3 6 4 7 5} {wr unit_kind $i $kind;wr unit_hp $i [lindex {32 48 48 32 72 96 48} $kind]}
 wr sprites_dirty 0 3;wr fog_refresh 0 1
 view_y [expr {$ey*16-60}]
 delay 2
 foreach {tag k8 k9} {a 0 2 b 3 4 c 5 6} {
  wr unit_kind 8 $k8;wr unit_kind 9 $k9;wr unit_hp 8 99;wr unit_hp 9 99
  wr sprites_dirty 0 3;delay 1;shot enemykinds-$tag
 }
 puts stderr "RESULT failures=$::failures";exit
}
coroutine ek_run guard ek_case
after realtime 400 {puts stderr "FAIL ek watchdog";exit}
