set throttle off
set ::f [open $::env(OUT) w]
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
proc lg {t} { puts $::f "t=$t pc=[format %04X [reg PC]] sp=[format %04X [reg SP]] state=[w 0xF014] f341=[format %02X [peek 0xF341]]"; flush $::f }
foreach t {0.5 1 1.5 2 2.5 3 4 6 8 12 16} { after time $t "lg $t" }
after time 17 { close $::f; exit }
