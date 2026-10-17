set throttle off
set ::samples {}
proc sample {} {
  lappend ::samples [reg PC]
  after time 0.011 sample
}
after time 8 { sample }
after time 14 {
  set fd [open $::env(PROF) w]; foreach p $::samples { puts $fd $p }; close $fd
  screenshot -raw $::env(SHOT)
  exit
}
