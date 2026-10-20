set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 23 { snap u0 }
after time 24 { tap 8 0x01 }
after time 26 { tap 8 0x01 }
after time 28 { tap 8 0x01 }
after time 29 { snap u1 }
after time 30 { exit }
