# percorre o hub: titulo -> garagem (menu principal) -> seus carros -> Bobs -> corrida; salva capturas (env VRAM)
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] | $s"; flush $::log }
set t 8
after time $t { tap 8 0x01; say "titulo -> hub" }
after time 14 { snap hub1; say "hub: menu principal" }
after time 15 { tap 8 0x20 }
after time 15.6 { tap 8 0x20 }
after time 16.5 { snap hub2; say "seleciona YOUR CARS" }
after time 17 { tap 8 0x01 }
after time 18.5 { snap cars; say "lista de carros" }
after time 19 { tap 8 0x01 }
after time 20 { snap hub3; say "voltou ao menu" }
after time 21 { tap 8 0x40 }
after time 21.6 { tap 8 0x40 }
after time 22.2 { tap 8 0x01; say "NEWSPAPER?" }
after time 24 { snap news; say "jornal" }
after time 25 { tap 8 0x01 }
after time 26 { tap 8 0x40 }
after time 26.6 { tap 8 0x01; say "Bobs" }
after time 29 { snap bobs; say "menu Bobs" }
after time 30 { tap 8 0x01 }
after time 40 { snap race; say "corrida" ; close $::log; exit }
