# transicoes de cena: titulo -> nome -> garagem -> Bob's -> volta; fotografa em varios instantes
set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f; set f [open $::env(SHOTDIR)/$n.r2 w]; puts $f [debug read {VDP regs} 2]; close $f }
after time 8 { tap 8 0x01 }
foreach t {10 12 14 16} { after time $t "snap n$t" }
after time 18 { keymatrixdown 8 0x10 }
after time 20 { keymatrixup 8 0x10 }
after time 21 { tap 8 0x01 }
foreach t {23 25 27} { after time $t "snap b$t" }
after time 30 { exit }
