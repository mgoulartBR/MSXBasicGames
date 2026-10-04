# Day length test: measures emulated time from "Start Work" to the day-over popup (expects 60 s)
source [file join [file dirname [info script]] lib.tcl]
set ::t0 0
set ::done 0
proc poll {} {
    if {$::done} return
    if {[vpeek [expr {0x2000 + 0x930}]] == 0xF1} {
        set f [open $::env(RT_OUT) w]; puts $f "popup at [format %.1f [expr {[machine_info time] - $::t0}]] s after start work"; close $f
        set ::done 1; exit
    }
    after time 0.25 poll
}
at 4.5 { space }
at 6.0 { space }
at 7.0 { space }
at 8.0 { set ::t0 [machine_info time]; poll }
at 120 { exit }
