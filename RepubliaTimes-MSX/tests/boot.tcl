# Boot-only check: screenshot of the first screen. Name from env RT_SHOT.
source [file join [file dirname [info script]] lib.tcl]
at 5 "shot $::env(RT_SHOT)"
at 5.5 { exit }
