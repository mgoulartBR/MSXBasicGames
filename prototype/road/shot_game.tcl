set throttle on
set frameskip 0
foreach {t n} {8 a 10 b 12 c} { after time $t [list screenshot $::env(SHOTNAME)_$n.png] }
after time 13 { exit }
