# Autosave / Continue (needs the SRAM mapper: ROMTYPE=ASCII8SRAM2 and an empty ~/.openMSX/persistent)
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set b 7.0
at [expr {$b + 1.0}] { check "SRAM detected, no save yet" {[bk 28] == 1 && [bk 29] == 0} }
key [expr {$b + 1.5}] space
key [expr {$b + 2.5}] space                 ;# deck screen: Start
key [expr {$b + 3.5}] space                 ;# select the small blind
key [expr {$b + 5.5}] right
key [expr {$b + 5.9}] space
key [expr {$b + 6.3}] right
key [expr {$b + 6.7}] space
key [expr {$b + 7.1}] d                     ;# discard two cards
at [expr {$b + 9.5}] { check "one discard used before the reset" {[bk 0] == $::SC(round) && [bk 30] == 1} }
at [expr {$b + 9.6}] { set ::D [bk 7]; set ::H [bk 8]; set ::P [bk 17]; set ::J [bk 31]; reset }
# after the reset the boot takes ~7 s again
set r [expr {$b + 9.6 + 7.5}]
at [expr {$r}] { check "a saved run is offered on the title screen" {[bk 0] == $::SC(title) && [bk 29] == 1} }
snap [expr {$r + 0.2}] 95_continue
key [expr {$r + 0.5}] space                 ;# Continue
at [expr {$r + 2.5}] { check "continued: round restored (discards, hand, deck pile)" {[bk 0] == $::SC(round) && [bk 7] == $::D && [bk 8] == $::H && [bk 17] == $::P && [bk 30] == 1} }
snap [expr {$r + 2.6}] 96_continued
finish [expr {$r + 3.5}]
