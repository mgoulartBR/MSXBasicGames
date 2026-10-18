# ciclo completo: titulo -> garagem -> Bob's (Espaco repetido aceita tudo) -> corrida curta (-DRACE_LEN=40 -DAUTOPILOT=1) -> resultado -> garagem ...
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::log [open $::env(OUT) w]
set ::last -1; set ::laps 0
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] res=[w 0xF018] money=[w 0xF030] day=[w 0xF032] cars=[w 0xF034] | $s"; flush $::log }
proc hubkeys {} { if {[w 0xF014] == 7} { tap 8 0x01; after time 1.6 hubkeys } }
proc poll {} {
    set st [w 0xF014]
    if {$st != $::last} {
        set ::last $st; say "estado $st"
        switch $st {
            9 { after time 1 { tap 8 0x01 } }
            7 { after time 3 hubkeys }
            2 { incr ::laps; after time 1.5 { tap 8 0x01 } }
        }
    }
    if {$::laps >= $::env(LAPS)} { say "fim"; close $::log; exit }
    after time 0.1 poll
}
after time 5 poll
after time 400 { say "timeout"; close $::log; exit }
