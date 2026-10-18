set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 14 { tap 8 0x01 }
after time 17 { tap 8 0x01 }
after time 24 { snap r1 }
after time 25 { tap 8 0x01 }
after time 30 { snap r2 }
after time 31 { exit }
