# consequencias: corrida (quebra forcada com -DTEST_BRK=1) -> mensagem -> garagem: Bobs recusa -> FIX IT -> Bobs aceita; registra dinheiro/estado
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {name} { save_debuggable VRAM $::env(VRAM).$name; set f [open $::env(VRAM).$name.r2 w]; puts $f [debug read "VDP regs" 2]; close $f }
set ::log [open $::env(OUT) w]
proc say {s} { puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] res=[w 0xF018] money=[w 0xF030] day=[w 0xF032] cars=[w 0xF034] | $s"; flush $::log }
set ::last -1
proc hubkeys {} { if {[w 0xF014] == 7} { tap 8 0x01; after time 1.6 hubkeys } }
set ::laps 0
proc poll {} {
    set st [w 0xF014]
    if {$st != $::last} {
        set ::last $st; say "estado $st"
        switch $st {
            9 { after time 1 { tap 8 0x01 } }
            7 { if {$::laps == 0} { after time 3 hubkeys } }
            2 { incr ::laps; after time 1.5 { tap 8 0x01 }; after time 7 { snap msg; say "mensagem" } }
        }
    }
    if {$::laps >= 1 && $st == 7 && $::last == 7} { }
    after time 0.1 poll
}
after time 5 poll
# depois da corrida (garagem): Bobs (carro quebrado) -> mensagem; YOUR CARS -> FIX IT
set base 0
proc stage2 {} {
    say "garagem apos quebra"
    after time 2.5 { tap 8 0x01 }           ;# fecha mensagem
    after time 5 { tap 8 0x01; say "tentou Bobs (deve recusar)"; snap notrun }
    after time 7.5 { tap 8 0x01 }           ;# fecha aviso
    after time 9 { tap 8 0x20 }; after time 10 { tap 8 0x20 }; after time 11 { tap 8 0x01 }   ;# YOUR CARS
    after time 13 { tap 8 0x01 }            ;# 1o carro
    after time 15 { tap 8 0x40; }; after time 16 { tap 8 0x40 }; after time 17 { tap 8 0x40 }; after time 18 { tap 8 0x40 }
    after time 19 { snap menufix; tap 8 0x01 }     ;# FIX OR JUNK IT
    after time 21 { snap fixmenu; tap 8 0x01 }     ;# FIX IT
    after time 23 { say "pagou conserto"; snap fixed }
}
proc wait2 {} { if {$::laps >= 1 && [w 0xF014] == 7} { after time 0.5 stage2 } else { after time 0.5 wait2 } }
after time 30 wait2
after time 180 { say "fim"; close $::log; exit }
