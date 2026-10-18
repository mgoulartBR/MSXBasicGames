set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }
after time 19 { snap m1 }
after time 20 { tap 8 0x40 }
after time 21 { tap 8 0x40 }
after time 22 { snap m2 }
after time 23 { tap 8 0x01 }
after time 26 { snap m3 }
after time 27 { exit }
