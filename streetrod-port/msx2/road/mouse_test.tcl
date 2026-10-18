# mouse (simulado: -DTEST_MOUSE, setas = deslocamento, ESPACO = botao): clica no relogio, passa o cursor sobre YES e clica -> volta ao titulo (state 9)
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
proc say {s} { set f [open $::env(OUT) a]; puts $f "t=[machine_info time] state=[w 0xF014] $s"; close $f }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }
after time 19 { snap m1; say "lista aberta" }
after time 20 { keymatrixdown 8 0x50 }
after time 20.45 { keymatrixup 8 0x50 }
after time 22 { snap m2; say "hover" }
after time 23 { tap 8 0x01 }
after time 27 { say "depois do clique"; snap m3; exit }
