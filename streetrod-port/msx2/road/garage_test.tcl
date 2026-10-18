# compra um carburador de 4 barrilhetes e instala no carro inicial; compara a potencia (CAR INFO) antes/depois
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] money=[w 0xF030] cars=[w 0xF034] spare=[w 0xF036] power=[w 0xF038] B=[w 0xF03A] wear=[w 0xF03C] ptei=[format %x [w 0xF03E]] | $s"; flush $::log }
set ::seq {}; set ::t 8.0
proc kk {k} { lappend ::seq [list $::t $k]; set ::t [expr {$::t + 1.5}] }
proc lg {s} { lappend ::seq [list $::t "say $s"]; set ::t [expr {$::t + 0.4}] }
proc sn {n} { lappend ::seq [list $::t "snap $n"]; set ::t [expr {$::t + 0.4}] }
kk space; set ::t 15
kk up; kk space                 ;# jornal
kk down; kk space               ;# auto parts
kk down; kk down; kk space      ;# carburadores
kk down; kk space; lg "comprou carb 4-brl"; kk space   ;# 2o item
kk shift; kk shift; kk shift    ;# volta ao hub
kk up; kk space; sn carlist     ;# YOUR CARS
kk space; sn carmenu            ;# primeiro carro
kk down; kk down; kk down; kk space; sn info1; lg "info antes"; kk shift
kk up; kk up; kk space; sn parts   ;# CAR PARTS
kk down; kk down; kk space; sn spare   ;# CARBURETOR -> spare list
kk space; lg "instalou"; kk space; kk shift   ;# instala, fecha nota, volta ao menu do carro
kk down; kk down; kk space; sn info2; lg "info depois"
foreach s $::seq {
    set at [lindex $s 0]; set k [lindex $s 1]
    switch -glob $k {
        space { after time $at { tap 8 0x01 } }
        up    { after time $at { tap 8 0x20 } }
        down  { after time $at { tap 8 0x40 } }
        shift { after time $at { tap 7 0x04 } }
        say*  { after time $at [list say [lrange $k 1 end]] }
        snap* { after time $at [list snap [lindex $k 1]] }
    }
}
after time [expr {$::t + 1}] { close $::log; exit }
