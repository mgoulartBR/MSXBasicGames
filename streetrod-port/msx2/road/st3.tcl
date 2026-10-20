set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::log [open $::env(OUT) w]
foreach {at k} {8 space 14 space 18 space} { after time $at { tap 8 0x01 } }
set ::last -1
proc poll {} {
    set st [w 0xF014]
    if {$st != $::last} { set ::last $st; puts $::log "t=[format %5.1f [machine_info time]] state=$st spd=[w 0xF006] pos=[w 0xF008] money=[w 0xF030]"; flush $::log }
    after time 0.5 poll
}
after time 5 poll
foreach t {30 40 50 60 70} { after time $t "puts \$::log \"  t=$t state=\[w 0xF014\] mph=\[w 0xF006\] pos=\[w 0xF008\] rpm=\[w 0xF010\] gear=\[w 0xF012\]\"; flush \$::log" }
after time 100 { close $::log; exit }
