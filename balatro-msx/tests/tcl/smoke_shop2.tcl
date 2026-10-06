source [file join [file dirname [info script]] lib.tcl]
set b 7.0
key [expr {$b + 1.5}] space
key [expr {$b + 2.5}] space       ;# deck screen: Start
key [expr {$b + 3.5}] space
key [expr {$b + 5.0}] 3
key [expr {$b + 5.4}] 3
key [expr {$b + 5.8}] 1                 ;# instant win
key [expr {$b + 9.5}] space             ;# (skip cash-out reveal)
key [expr {$b + 10.2}] space            ;# cash out
snap [expr {$b + 11.5}] 40_shop
key [expr {$b + 12.0}] down             ;# default focus: first shop card
snap [expr {$b + 12.6}] 41_shop_focus
key [expr {$b + 13.0}] space            ;# buy it
snap [expr {$b + 13.8}] 42_shop_bought
key [expr {$b + 14.2}] down             ;# to the packs
key [expr {$b + 14.6}] space            ;# open the pack
snap [expr {$b + 16.0}] 43_pack
key [expr {$b + 16.4}] right
snap [expr {$b + 17.0}] 44_pack_focus
key [expr {$b + 17.4}] space            ;# take a card
snap [expr {$b + 18.4}] 45_pack_after
finish [expr {$b + 19}]
