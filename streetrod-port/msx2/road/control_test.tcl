# Teste do controle manual: acelera (cursor cima), troca marchas com ESPACO, depois freia (cursor baixo) e vira (esquerda).
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc sw {a} { set v [w $a]; if {$v > 32767} {set v [expr {$v - 65536}]}; return $v }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %.1f [machine_info time]] $s"; flush $::log }
set ::shifting 0
proc pollshift {} {
    set rpm [w 0xF010]; set g [w 0xF012]
    if {!$::shifting && $g >= 1 && $g < 4 && $rpm > 4700} {
        set ::shifting 1
        keymatrixdown 8 0x01
        after time 0.15 { keymatrixup 8 0x01; after time 0.4 { set ::shifting 0 } }
    }
    after time 0.05 pollshift
}
after time 6 { keymatrixdown 8 0x20; say "acelerando"; pollshift }
after time 22 { say "gear=[w 0xF012] rpm=[w 0xF010] mph=[w 0xF006] t60=[w 0xF00A] t100=[w 0xF00C] top=[w 0xF00E]" }
after time 22.5 { set ::P0 [sw $::env(AP)]; keymatrixdown 8 0x10; say "virando a esquerda P=$::P0" }
after time 24.5 { keymatrixup 8 0x10; say "P=[sw $::env(AP)] (esperado: menor que $::P0) mph=[w 0xF006]" }
after time 25 { keymatrixup 8 0x20; keymatrixdown 8 0x40; say "freando mph=[w 0xF006] gear=[w 0xF012]" }
after time 28 { say "freado mph=[w 0xF006] gear=[w 0xF012] rpm=[w 0xF010]"; keymatrixup 8 0x40; close $::log; exit }
