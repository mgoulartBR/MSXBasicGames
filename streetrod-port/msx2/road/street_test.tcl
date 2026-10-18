# rua: hub -> Bob's -> "CRUISING IN TOWN" -> dirige (piloto automatico) -> multa/chegada -> menu do Bob's; registra estado/dinheiro/gasolina
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] pos=[w 0xF01E] money=[w 0xF030] | $s"; flush $::log }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
foreach {at k} {8 space 14 space 18 space 22 space 26 space} { after time $at { tap 8 0x01 } }
after time 20 { say "dirigindo"; snap drive }
after time 45 { say "dirigindo2"; snap drive2 }
after time 60 { say "depois"; snap after }
after time 64 { tap 8 0x01 }
after time 70 { say "fim"; snap end; close $::log; exit }
