# compra um carro (dinheiro de teste), uma peca, vende a peca; registra estado nas variaveis de depuracao
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] money=[w 0xF030] cars=[w 0xF034] spare=[w 0xF036] | $s"; flush $::log }
set ::seq {}; set ::t 8.0
proc kk {k} { lappend ::seq [list $::t $k]; set ::t [expr {$::t + 1.4}] }
proc lg {s} { lappend ::seq [list $::t "say $s"]; set ::t [expr {$::t + 0.3}] }
kk space; set ::t 15
kk up; kk space; lg "menu do jornal"
kk down; kk space; lg "categorias de pecas"
kk space; lg "lista de motores"
kk space; lg "comprou 1a peca (V-6)"
kk space; lg "nota fechada"
kk down; kk space; lg "comprou 2a peca"
kk space; lg "nota fechada"
kk shift; lg "categorias"
kk shift; lg "menu jornal"
kk down; kk space; lg "lista de pecas para vender"
kk space; lg "vendeu 1"
kk shift; lg "menu jornal"
kk up; kk up; kk space; lg "carros usados"
kk space; lg "menu de compra"
kk space; lg "comprou carro?"
kk space; lg "nota IT'S YOURS fechada"
foreach s $::seq {
    set at [lindex $s 0]; set k [lindex $s 1]
    switch -glob $k {
        space { after time $at { tap 8 0x01 } }
        up    { after time $at { tap 8 0x20 } }
        down  { after time $at { tap 8 0x40 } }
        shift { after time $at { tap 6 0x01 } }
        say*  { after time $at [list say [lrange $k 1 end]] }
    }
}
after time [expr {$::t + 1}] { close $::log; exit }
