set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }              ;# clica no galao
after time 18 { snap g1 }                 ;# entrando
after time 23 { snap g2 }                 ;# estacionado
after time 24 { tap 8 0x01 }              ;# clica na bomba
after time 27 { snap g3 }                 ;# menu
after time 28 { tap 8 0x01 }              ;# FILL IT UP
after time 31 { snap g4 }                 ;# nota
after time 32 { tap 8 0x01 }
after time 40 { snap g5 }                 ;# de volta
after time 41 { exit }
