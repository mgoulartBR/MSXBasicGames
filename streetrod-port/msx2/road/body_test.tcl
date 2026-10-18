# oficina: clica no compressor e faz pintura (verde), adesivo, teto rebaixado e sem para-choques; guarda a VRAM final e a de cada etapa
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram; set f [open $::env(SHOTDIR)/$n.vram.pal w]; for {set i 0} {$i<32} {incr i} {puts -nonewline $f "[debug read {VDP palette} $i] "}; close $f }
proc say {s} { set f [open $::env(OUT) a]; puts $f "t=[machine_info time] state=[w 0xF014] money=[w 0xF030] $s"; close $f }
set ::seq {}; set ::t 16.0
proc kk {k {d 1.3}} { lappend ::seq [list $::t $k]; set ::t [expr {$::t + $d}] }
proc go {} {
  kk "say start" 0.2
  kk space 2        ;# compressor -> menu
  kk "snap m0" 0.3
  kk space; kk down; kk down; kk space 3     ;# NEW PAINT JOB -> terceira cor
  kk "say after_paint" 0.5
  kk "snap s1" 0.5
  kk space; kk down; kk space; kk down; kk down; kk down; kk space 3   ;# STICKERS -> 4o
  kk "snap s2" 0.5
  kk space; kk down; kk down; kk space 3     ;# CHOP ROOF
  kk "snap s3" 0.5
  kk space; kk down; kk down; kk down; kk space 3    ;# STRIP BUMPERS
  kk "snap s4" 0.5
  foreach s $::seq { set at [lindex $s 0]; set k [lindex $s 1]
    switch -glob $k { space { after time $at { tap 8 0x01 } } down { after time $at { tap 8 0x40 } } snap* { after time $at [list snap [lindex $k 1]] } say* { after time $at [list say [lindex $k 1]] } } }
  after time [expr {$::t + 2}] { exit }
}
after time 8 { tap 8 0x01 }
go
