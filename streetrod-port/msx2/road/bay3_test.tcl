# motor aberto direto (-DBAY_OPEN, cursor sobre PARTS): snapshot do motor, PARTS, de volta ao motor
set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
after time 8 { tap 8 0x01 }
after time 16 { snap e1 }
after time 17 { tap 8 0x01 }
after time 21 { snap e2 }
after time 22 { tap 7 0x04 }
after time 25 { snap e3 }
after time 26 { exit }
