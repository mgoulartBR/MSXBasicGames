# Canteiro de obras inimigo (estado 1): usa a arte dos mortos quando integrada; senao o canteiro dos humanos.
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc place_site {k type x y} {
 set n [expr {$type==1?3:4}];set a [expr {$::S(buildings)+12*$k}]
 foreach {o v} [list 0 $type 1 1 2 $x 3 $y 4 255 5 100 6 0 7 0 8 0 9 0 10 0 11 0] {debug write memory [expr {$a+$o}] $v}
 wr building_owner $k 1;wr building_hp $k 96
 for {set dy 0} {$dy<$n} {incr dy} {for {set dx 0} {$dx<$n} {incr dx} {
  set c [expr {($y+$dy)*32+$x+$dx}]
  wr terrain $c [expr {10+$k}];wr art_map $c [expr {185+$dy*4+$dx}]
 }}
}
proc place {i x y} {
 set a [expr {$::S(units)+16*$i}]
 foreach {o v} [list 0 [expr {$x*16&255}] 1 [expr {$x*16>>8}] 2 [expr {$y*16&255}] 3 [expr {$y*16>>8}] 4 $x 5 $y 6 $x 7 $y 8 $x 9 $y 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
}
proc es_case {} {
 delay 55
 place_site 2 2 5 24;place_site 3 1 10 24;wr building_count 0 4
 place 2 4 26;place 3 9 26;place 0 4 30;place 1 9 30
 wr sprites_dirty 0 3;wr fog_refresh 0 1
 view_y 400;delay 4;shot enemysite-a
 place 2 1 1;place 3 2 1;place 0 3 1;place 1 1 3
 wr sprites_dirty 0 3;wr fog_refresh 0 1
 view_y 400;delay 4;shot enemysite-remembered
 puts stderr "RESULT failures=$::failures";exit
}
coroutine es_run guard es_case
after realtime 300 {puts stderr "FAIL es watchdog";exit}
