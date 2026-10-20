set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
proc sp {} { tap 8 0x01; after time 1.5 sp }
after time 21 sp
set ::last 0
proc poll {} { set v [expr {[debug read memory 0xF03A] | ([debug read memory 0xF03B] << 8)}]; if {$v != $::last} { set ::last $v; snap [format "s%02d" $v] }; after time 0.3 poll }
after time 20 poll
after time 90 { exit }
