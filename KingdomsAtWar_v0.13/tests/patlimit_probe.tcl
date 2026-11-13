# Sonda: o V9990 em P1 aceita numero de pattern acima de 2047 na tabela de nomes da camada B?
source helpers011.tcl
proc vr {a} {return [debug read {Sunrise GFX9000 VRAM} $a]}
proc vw {a v} {debug write {Sunrise GFX9000 VRAM} $a $v}
proc pat_addr {n} {return [expr {0x40000+(($n>>5)<<10)+(($n&31)<<2)}]}
proc copy_pat {src dst} {for {set k 0} {$k<8} {incr k} {for {set b 0} {$b<4} {incr b} {vw [expr {[pat_addr $dst]+($k<<7)+$b}] [vr [expr {[pat_addr $src]+($k<<7)+$b}]]}}}
proc set_name {x y n0} {
 set a [expr {0x7E000+($y<<8)+($x<<2)}]
 foreach {off n} [list 0 $n0 2 [expr {$n0+1}] 128 [expr {$n0+2}] 130 [expr {$n0+3}]] {vw [expr {$a+$off}] [expr {$n&255}];vw [expr {$a+$off+1}] [expr {$n>>8}]}
}
proc pl_case {} {
 delay 55
 # tile do castelo humano em (3,7): le o nome atual
 set a [expr {0x7E000+(7<<8)+(3<<2)}]
 set n0 [expr {[vr $a]+256*[vr [expr {$a+1}]]}]
 puts stderr "INFO castle cell name=$n0"
 foreach {dst x} {2100 5 3000 8} {
  for {set q 0} {$q<4} {incr q} {copy_pat [expr {$n0+$q}] [expr {$dst+$q}]}
  set_name $x 1 $dst
 }
 # controle: 1900 (abaixo de 2047)
 for {set q 0} {$q<4} {incr q} {copy_pat [expr {$n0+$q}] [expr {1900+$q}]}
 set_name 2 1 1900
 view_y 0;delay 1;shot patlimit
 puts stderr "RESULT failures=$::failures";exit
}
coroutine pl_run guard pl_case
after realtime 200 {puts stderr "FAIL watchdog";exit}
