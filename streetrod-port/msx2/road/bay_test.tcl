set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc hold {row mask t} { keymatrixdown $row $mask; after time $t [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 23 { snap b1 }
after time 24 { hold 8 0x40 0.3 }
after time 25 { tap 8 0x01 }
after time 28 { snap b2 }
after time 29 { tap 7 0x04 }
after time 31 { hold 8 0x10 0.5 }
after time 32 { tap 8 0x01 }
after time 36 { snap b3 }
after time 37 { exit }
