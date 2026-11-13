source kingdom_helpers.tcl
proc place_unit {id x y} {
 set a [expr {$::S(units)+16*$id}]
 set px [expr {$x*16}];set py [expr {$y*16}]
 debug write memory $a [expr {$px&255}];debug write memory [expr {$a+1}] [expr {$px>>8}]
 debug write memory [expr {$a+2}] [expr {$py&255}];debug write memory [expr {$a+3}] [expr {$py>>8}]
 foreach off {4 6 8} {debug write memory [expr {$a+$off}] $x}
 foreach off {5 7 9} {debug write memory [expr {$a+$off}] $y}
}
proc occl_case {} {
 delay 65
 place_unit 0 3 7
 place_unit 1 5 8
 place_unit 2 2 6
 debug write memory $::S(sprites_dirty) 3
 delay 2
 shot occl-right
 puts stderr "RESULT failures=$::failures";exit
}
coroutine occl_run guard occl_case
after realtime 200 {puts stderr "FAIL occl watchdog";exit}
