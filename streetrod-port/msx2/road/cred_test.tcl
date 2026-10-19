set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 21 { tap 8 0x40 }
after time 22 { tap 8 0x40 }
after time 23 { tap 8 0x40 }
after time 24 { snap c0 }
after time 25 { tap 8 0x01 }
after time 28 { snap c1 }
after time 29 { tap 8 0x01 }
after time 34 { snap c2 }
after time 35 { exit }
