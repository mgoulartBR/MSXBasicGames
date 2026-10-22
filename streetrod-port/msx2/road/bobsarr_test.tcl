# Bob's: fotografa a chegada do carro (garagem -> porta da esquerda -> Bob's, sem a viagem: -DSKIP_STREET)
set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f; set f [open $::env(SHOTDIR)/$n.vram.r2 w]; puts $f [debug read {VDP regs} 2]; close $f }
after time 8 { tap 8 0x01 }
after time 14 { keymatrixdown 8 0x10 }
after time 16 { keymatrixup 8 0x10 }
after time 17 { tap 8 0x01 }
foreach t {24 36 48 60 72 84} { after time $t "snap a$t" }
after time 86 { exit }
