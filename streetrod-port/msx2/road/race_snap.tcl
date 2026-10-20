set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f
  set f [open $::env(SHOTDIR)/$n.r2 w]; puts $f [debug read {VDP regs} 2]; close $f }
set ::last -1; set ::n 0; set ::t1 0
proc hubkeys {} { if {[w 0xF014] == 7} { tap 8 0x01; after time 1.6 hubkeys } }
proc poll {} {
    set st [w 0xF014]
    if {$st != $::last} { set ::last $st
        switch $st { 9 { after time 1 { tap 8 0x01 } } 7 { after time 3 hubkeys } 1 { set ::t1 [machine_info time] } }
    }
    if {$st == 1 && $::n < $::env(NSNAP) && [machine_info time] > $::t1 + 6 + 3*$::n} {
        incr ::n; snap [format "r%d" $::n]
        set f [open $::env(SHOTDIR)/y$::n.txt w]; for {set i 0} {$i<15} {incr i} { puts -nonewline $f "[w [expr {0xE2CF+2*$i}]] " }; puts $f ""; close $f
    }
    if {$::n >= $::env(NSNAP)} { exit }
    after time 0.1 poll
}
after time 5 poll
after time 300 { exit }
