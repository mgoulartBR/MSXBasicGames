# Pointer-logic test: debug key 6 teleports the pointer over the next widget, 7 clicks.
source [file join [file dirname [info script]] lib.tcl]
set b 7.0
key [expr {$b + 1.5}] space
key [expr {$b + 3.5}] space
snap [expr {$b + 5.0}] 30_ptr_round
# walk the pointer over the widgets: first those after W_INFO (hand cards follow)
for {set i 0} {$i < 4} {incr i} { key [expr {$b + 5.5 + $i*0.4}] 6 }
snap [expr {$b + 7.4}] 31_ptr_hover
key [expr {$b + 7.6}] 7
snap [expr {$b + 8.2}] 32_ptr_click
key [expr {$b + 8.4}] 6
key [expr {$b + 8.8}] 7
snap [expr {$b + 9.4}] 33_ptr_click2
finish [expr {$b + 10}]
