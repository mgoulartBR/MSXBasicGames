# Newest Jokers (art in the last ROM segments, streamed by HMMC) and Spectral cards
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set b 7.0
key [expr {$b + 1.5}] space
key [expr {$b + 2.5}] space       ;# deck screen: Start
key [expr {$b + 3.5}] space
key [expr {$b + 5.0}] minus
key [expr {$b + 5.4}] minus
key [expr {$b + 5.8}] minus
key [expr {$b + 6.2}] minus
key [expr {$b + 6.6}] minus
key [expr {$b + 7.0}] 0
key [expr {$b + 7.4}] 0
key [expr {$b + 8.2}] up
snap [expr {$b + 8.8}] 80_new_jokers
at [expr {$b + 9.0}] { check "five newest jokers owned" {[bk 9] == 5} }
finish [expr {$b + 10}]
