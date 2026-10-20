set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::last -1; set ::t1 0; set ::acc {}
proc hubkeys {} { if {[w 0xF014] == 7} { tap 8 0x01; after time 1.6 hubkeys } }
proc poll {} {
    set st [w 0xF014]
    if {$st != $::last} { set ::last $st
        switch $st { 9 { after time 1 { tap 8 0x01 } } 7 { after time 3 hubkeys } 1 { set ::t1 [machine_info time] } } }
    if {$st == 1 && [machine_info time] > $::t1 + 4} { lappend ::acc "[w $::env(FPSA)]/[w $::env(CMDA)]" }
    if {[llength $::acc] >= 40} { set f [open $::env(OUT) w]; puts $f $::acc; close $f; exit }
    after time 0.5 poll
}
after time 5 poll
after time 200 { exit }
