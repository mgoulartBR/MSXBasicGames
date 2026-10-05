# Reads the result block of the Z80 self-test ROM: [0]=done [1]=total [2]=failures [3..8]=failing case bitmask
set f [open $::env(RESULTS) a]
after time 8.0 {
    set v {}
    for {set i 0} {$i < 10} {incr i} { lappend v [peek [expr {$::env(ST) + $i}]] }
    set done [lindex $v 0]; set total [lindex $v 1]; set fails [lindex $v 2]
    if {$done == 1 && $total >= 30 && $fails == 0} { puts $f "PASS Z80 self-test: $total cases (compiled by SDCC) all correct" } else { puts $f "FAIL Z80 self-test: done=$done total=$total failures=$fails mask=[lrange $v 3 8]" }
    flush $f; exit
}
