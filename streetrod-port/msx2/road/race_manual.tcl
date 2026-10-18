# Fluxo manual guiado pelo estado do jogo (F014: 9 titulo, 0 contagem, 1 corrida, 2 fim):
# titulo -> ESPACO -> contagem -> engata antes do verde (largada falsa) -> ESPACO reinicia -> corrida com trocas manuais.
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] res=[w 0xF018] pos=[w 0xF008] mph=[w 0xF006] gear=[w 0xF012] rpm=[w 0xF010] time10=[w 0xF01A] in=[format %x [w 0xF020]] | $s"; flush $::log }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.04 [list keymatrixup $row $mask] }
set ::step 0; set ::shifting 0; set ::wait 0
proc poll {} {
    set st [w 0xF014]; set g [w 0xF012]; set rpm [w 0xF010]
    switch $::step {
        0 { if {$st == 9} { say "titulo visivel: ESPACO"; tap 8 0x01; set ::step 9 } }
        9 { if {$st == 7} { say "hub: Bobs Drive-In"; after time 3.0 { tap 8 0x01 }; set ::step 10 } }
        10 { if {$st == 8} { say "menu visivel: ESPACO"; after time 2.0 { tap 8 0x01; keymatrixdown 8 0x20 }; set ::step 1 } }
        1 { if {$st == 0} { say "contagem: engata antes do verde"; after time 1.0 { say "vai bater ESPACO"; tap 8 0x01; after time 0.05 { say "espaco apertado" } }; set ::step 2 } }
        2 { if {$st == 2} { say "esperado res=3 (largada falsa)"; after time 0.8 { tap 8 0x01 }; set ::step 11 } }
        11 { if {$st == 7} { say "hub de novo"; after time 3.0 { tap 8 0x01 }; set ::step 12 } }
        12 { if {$st == 8} { say "menu de novo: ESPACO"; after time 2.0 { tap 8 0x01 }; set ::step 3 } }
        3 { if {$st == 0 && [incr ::wait] > 5} { say "reiniciou: nova contagem"; set ::step 4 } }
        4 { if {$st == 1 && !$::shifting && $g >= 1 && $g < 4 && $rpm > 4700} { set ::shifting 1; tap 8 0x01; after time 0.5 { set ::shifting 0 } }
            if {$st == 2} { say "FIM DA CORRIDA"; close $::log; exit } }
    }
    after time 0.05 poll
}
after time 3 poll
after time 110 { say "timeout"; close $::log; exit }
