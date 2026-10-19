set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.12 [list keymatrixup $row $mask] }
set ::f [open $::env(OUT) w]
set ::base ""
proc samp {} {
  set r2 [vdpreg 2]; set pg [expr {($r2 >> 5) & 3}]
  set a [expr {$pg * 0x8000 + 125 * 128}]
  set n 0; set l ""
  for {set i 0} {$i < 128} {incr i 4} { set v [debug read VRAM [expr {$a + $i}]]; if {$::base ne ""} { if {$v != [lindex $::base [expr {$i/4}]]} {incr n} }; lappend l $v }
  if {$::base eq ""} { set ::base $l }
  puts $::f "t=[format %7.3f [machine_info time]] pg=$pg diff=$n"; flush $::f
  after time 0.02 samp
}
after time 8 { tap 8 0x01 }
after time 16 { tap 8 0x01 }
after time 16.2 samp
after time 21 { close $::f; exit }
