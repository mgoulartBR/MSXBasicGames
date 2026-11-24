# Auto-reparo dos nomes da camada B: um nome estragado numa celula visivel volta ao valor certo sozinho.
source helpers011.tcl
proc vr {a} {return [debug read {Sunrise GFX9000 VRAM} $a]}
proc vw {a v} {debug write {Sunrise GFX9000 VRAM} $a $v}
proc sh_case {} {
 delay 45
 set cells {{3 3} {10 7} {15 9} {0 0}}
 set good {}
 foreach c $cells {lassign $c x y;set a [expr {0x7E000+($y<<8)+($x<<2)}];lappend good [expr {[vr $a]+256*[vr [expr {$a+1}]]}];vw $a 1;vw [expr {$a+1}] 0}
 delay 10
 set i 0
 foreach c $cells {lassign $c x y;set a [expr {0x7E000+($y<<8)+($x<<2)}];set n [expr {[vr $a]+256*[vr [expr {$a+1}]]}]
  ck healed_${x}_$y {$n==[lindex $good $i]};incr i}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine sh_run guard sh_case
after realtime 400 {puts stderr "FAIL sh watchdog";exit}
