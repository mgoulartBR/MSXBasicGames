set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 45 { set f [open $::env(OUTF) w]; for {set i 32} {$i<47} {incr i} { puts -nonewline $f "[debug read {VDP regs} $i] " }; puts $f ""; puts $f "S2=[debug read {VDP status regs} 2]"; close $f; exit }
