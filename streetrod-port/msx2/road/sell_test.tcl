# vende o carro inicial na garagem (aceita a oferta) e confere dinheiro/carros
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] money=[w 0xF030] cars=[w 0xF034] | $s"; flush $::log }
set ::seq {}; set ::t 8.0
proc kk {k} { lappend ::seq [list $::t $k]; set ::t [expr {$::t + 1.5}] }
proc lg {s} { lappend ::seq [list $::t "say $s"]; set ::t [expr {$::t + 0.4}] }
kk space; set ::t 15
kk up; kk up; kk space; lg "YOUR CARS"
kk space; lg "menu do carro"
kk down; kk down; kk down; kk down; kk space; lg "SELL IT"
kk space; lg "aceita oferta"
kk space; kk shift; lg "de volta"
foreach s $::seq {
    set at [lindex $s 0]; set k [lindex $s 1]
    switch -glob $k {
        space { after time $at { tap 8 0x01 } }
        up    { after time $at { tap 8 0x20 } }
        down  { after time $at { tap 8 0x40 } }
        shift { after time $at { tap 7 0x04 } }
        say*  { after time $at [list say [lrange $k 1 end]] }
    }
}
after time [expr {$::t + 1}] { close $::log; exit }
