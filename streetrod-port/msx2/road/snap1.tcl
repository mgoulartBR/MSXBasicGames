set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }

after time 20 { snap g0 }
after time 21 { snap g1 }
after time 22 { snap g2 }
after time 23 { snap g3 }
after time 24 { snap g4 }
after time 26 { snap g5 }
after time 28 { snap g6 }
after time 30 { exit }
