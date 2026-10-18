# ESC volta: hub -> jornal -> pecas -> ESC, ESC; hub -> Bobs (sem carro: aviso) ; registra estado
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
after time 8 { tap 8 0x01 }
after time 15 { tap 8 0x20 }            ;# up -> NEWSPAPER
after time 16.5 { tap 8 0x01 }          ;# abre jornal
after time 18 { tap 8 0x40 }
after time 19.5 { tap 8 0x01 }          ;# auto parts
after time 21 { snap parts; tap 7 0x20 } ;# ESC -> categorias? (volta ao menu do jornal)
after time 22.5 { snap news }
after time 23 { tap 7 0x20 }            ;# ESC -> hub
after time 24.5 { snap hub; exit }
