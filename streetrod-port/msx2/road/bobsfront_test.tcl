# Bob's Drive-In: clica na abertura da garagem (rua pulada), fotografa a lanchonete, clica no carro verde (abre o desafio), fotografa, ESC e EXIT
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }
after time 20 { snap b1 }
after time 21 { tap 8 0x01 }
after time 25 { snap b2 }
after time 26 { tap 7 0x04 }
after time 29 { snap b3 }
after time 30 { tap 7 0x04 }
after time 34 { snap b4 }
after time 35 { exit }
