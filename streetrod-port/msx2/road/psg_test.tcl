set throttle off
set ::f [open $::env(OUT) w]
proc samp {} {
  set l "t=[format %6.2f [machine_info time]]"
  foreach r {0 1 2 3 4 5 6 7 8 9 10} { append l " [debug read {PSG regs} $r]" }
  puts $::f $l; flush $::f
  after time 0.25 samp
}
after time 14 samp
after time 40 { close $::f; exit }
