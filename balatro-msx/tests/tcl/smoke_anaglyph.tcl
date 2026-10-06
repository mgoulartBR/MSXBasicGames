# Anaglyph Deck + Double Tag icon on the Blind select screen (debug ROM)
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set t [start_run 7.0 1]
at $t { check "Anaglyph Deck: id 14" {[bk 24] == 14} }
key [expr {$t + 0.4}] backslash                               ;# debug: a Double Tag
at [expr {$t + 1.2}] { check "a Double Tag is held" {[bk 35] == 1} }
snap [expr {$t + 1.5}] 102_double_tag_hud
finish [expr {$t + 2.5}]
