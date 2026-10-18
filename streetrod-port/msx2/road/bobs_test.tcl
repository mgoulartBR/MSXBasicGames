# percorre o Bob's com capturas (lista de pilotos, carro, tipo de corrida, aposta, aceite)
set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
set ::seq {}; set ::t 8.0
proc kk {k} { lappend ::seq [list $::t $k]; set ::t [expr {$::t + 1.5}] }
proc sn {n} { lappend ::seq [list $::t "snap $n"]; set ::t [expr {$::t + 0.3}] }
kk space; set ::t 15
kk space; sn list                      ;# Bobs -> lista de pilotos
kk down; kk down; kk down; sn list2
kk space; sn info                      ;# quem e / carro
kk space; sn types                     ;# tipo de corrida
kk down; kk down; kk down; kk space; sn pinkreq   ;# pink slip (sem vitorias: recusa)
kk space; sn types2
kk up; kk up; kk up; kk space; sn bets ;# drag race -> aposta
kk space; sn accept
foreach s $::seq {
    set at [lindex $s 0]; set k [lindex $s 1]
    switch -glob $k {
        space { after time $at { tap 8 0x01 } }
        up    { after time $at { tap 8 0x20 } }
        down  { after time $at { tap 8 0x40 } }
        snap* { after time $at [list snap [lindex $k 1]] }
    }
}
after time [expr {$::t + 1}] { exit }
