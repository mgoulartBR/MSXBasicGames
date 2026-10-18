set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f; set g [open $::env(SHOTDIR)/$n.r2 w]; puts $g [vdpreg 2]; close $g }
after time 8 { tap 8 0x01 }
foreach {t n} {10.9 s1 11.1 s2 11.3 s3 11.5 s4 11.7 s5 12.2 s6} { after time $t "snap $n" }
after time 13 { exit }
