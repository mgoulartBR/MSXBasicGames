# clica no objeto sob o cursor inicial (-DCUR_X/-DCUR_Y) e guarda a VRAM do resultado
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
set ::go 0
proc poll {} { if {[w 0xF014] == 9 && !$::go} { set ::go 1; after time 1 { tap 8 0x01 } }
  if {[w 0xF014] == 7} { after time 1 { tap 8 0x01 }; after time 4 { snap res; exit }; return }; after time 0.2 poll }
after time 5 poll
