set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {name} { save_debuggable VRAM $::env(SHOTDIR)/$name.vram; set f [open $::env(SHOTDIR)/$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
foreach {at k} {8 space 14 space 18 space 22 space 26 space} { after time $at { tap 8 0x01 } }
set ::n 0
proc poll {} {
    if {[w 0xF014] == 1 && $::n < 14} { incr ::n; snap [format "g%02d" $::n]; after time 0.7 poll; return }
    after time 0.3 poll
}
after time 20 poll
after time 90 { exit }
