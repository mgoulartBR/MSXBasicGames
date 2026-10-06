# Magic Deck: Crystal Ball (3 consumable slots) + 2 copies of The Fool (debug ROM)
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set t [start_run 7.0 3]
at $t { check "Magic Deck: id 12, Crystal Ball (voucher bit 9), 3 consumable slots" {[bk 24] == 12 && ([bk 38] & 2) && [bk 33] == 3} }
key [expr {$t + 0.5}] space                                   ;# select the Small Blind
at [expr {$t + 2.5}] { check "Magic Deck: both Fools in the consumable row" {[bk 0] == $::SC(round) && [bk 33] == 3 && [bk 40] == 2} }
snap [expr {$t + 2.7}] 100_magic_deck
finish [expr {$t + 3.5}]
