# Full-flow smoke test (debug ROM): title -> blind -> round -> play/discard -> win -> cash out -> shop -> next blind -> lose.
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set b 7.0
at [expr {$b + 1.0}] { check "boot: title screen" {[bk 0] == $::SC(title)} }
snap [expr {$b + 1.2}] s01_title
key [expr {$b + 1.5}] space
at [expr {$b + 3.0}] { check "title -> blind select" {[bk 0] == $::SC(blind) && [bk 2] == 1 && [bk 3] == 0} }
snap [expr {$b + 3.2}] s02_blind
key [expr {$b + 3.5}] space
at [expr {$b + 5.0}] { check "blind select -> round, 8 cards, 4 hands, 3 discards" {[bk 0] == $::SC(round) && [bk 8] == 8 && [bk 6] == 4 && [bk 7] == 3 && [bk 17] == 44} }
snap [expr {$b + 5.2}] s03_round
# discard two cards: move to the first card, select two, press D
key [expr {$b + 5.5}] right
key [expr {$b + 5.9}] space
key [expr {$b + 6.3}] right
key [expr {$b + 6.7}] space
at [expr {$b + 7.0}] { check "two cards selected" {[bk 16] == 3} }
key [expr {$b + 7.2}] d
at [expr {$b + 8.0}] { check "discard: 2 discards left, hand refilled to 8, deck 42" {[bk 7] == 2 && [bk 8] == 8 && [bk 17] == 42} }
snap [expr {$b + 8.1}] s04_discarded
# play five cards
key [expr {$b + 8.4}] space
key [expr {$b + 8.8}] right
key [expr {$b + 9.2}] space
key [expr {$b + 9.6}] right
key [expr {$b + 10.0}] space
key [expr {$b + 10.4}] right
key [expr {$b + 10.8}] space
key [expr {$b + 11.2}] right
key [expr {$b + 11.6}] space
at [expr {$b + 12.0}] { check "five cards selected" {[bk 16] != 0} }
key [expr {$b + 12.2}] p
at [expr {$b + 21.0}] { check "play: 3 hands left, score > 0, hand refilled" {[bk 6] == 3 && [bk32 11] > 0 && [bk 8] == 8 && [bk 1] == 0} }
snap [expr {$b + 21.1}] s05_played
# cheat-win the round
key [expr {$b + 21.5}] 1
at [expr {$b + 24.0}] { check "round won -> cash out screen" {[bk 0] == $::SC(cashout)} }
snap [expr {$b + 24.1}] s06_cashout
key [expr {$b + 25.0}] space
key [expr {$b + 26.0}] space
at [expr {$b + 27.5}] { check "cash out -> shop" {[bk 0] == $::SC(shop)} }
snap [expr {$b + 27.6}] s07_shop
key [expr {$b + 28.0}] n
at [expr {$b + 29.5}] { check "next round -> big blind select (blind=1, ante=1)" {[bk 0] == $::SC(blind) && [bk 3] == 1 && [bk 2] == 1} }
key [expr {$b + 30.0}] space
at [expr {$b + 32.0}] { check "big blind round started" {[bk 0] == $::SC(round) && [bk 3] == 1 && [bk 6] == 4} }
key [expr {$b + 32.5}] 5
at [expr {$b + 34.0}] { check "lose -> game over screen" {[bk 0] == $::SC(over)} }
snap [expr {$b + 34.1}] s08_over
key [expr {$b + 36.0}] space
at [expr {$b + 37.5}] { check "game over -> title" {[bk 0] == $::SC(title)} }
finish [expr {$b + 38}]
