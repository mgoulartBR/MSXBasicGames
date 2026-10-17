# Fluxo manual: (1) largada falsa (engata antes do verde) -> resultado 3; (2) ESPACO reinicia; (3) corrida normal com trocas manuais.
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] res=[w 0xF018] pos=[w 0xF008] opos=[w 0xF016] mph=[w 0xF006] gear=[w 0xF012] rpm=[w 0xF010] time10=[w 0xF01A] avg=[w 0xF01C] | $s"; flush $::log }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
set ::phase 0
set ::shifting 0
proc poll {} {
    set st [w 0xF014]; set g [w 0xF012]; set rpm [w 0xF010]
    if {$::phase == 2 && $st == 1 && !$::shifting && $g >= 1 && $g < 4 && $rpm > 4700} {
        set ::shifting 1; tap 8 0x01; after time 0.5 { set ::shifting 0 }
    }
    if {$::phase == 2 && $st == 2} { say "FIM DA CORRIDA"; close $::log; exit }
    after time 0.05 poll
}
after time 6 { keymatrixdown 8 0x20; say "fase 1: acelera, engata antes do verde"; after time 0.7 { tap 8 0x01 } }
after time 8 { say "esperado state=2 res=3 (largada falsa)"; tap 8 0x01 }
after time 9.5 { say "apos ESPACO: deve reiniciar (state=0)"; set ::phase 2; poll }
after time 90 { say "timeout"; close $::log; exit }
