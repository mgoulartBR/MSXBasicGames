# Deck / stake selection screen (debug ROM): change the deck and the stake with the keyboard, start the run
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set b 7.0
key [expr {$b + 1.5}] space
at [expr {$b + 2.2}] { check "title -> deck screen" {[bk 0] == $::SC(deck)} }
snap [expr {$b + 2.3}] 90_deck_screen
key [expr {$b + 2.6}] up            ;# stake row
key [expr {$b + 3.0}] up            ;# deck row
key [expr {$b + 3.4}] right         ;# Blue Deck
key [expr {$b + 3.8}] down
key [expr {$b + 4.2}] right         ;# Red Stake
key [expr {$b + 4.6}] right         ;# Green Stake
snap [expr {$b + 5.0}] 91_deck_chosen
key [expr {$b + 5.2}] down          ;# Start
key [expr {$b + 5.6}] space
at [expr {$b + 7.0}] { check "run started with Blue Deck / Green Stake" {[bk 0] == $::SC(blind) && [bk 24] == 1 && [bk 25] == 2} }
snap [expr {$b + 7.1}] 92_blind_green
key [expr {$b + 7.5}] space
at [expr {$b + 9.0}] { check "Blue Deck gives 5 hands" {[bk 0] == $::SC(round) && [bk 6] == 5} }
finish [expr {$b + 10}]
