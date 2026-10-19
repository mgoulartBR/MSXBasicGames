set throttle off
set ::f [open $::env(OUT) w]
proc lg {t} { puts $::f "t=$t pc=[format %04X [reg PC]] sp=[format %04X [reg SP]] f37d=[format %02X [peek 0xF37D]]"; flush $::f }
foreach t {1 2 3 4 5 8 12} { after time $t "lg $t" }
after time 13 { close $::f; exit }
