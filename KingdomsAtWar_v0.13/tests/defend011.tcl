# Defesa automatica: soldado parado sem "segurar posicao" ataca inimigo visivel a ate 5 celulas; com "segurar" so revida no alcance.
set ::env(KAW_DEF) 1
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc place {i x y} {
 set a [expr {$::S(units)+16*$i}]
 foreach {o v} [list 0 [expr {$x*16&255}] 1 [expr {$x*16>>8}] 2 [expr {$y*16&255}] 3 [expr {$y*16>>8}] 4 $x 5 $y 6 $x 7 $y 8 $x 9 $y 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
}
proc def_case {} {
 delay 55
 place 2 8 4;place 3 10 4;place 8 8 8
 wr stance 3 1;wr combat_target 2 255;wr combat_target 3 255;wr combat_target 8 255
 wr sprites_dirty 0 3;wr fog_refresh 0 1
 await {[b combat_target 2]==8} 20
 ck idle_soldier_defends {[b combat_target 2]==8 || [b unit_hp 8]<48}
 ck hold_soldier_stays {[b combat_target 3]==255}
 await {[b unit_hp 8]<48} 40
 ck enemy_hurt {[b unit_hp 8]<48}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine def_run guard def_case
after realtime 300 {puts stderr "FAIL def watchdog";exit}
