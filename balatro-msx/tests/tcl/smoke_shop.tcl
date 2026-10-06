source [file join [file dirname [info script]] lib.tcl]
set b 7.0
key [expr {$b + 1.5}] space       ;# title -> deck screen
key [expr {$b + 2.5}] space       ;# deck screen: Start
key [expr {$b + 3.5}] space       ;# select small blind
key [expr {$b + 5.0}] 3           ;# +$50
key [expr {$b + 5.4}] 2           ;# random joker
key [expr {$b + 5.8}] 2
key [expr {$b + 6.2}] 4           ;# planet + tarot
snap [expr {$b + 6.8}] 10_round_items
key [expr {$b + 7.0}] 1           ;# win the round instantly
snap [expr {$b + 9.0}] 11_banner
snap [expr {$b + 11.0}] 12_cashout
snap [expr {$b + 13.0}] 13_cashout_done
key [expr {$b + 13.5}] space      ;# cash out
snap [expr {$b + 15.0}] 14_shop
finish [expr {$b + 16}]
