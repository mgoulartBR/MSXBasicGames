source kingdom_helpers.tcl
proc flash_case {} {
 delay 45
 debug write memory $::S(unit_hp) 20 ;# unit 0
 debug write memory [expr {$::S(unit_hp)+2}] 24
 debug write memory [expr {$::S(hurt_flash)+2}] 200
 debug write memory [expr {$::S(hurt_flash)+0}] 200
 debug write memory $::S(sprites_dirty) 3
 delay 1
 shot flash
 puts stderr "RESULT failures=$::failures";exit
}
coroutine flash_run guard flash_case
after realtime 200 {puts stderr "FAIL flash watchdog";exit}
