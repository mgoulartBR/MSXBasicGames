# animacao de entrada do carro: fotografa a VRAM + R#2 a cada 0,3 s depois de iniciar o jogo
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f
  set f [open $::env(SHOTDIR)/$n.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
set ::go 0
proc poll {} { if {[w 0xF014] == 9 && !$::go} { set ::go 1; after time 1 { tap 8 0x01 }
    for {set i 0} {$i < 10} {incr i} { after time [expr {1.3 + 0.35*$i}] [list snap f$i] }
    after time 6 { exit } }; after time 0.2 poll }
after time 5 poll
