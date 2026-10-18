set throttle off
set ::f [open $::env(OUT) w]
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc rep {i} { puts $::f "t=$i state=[w 0xF014] pc=[format %04X [reg PC]] R1=[debug read {VDP regs} 1] R8=[debug read {VDP regs} 8]"; flush $::f }
after time 8 { tap 8 0x01 }
for {set i 9} {$i < 16} {incr i} { after time $i [list rep $i] }
after time 17 { close $::f; exit }
