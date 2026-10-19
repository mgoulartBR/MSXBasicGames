set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f; set g [open $::env(SHOTDIR)/$n.r2 w]; puts $g [vdpreg 2]; close $g }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 25 { tap 8 0x01 }
after time 32 { snap s1 }
after time 33 { exit }
