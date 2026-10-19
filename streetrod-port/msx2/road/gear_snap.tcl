set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f; set g [open $::env(SHOTDIR)/$n.r2 w]; puts $g [vdpreg 2]; close $g }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 25 { tap 8 0x01 }
after time 30 { snap k0 }
after time 31 { keymatrixdown 8 0x20 }
after time 36 { snap k1 }
after time 37 { tap 8 0x01 }
after time 37.12 { snap k2 }
after time 37.3 { snap k3 }
after time 38.5 { snap k4 }
after time 40 { tap 8 0x01 }
after time 40.2 { snap k5 }
after time 42 { snap k6 }
after time 43 { exit }
