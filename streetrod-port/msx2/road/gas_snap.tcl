set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f; set g [open $::env(SHOTDIR)/$n.r2 w]; puts $g [vdpreg 2]; close $g }
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }
foreach {t n} {16.6 q1 16.9 q2 17.2 q3 17.5 q4 18.2 q5 22 q6} { after time $t "snap $n" }
after time 23 { exit }
