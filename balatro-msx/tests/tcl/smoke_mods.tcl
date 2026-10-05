# Card modifiers (debug ROM): enhancements / editions / seals drawn, described and scored
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set b 7.0
key [expr {$b + 1.5}] space
key [expr {$b + 2.5}] space       ;# deck screen: Start
key [expr {$b + 3.5}] space
key [expr {$b + 5.0}] 9
key [expr {$b + 5.2}] 0
key [expr {$b + 5.5}] 0
snap [expr {$b + 5.9}] 70_mods_hand
key [expr {$b + 6.0}] right
key [expr {$b + 6.4}] right
snap [expr {$b + 6.9}] 71_mods_info
key [expr {$b + 7.2}] space
key [expr {$b + 7.6}] right
key [expr {$b + 8.0}] space
key [expr {$b + 8.4}] p
snap [expr {$b + 10.0}] 72_mods_scoring
at [expr {$b + 16.0}] { check "modified cards play: score counted, hand refilled" {[bk32 11] > 0 && [bk 8] == 8} }
snap [expr {$b + 16.1}] 73_mods_after
finish [expr {$b + 17}]
