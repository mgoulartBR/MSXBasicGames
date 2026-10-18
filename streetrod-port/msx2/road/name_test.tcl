set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 22 { snap n0 }
after time 23 { tap 2 0x80 }
after time 24 { tap 4 0x10 }
after time 25 { tap 2 0x80 }
after time 26 { tap 4 0x20 }
after time 27 { snap n1 }
after time 28 { tap 8 0x01 }
after time 29 { snap n2 }
after time 30 { tap 7 0x80 }
after time 38 { snap n3 }
after time 39 { exit }
