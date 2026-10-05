source [file join [file dirname [info script]] lib.tcl]
# C-BIOS logo takes ~6 s of emulated time before the cartridge starts
set b 7.0
snap [expr {$b + 1.0}] 01_title
key [expr {$b + 1.5}] space
key [expr {$b + 2.5}] space       ;# deck screen: Start
snap [expr {$b + 3.0}] 02_blind
key [expr {$b + 3.5}] space
snap [expr {$b + 5.0}] 03_round
key [expr {$b + 5.5}] right
key [expr {$b + 6.0}] space
key [expr {$b + 6.5}] right
key [expr {$b + 7.0}] space
key [expr {$b + 7.5}] right
key [expr {$b + 8.0}] space
snap [expr {$b + 8.6}] 04_selected
key [expr {$b + 9.0}] p
snap [expr {$b + 10.0}] 05_scoring
snap [expr {$b + 12.0}] 06_scoring2
snap [expr {$b + 15.0}] 07_after
finish [expr {$b + 16}]
