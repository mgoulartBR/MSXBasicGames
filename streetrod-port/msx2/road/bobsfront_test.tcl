# Bob's: os carros chegam sozinhos; fotografa chegada, estacionados, clique na cabeca (dialogo) e a saida
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
proc hold {mask t} { keymatrixdown 8 $mask; after time $t [list keymatrixup 8 $mask] }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }              ;# clica na abertura da garagem -> Bob's
after time 18 { snap c1 }               ;# chegando
after time 24 { snap c2 }                 ;# estacionado
after time 25 { snap c3 }                 ;# cursor sobre a cabeca (etiqueta)
after time 26 { tap 8 0x01 }
after time 28 { snap c4 }                 ;# dialogo
after time 29 { tap 7 0x04 }              ;# sai do dialogo (ESC)
after time 45 { snap c5 }                 ;# depois de esperar: devem ter ido embora
after time 51 { exit }
