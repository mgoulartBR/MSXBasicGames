# jornal: carros usados -> comprar; pecas -> comprar; vender pecas; capturas em env VRAM
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
set ::seq {}
set ::t 8.0
proc kk {k} { lappend ::seq [list $::t $k]; set ::t [expr {$::t + 0.7}] }
proc ss {n} { lappend ::seq [list $::t "snap $n"]; set ::t [expr {$::t + 0.5}] }
# 8: titulo->hub (carrega ~3 s)
kk space; set ::t 14
kk up; kk space                     ;# NEWSPAPER -> menu do jornal
ss news1
kk space; ss used1                 ;# USED CARS
kk down; kk down; ss used2
kk space; ss buy1                  ;# menu de compra
kk space; set ::t [expr {$::t + 0.5}]; ss bought                 ;# compra ao preco pedido
kk space; kk space                  ;# fecha nota/volta
set ::t [expr {$::t + 1}]
ss news2
kk down; kk space; ss cats       ;# AUTO PARTS
kk space; ss eng                   ;# ENGINES
kk down; kk down; kk down; kk space; ss bought2
kk space; kk shift; kk shift; ss news3
kk down; kk down; kk space; ss sell
foreach s $::seq {
    set at [lindex $s 0]; set k [lindex $s 1]
    switch -glob $k {
        space { after time $at { tap 8 0x01 } }
        up    { after time $at { tap 8 0x20 } }
        down  { after time $at { tap 8 0x40 } }
        shift { after time $at { tap 6 0x01 } }
        snap* { after time $at [list snap [lindex $k 1]] }
    }
}
after time [expr {$::t + 1}] { exit }
