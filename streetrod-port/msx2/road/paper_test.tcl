# jornal: clica no jornal da garagem, sobe o cursor ate USED CARS, abre a lista, volta (ESC) e fotografa cada etapa
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }
after time 20 { snap n1 }
after time 21 { keymatrixdown 8 0x20 }
after time 23.2 { keymatrixup 8 0x20 }
after time 24 { snap n2 }
after time 25 { tap 8 0x01 }
after time 29 { snap n3 }
after time 30 { tap 7 0x04 }
after time 33 { snap n4 }
after time 34 { tap 7 0x04 }
after time 40 { snap n5 }
after time 41 { exit }
