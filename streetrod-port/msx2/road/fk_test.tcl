set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
after time 24 { set f [open $::env(OUT) w]; puts $f "state=[w 0xF014] t1=[w 0xF03A] money_after_load=[w 0xF03C] t3=[format %04X [w 0xF03E]] rank4=[w 0xF038]"; close $f; exit }
