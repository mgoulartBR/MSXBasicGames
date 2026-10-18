# navega no menu (setas), inicia a corrida e salva capturas (env VRAM)
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set seq {}
# tempo (s) -> tecla: espaco no titulo, depois menu: direita x2, baixo, direita x13 (Corvette), baixo, direita x3, espaco
set t 8.0; lappend seq [list $t 0x01]
set t 11.0
foreach k {0x80 0x80 0x40} { lappend seq [list $t $k]; set t [expr {$t + 0.5}] }
for {set i 0} {$i < 13} {incr i} { lappend seq [list $t 0x80]; set t [expr {$t + 0.45}] }
lappend seq [list $t 0x40]; set t [expr {$t + 0.5}]
for {set i 0} {$i < 3} {incr i} { lappend seq [list $t 0x80]; set t [expr {$t + 0.5}] }
set t [expr {$t + 1.5}]
set menu_shot $t
lappend seq [list $t 0x01]
foreach s $seq { after time [lindex $s 0] [list tap 8 [lindex $s 1]] }
after time [expr {$menu_shot - 0.5}] { save_debuggable VRAM $::env(VRAM).menu; set f [open $::env(VRAM).menu.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
after time [expr {$menu_shot + 12}] { save_debuggable VRAM $::env(VRAM).race; set f [open $::env(VRAM).race.r2 w]; puts $f [debug read "VDP regs" 2]; close $f; exit }
