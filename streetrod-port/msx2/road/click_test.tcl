# garagem clicavel: vai ao hub, move o cursor por teclas, clica nos objetos; guarda VRAM (render com sprites: vram2png.py ... garage spr)
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram }
proc mv {mask t} { keymatrixdown 8 $mask; after time $t [list keymatrixup 8 $mask] }
set ::go 0
proc poll {} { if {[w 0xF014] == 9 && !$::go} { set ::go 1; after time 1 { tap 8 0x01 } }
  if {[w 0xF014] == 7} { seq; return }; after time 0.2 poll }
proc seq {} {
  after time 1 { mv 0x10 1.2 }          ;# esquerda
  after time 3 { snap a_street; tap 8 0x01 }   ;# clica HIT THE STREET -> "CRUISING IN TOWN" nota
  after time 6 { snap b_note; exit }
}
after time 5 poll
