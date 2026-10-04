# Boot, start, and capture the first briefing pages. Name from env RT_SHOT.
source [file join [file dirname [info script]] lib.tcl]
set n $::env(RT_SHOT)
at 4.5 { space }
at 6.5 "shot ${n}_p1"
at 7 { space }
at 9 "shot ${n}_p2"
at 9.5 { space }
at 11.5 "shot ${n}_p3"
at 12 { exit }
