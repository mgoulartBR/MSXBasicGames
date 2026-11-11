# Torre inimiga (tipo 6, dono 1) contra unidades humanas: atira dentro de 6 celulas, ignora o que esta fora do alcance.
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc place_enemy {k type x y} {
 set base {64 80 89 105 121 137 153 169};set n [expr {$type==1?3:4}];set a [expr {$::S(buildings)+12*$k}]
 foreach {o v} [list 0 $type 1 2 2 $x 3 $y 4 255 5 0 6 0 7 0 8 0 9 0 10 0 11 0] {debug write memory [expr {$a+$o}] $v}
 wr building_owner $k 1;wr building_hp $k 96;wr tower_cooldown $k 0
 for {set dy 0} {$dy<$n} {incr dy} {for {set dx 0} {$dx<$n} {incr dx} {
  set c [expr {($y+$dy)*32+$x+$dx}]
  wr terrain $c [expr {10+$k}];wr art_map $c [expr {[lindex $base $type]+$dy*$n+$dx}]
 }}
}
proc place {i x y} {
 set a [expr {$::S(units)+16*$i}]
 foreach {o v} [list 0 [expr {$x*16&255}] 1 [expr {$x*16>>8}] 2 [expr {$y*16&255}] 3 [expr {$y*16>>8}] 4 $x 5 $y 6 $x 7 $y 8 $x 9 $y 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
}
proc et_case {} {
 delay 55
 # torre inimiga pronta em (6,12) (cobre x6..9, y12..15); soldado humano 2 em (6,10): 2 celulas, soldado 3 em (6,3): fora do alcance
 place_enemy 2 6 6 12;wr building_count 0 3
 place 2 6 10;place 3 6 3
 set shots0 [w tower_shots];set hp3 [b unit_hp 3];set hp9 [b unit_hp 8]
 wr sprites_dirty 0 3;wr fog_refresh 0 1
 view_y [expr {10*16}]
 await {[w tower_shots]>$shots0} 20
 ck tower_fires {[w tower_shots]>$shots0}
 await {[b unit_hp 2]<48} 20
 shot enemytower-fire
 ck human_unit_hit {[b unit_hp 2]<48}
 ck out_of_range_untouched {[b unit_hp 3]==$hp3}
 ck enemy_not_targeted {[b unit_hp 8]==$hp9}
 await {[b unit_hp 2]==0} 40
 ck tower_kills_idle_unit {[b unit_hp 2]==0}
 set s [w tower_shots];delay 4
 ck tower_idle_without_targets {[w tower_shots]==$s}
 puts stderr "INFO tower shots=[w tower_shots] kills=[w combat_kills]"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine et_run guard et_case
after realtime 400 {puts stderr "FAIL et watchdog";exit}
