set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f; set g [open $::env(SHOTDIR)/$n.r2 w]; puts $g [vdpreg 2]; close $g }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 25 { tap 8 0x01 }
after time 28 { snap k0 }
after time 29 { keymatrixdown 8 0x20 }
after time 32 { snap k1 }
after time 33 { tap 8 0x01 }
after time 33.1 { snap k2 }
after time 33.25 { snap k3 }
after time 33.8 { snap k4 }
after time 35 { tap 8 0x01 }
after time 35.15 { snap k5 }
after time 36 { snap k6 }
after time 37 { tap 8 0x01 }
after time 38.2 { snap k7 }
after time 39 { exit }
