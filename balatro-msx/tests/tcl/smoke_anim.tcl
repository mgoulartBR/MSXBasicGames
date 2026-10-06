source [file join [file dirname [info script]] lib.tcl]
set b 7.0
key [expr {$b + 1.5}] space
key [expr {$b + 2.5}] space       ;# deck screen: Start
key [expr {$b + 3.5}] space
key [expr {$b + 5.0}] 2
key [expr {$b + 5.4}] 2
key [expr {$b + 5.8}] 2
key [expr {$b + 6.2}] 4
key [expr {$b + 6.8}] right
key [expr {$b + 7.2}] space
key [expr {$b + 7.6}] right
key [expr {$b + 8.0}] space
key [expr {$b + 8.4}] right
key [expr {$b + 8.8}] space
snap [expr {$b + 9.4}] 50_ready
key [expr {$b + 9.6}] p
for {set i 0} {$i < 8} {incr i} { snap [expr {$b + 10.3 + $i*0.55}] 51_anim_$i }
snap [expr {$b + 16.0}] 52_done
finish [expr {$b + 17}]
