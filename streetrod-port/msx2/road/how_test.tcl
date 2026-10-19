set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 23 { tap 7 0x80 }
after time 34 { snap h1 }
after time 35 { tap 8 0x01 }
after time 38 { tap 8 0x01 }
after time 41 { snap h2 }
after time 42 { tap 8 0x40 }
after time 43 { tap 8 0x40 }
after time 44 { tap 8 0x01 }
after time 47 { snap h3 }
after time 48 { exit }
