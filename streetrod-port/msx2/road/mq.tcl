set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::last -1; set ::t1 0
proc hubkeys {} { if {[w 0xF014] == 7} { tap 8 0x01; after time 1.6 hubkeys } }
proc poll {} {
    set st [w 0xF014]
    if {$st != $::last} { set ::last $st
        switch $st { 9 { after time 1 { tap 8 0x01 } } 7 { after time 3 hubkeys } 1 { set ::t1 [machine_info time] } } }
    if {$st == 1 && [machine_info time] > $::t1 + 8} { set f [open $::env(OUT) w]; puts $f "pos=[w 0xF008] opos=[w 0xF016] d8=[w 0xF03C] n=[w 0xF03E]"; close $f; exit }
    after time 0.5 poll
}
after time 5 poll
after time 200 { exit }
