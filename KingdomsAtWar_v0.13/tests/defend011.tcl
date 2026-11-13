# Defesa automatica: soldado parado sem "segurar posicao" ataca inimigo visivel a ate 5 celulas; com "segurar" so revida no alcance.
set ::env(KAW_DEF) 1
source helpers011.tcl
proc wr {name off v} {debug write memory [expr {$::S($name)+$off}] $v}
proc place {i x y} {
 set a [expr {$::S(units)+16*$i}]
 foreach {o v} [list 0 [expr {$x*16&255}] 1 [expr {$x*16>>8}] 2 [expr {$y*16&255}] 3 [expr {$y*16>>8}] 4 $x 5 $y 6 $x 7 $y 8 $x 9 $y 12 0 13 0 14 0 15 0] {debug write memory [expr {$a+$o}] $v}
}
proc def_case {} {
 delay 65
 place 2 8 4;place 3 10 4;place 8 8 8
 wr stance 3 1;wr combat_target 2 255;wr combat_target 3 255;wr combat_target 8 255
 wr sprites_dirty 0 3;wr fog_refresh 0 1
 await {[b combat_target 2]==8} 20
 ck idle_soldier_defends {[b combat_target 2]==8 || [b unit_hp 8]<48}
 ck hold_soldier_stays {[b combat_target 3]==255}
 await {[b unit_hp 8]<48} 40
 ck enemy_hurt {[b unit_hp 8]<48}
 # volta ao posto depois da luta
 wr unit_hp 8 0
 await {[expr {abs([unit 2 x]-8)+abs([unit 2 y]-4)}]<=1 && [b auto_hx 2]==255} 30
 ck returns_to_post {[expr {abs([unit 2 x]-8)+abs([unit 2 y]-4)}]<=1 && [b auto_hx 2]==255}
 # limite de perseguicao: o inimigo foge a ~1,25 celulas/s com muita vida; o soldado desiste ao passar de 8 celulas do posto
 place 2 8 4;wr combat_target 2 255;wr auto_hx 2 255
 place 8 8 9;wr unit_hp 8 250;wr combat_target 8 255;wr sprites_dirty 0 3;wr fog_refresh 0 1
 set far 0
 for {set k 0} {$k<22} {incr k} {
  place 8 8 [expr {9+$k}];delay 0.8
  set dx [expr {abs([unit 2 x]-8)}];set dy [expr {abs([unit 2 y]-4)}];set d [expr {$dx>$dy?$dx:$dy}];if {$d>$far} {set far $d}
  puts stderr "INFO k=$k u2=[unit 2 x],[unit 2 y] tgt=[b combat_target 2] home=[b auto_hx 2],[b auto_hy 2] req=[b requested 2] st=[unit 2 state]"
 }
 puts stderr "INFO leash far=$far"
 ck chase_started {$far>=4}
 ck leash_reached {$far>=8}
 ck leash_respected {$far<=11}
 await {[expr {abs([unit 2 x]-8)+abs([unit 2 y]-4)}]<=1 && [b auto_hx 2]==255} 40
 ck leashed_soldier_returns {[expr {abs([unit 2 x]-8)+abs([unit 2 y]-4)}]<=1 && [b combat_target 2]==255}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine def_run guard def_case
after realtime 600 {puts stderr "FAIL def watchdog";exit}
