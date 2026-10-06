# Nebula Deck: Telescope voucher, one consumable slot (debug ROM)
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set t [start_run 7.0 2]
at $t { check "Nebula Deck: id 13, Telescope (voucher bit 10), 1 consumable slot" {[bk 24] == 13 && ([bk 38] & 4) && [bk 33] == 1} }
key [expr {$t + 0.5}] space
key [expr {$t + 3.0}] 0                                      ;# a random Spectral card
key [expr {$t + 3.6}] 0                                      ;# a 2nd one does not fit: still 1 slot
at [expr {$t + 5.0}] { check "Nebula Deck: a 2nd consumable does not fit (1 slot)" {[bk 40] == 1 && [bk 33] == 1} }
snap [expr {$t + 5.2}] 101_nebula_deck
finish [expr {$t + 6.0}]
