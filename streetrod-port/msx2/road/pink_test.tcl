# pink slip: vence/perde conforme o build (-DTEST_LOSE); registra carros/dinheiro; usa Espaco repetido no Bobs com tipo escolhido por teclas
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] res=[w 0xF018] money=[w 0xF030] cars=[w 0xF034] | $s"; flush $::log }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
set ::seq {}; set ::t 8.0
proc kk {k} { lappend ::seq [list $::t $k]; set ::t [expr {$::t + 1.5}] }
proc lg {s} { lappend ::seq [list $::t "say $s"]; set ::t [expr {$::t + 0.3}] }
proc sn {n} { lappend ::seq [list $::t "snap $n"]; set ::t [expr {$::t + 0.3}] }
kk space; set ::t 15
kk space; kk space; kk space          ;# Bobs, piloto, nota
kk down; kk down; kk down; kk space   ;# PINK SLIPS
kk space; lg "aceitou pink slip"      ;# nota "cool dude"
set ::t [expr {$::t + 60}]
lg "depois da corrida"
sn after
foreach s $::seq {
    set at [lindex $s 0]; set k [lindex $s 1]
    switch -glob $k {
        space { after time $at { tap 8 0x01 } }
        down  { after time $at { tap 8 0x40 } }
        say*  { after time $at [list say [lrange $k 1 end]] }
        snap* { after time $at [list snap [lindex $k 1]] }
    }
}
# resultado: o jogo volta a garagem sozinho com Espaco
proc poll {} { if {[w 0xF014] == 2} { after time 4.5 { tap 8 0x01 } ; after time 9 { say "na garagem (msg do resultado)"; snap msg }; return }; after time 0.3 poll }
after time 20 poll
after time 100 { say "fim"; close $::log; exit }
