set throttle on
set frameskip 0
foreach {t n} {5.5 a 6.5 b 8 c} { after time $t [list screenshot $::env(SHOTNAME)_$n.png] }
after time 8.5 { exit }
