set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 21 { snap u1 }
after time 22 { tap 8 0x01 }
after time 27 { snap u2 }
after time 28 { tap 8 0x01 }
after time 30 { snap u3 }
after time 31 { tap 8 0x01 }
after time 33 { snap u4 }
after time 34 { tap 8 0x01 }
after time 35.5 { tap 8 0x01 }
after time 37 { tap 8 0x40 }
after time 38 { tap 8 0x01 }
after time 40 { snap u5 }
after time 41 { exit }
