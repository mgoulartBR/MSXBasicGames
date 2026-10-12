set throttle off
set frameskip 0
foreach {t n} {14 a 16 b 18 c 20 d 22 e 24 f} { after time $t [list screenshot shot_$n.png] }
after time 25 { exit }
