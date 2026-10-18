set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
after time 22 { set f [open $::env(OUT) a]; puts $f "state=[w 0xF014] sv=[w 0xF03A] money=[w 0xF030] day=[w 0xF032] nown=[w 0xF034]"; close $f; exit }
