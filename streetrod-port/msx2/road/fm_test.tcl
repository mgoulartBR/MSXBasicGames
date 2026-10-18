set throttle off
set ::f [open $::env(OUT) w]
set ::ra 0
debug set_watchpoint write_io 0x7C {} { set ::ra $::wp_last_value }
debug set_watchpoint write_io 0x7D {} { puts $::f "t=[format %.2f [machine_info time]] r=[format %02X $::ra] v=[format %02X $::wp_last_value]" }
after time 25 { close $::f; exit }
