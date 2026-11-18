# Quatro soldados inimigos ao mesmo tempo (atores 8..11): os dois extras (10 e 11) so usam poses de 32 px, bloco unico.
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc place {i x y {own 0}} {
 set a [expr {$::S(units)+16*$i}]
 foreach {o v} [list 0 [expr {$x*16&255}] 1 [expr {$x*16>>8}] 2 [expr {$y*16&255}] 3 [expr {$y*16>>8}] 4 $x 5 $y 6 $x 7 $y 8 $x 9 $y 10 $own 11 0 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
}
proc ex_case {} {
 delay 45
 # tres soldados humanos para revelar a area, quatro inimigos de tipos diferentes
 wr unit_count 0 8
 foreach {i x y k hp} {4 3 11 1 48 5 5 11 1 48 6 7 11 1 48 7 9 11 1 48} {place $i $x $y;wr unit_kind $i $k;wr unit_hp $i $hp;wr combat_target $i 255;wr unit_build $i 255}
 foreach {i x y k hp} {8 3 13 4 72 9 5 13 3 32 10 7 13 1 48 11 9 13 3 32} {place $i $x $y 1;wr unit_kind $i $k;wr unit_hp $i $hp;wr combat_target $i 255;wr unit_facing $i 2;wr unit_frame $i 9}
 wr sprites_dirty 0 3;wr fog_refresh 0 1
 view_y 200
 delay 4
 shot extras-a
 # golpes e clarao: os quatro inimigos atacam (gestos) e as barras de vida aparecem
 foreach i {8 9 10 11} {wr attack_flash $i 8;wr hurt_flash $i 8}
 wr unit_hp 10 20;wr combat_active 0 1;wr sprites_dirty 0 3
 delay 0.05;shot extras-b
 puts stderr "INFO cam=[w cam_x],[w cam_y]"
 puts stderr "INFO hp=[b unit_hp 8],[b unit_hp 9],[b unit_hp 10],[b unit_hp 11] kinds=[b unit_kind 8],[b unit_kind 9],[b unit_kind 10],[b unit_kind 11]"
 puts stderr "RESULT failures=$::failures";exit
}
coroutine ex_run guard ex_case
after realtime 300 {puts stderr "FAIL ex watchdog";exit}
