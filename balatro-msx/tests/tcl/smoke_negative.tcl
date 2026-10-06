# Black Deck (+1 Joker slot), Negative Jokers / consumables, overlapping Joker row (debug ROM)
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set b 7.0
key [expr {$b + 1.5}] space          ;# title -> deck screen
key [expr {$b + 2.6}] up             ;# stake row
key [expr {$b + 3.0}] up             ;# deck row
key [expr {$b + 3.4}] left           ;# Anaglyph
key [expr {$b + 3.8}] left           ;# Nebula
key [expr {$b + 4.2}] left           ;# Magic
key [expr {$b + 4.6}] left           ;# Black
snap [expr {$b + 5.0}] 95_black_deck
key [expr {$b + 5.2}] down
key [expr {$b + 5.6}] down           ;# Start
key [expr {$b + 6.0}] space
key [expr {$b + 7.5}] space          ;# select the blind
at [expr {$b + 9.5}] { check "Black Deck: deck id 11, 6 Joker slots, 3 hands" {[bk 24] == 11 && [bk 32] == 6 && [bk 6] == 3} }
key [expr {$b + 10.0}] minus
key [expr {$b + 10.4}] minus
key [expr {$b + 10.8}] minus
key [expr {$b + 11.2}] minus
key [expr {$b + 11.6}] minus
at [expr {$b + 12.2}] { check "five Jokers fit with the 6th slot" {[bk 9] == 5 && [bk 36] == 22} }
snap [expr {$b + 12.4}] 96_black_jokers
key [expr {$b + 12.8}] equal         ;# every Joker Negative + a Negative Planet
at [expr {$b + 13.6}] { check "Negative Jokers raise the slots (8 max), Negative consumable adds a slot" {[bk 32] == 8 && [bk 33] == 3 && [bk 34] == 1} }
at [expr {$b + 13.7}] { check "Joker row overlaps (pitch < 24)" {[bk 36] < 24} }
snap [expr {$b + 13.8}] 97_negative_row
# the pointer teleports to the next widget at every press of "6": log which widget is focused (Jokers are ids 16..23)
for {set k 0} {$k < 12} {incr k} {
    key [expr {$b + 14.2 + $k * 0.5}] 6
    at [expr {$b + 14.5 + $k * 0.5}] "puts \$::RES \"INFO focus after press [expr {$k + 1}]: \[bk 39\]\"; flush \$::RES; screenshot \$::env(SHOT_OUT)/98_hover_$k.png"
}
finish [expr {$b + 21.5}]
