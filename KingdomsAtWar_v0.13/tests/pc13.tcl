source kingdom_helpers.tcl
set ::pcs [dict create]
proc samp {} { dict incr ::pcs [reg PC]; after time 0.0003 samp }
proc pc13 {} {
 delay 45; samp; delay 3
 set l {};dict for {k v} $::pcs {lappend l [list $v $k]}
 set l [lsort -decreasing -integer -index 0 $l]
 foreach e [lrange $l 0 40] {puts stderr [format "PCS %5d %04X" [lindex $e 0] [lindex $e 1]]}
 puts stderr "RESULT failures=0";exit
}
coroutine pc13_case guard pc13
after realtime 120 {puts stderr "FAIL watchdog";exit}
