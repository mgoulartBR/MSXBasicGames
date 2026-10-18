# jornal pelo mouse (simulado): clica no jornal, vai ao botao EXIT da barra de status e clica; depois repete e abre USED CARS, fecha pelo X
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
proc hold {mask t} { keymatrixdown 8 $mask; after time $t [list keymatrixup 8 $mask] }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }                ;# clica no jornal
after time 19 { snap p1 }
after time 20 { hold 0x20 0.4 }             ;# sobe: USED CARS (y ~ 50)
after time 21.5 { tap 8 0x01 }              ;# abre a lista
after time 25 { snap p2 }
after time 26 { hold 0x10 0.3 }
after time 27 { hold 0x20 0.2 }
after time 28 { snap p3 }
after time 40 { exit }
