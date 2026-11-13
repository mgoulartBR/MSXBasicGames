# Predios do reino inimigo (Mortos): coloca os 8 tipos no mapa e captura a cena.
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc place_enemy {k type x y} {
 set base {64 80 89 105 121 137 153 169};set n [expr {$type==1?3:4}];set a [expr {$::S(buildings)+12*$k}]
 foreach {o v} [list 0 $type 1 2 2 $x 3 $y 4 255 5 0 6 0 7 0 8 0 9 0 10 0 11 0] {debug write memory [expr {$a+$o}] $v}
 wr building_owner $k 1;wr building_hp $k 96
 for {set dy 0} {$dy<$n} {incr dy} {for {set dx 0} {$dx<$n} {incr dx} {
  set c [expr {($y+$dy)*32+$x+$dx}]
  wr terrain $c [expr {10+$k}];wr art_map $c [expr {[lindex $base $type]+$dy*$n+$dx}]
 }}
}
proc eb_case {} {
 delay 65
 # fileira de cima (y=21): casa, quartel, serraria, forja; fileira de baixo (y=27): castelo inimigo, estabulo, torre, oficina
 foreach {k type x y} {2 1 0 21  3 2 4 21  4 3 8 21  5 4 12 21  6 5 8 27  7 6 12 27  8 7 16 27} {place_enemy $k $type $x $y}
 wr building_count 0 9
 # aliados espalhados para revelar a area
 set cells {{4 24} {9 24} {13 24} {17 25}}
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];lassign [lindex $cells $k] cx cy;set a [expr {$::S(units)+16*$i}]
  foreach {o v} [list 0 [expr {$cx*16&255}] 1 [expr {$cx*16>>8}] 2 [expr {$cy*16&255}] 3 [expr {$cy*16>>8}] 4 $cx 5 $cy 6 $cx 7 $cy 8 $cx 9 $cy 10 0 11 0 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
  wr combat_target $i 255;wr unit_build $i 255;wr unit_kind $i 1;wr unit_hp $i 48
 }
 wr unit_count 0 8;wr sprites_dirty 0 3;wr fog_refresh 0 1
 view_y [expr {21*16+72}]
 delay 3
 # tira os aliados da cena (a area ja revelada fica lembrada)
 for {set k 0} {$k<4} {incr k} {
  set i [expr {4+$k}];set a [expr {$::S(units)+16*$i}]
  foreach {o v} {0 16 1 0 2 16 3 0 4 1 5 1 6 1 7 1 8 1 9 1} {debug write memory [expr {$a+$o}] $v}
 }
 wr sprites_dirty 0 3;delay 2
 shot enemybuild-a
 camera_until cam_x 112 128
 delay 1
 shot enemybuild-b
 puts stderr "RESULT failures=$::failures";exit
}
coroutine eb_run guard eb_case
after realtime 600 {puts stderr "FAIL eb watchdog";exit}
