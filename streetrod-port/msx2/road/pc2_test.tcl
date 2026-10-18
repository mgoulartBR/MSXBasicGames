set throttle off
set ::f [open $::env(OUT) w]
proc lg {} { set pc [reg PC]; if {$pc >= 0xC000 && $pc < 0xF000} { puts $::f "t=[machine_info time] RAMPC=[format %04X $pc] sp=[format %04X [reg SP]]"; set sp [reg SP]; for {set i 0} {$i<12} {incr i} { append l " [format %04X [expr {[peek [expr {$sp+2*$i}]] + 256*[peek [expr {$sp+2*$i+1}]]}]]" }; puts $::f "stack:$l"; puts $::f "AF=[format %04X [reg AF]] BC=[format %04X [reg BC]] DE=[format %04X [reg DE]] HL=[format %04X [reg HL]]"; close $::f; exit }; after time 0.02 lg }
after time 0.3 lg
after time 12 { close $::f; exit }
