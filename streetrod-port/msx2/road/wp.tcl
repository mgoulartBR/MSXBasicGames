set throttle off
set ::f [open $::env(OUT) w]
set ::n 0
debug set_watchpoint write_mem 0xF37D {} { if {$::n < 12} { puts $::f "t=[machine_info time] pc=[format %04X [reg PC]] val=[format %02X $::wp_last_value] sp=[format %04X [reg SP]]"; flush $::f; incr ::n } }
after time 8 { close $::f; exit }
