# Skip-blind / tag flow (debug ROM)
source [file join [file dirname [info script]] lib.tcl]
source [file join [file dirname [info script]] asserts.tcl]
set b 7.0
key [expr {$b + 1.5}] space
snap [expr {$b + 3.0}] 60_blind_tags
at [expr {$b + 3.1}] { check "tags offered for small and big blind" {[bk 22] != 0 && [bk 23] != 0} }
key [expr {$b + 3.5}] down                  ;# focus Skip Blind
snap [expr {$b + 4.0}] 61_skip_focus
key [expr {$b + 4.4}] space                 ;# skip the small blind
at [expr {$b + 6.0}] { check "small blind skipped: big blind is next, skips=1" {[bk 3] == 1 && [bk 21] == 1} }
snap [expr {$b + 6.1}] 62_after_skip
finish [expr {$b + 7}]
