# PC sampling profiler: samples the Z80 PC every 20 ms (emulated) into /tmp/rt_prof.txt
source [file join [file dirname [info script]] lib.tcl]
file delete /tmp/rt_prof.txt
set ::pf [open /tmp/rt_prof.txt w]
proc sample {} { puts $::pf [format %04X [reg pc]]; flush $::pf; after time 0.0137 sample }
at 5 { space }
at 5.05 { sample }
at 9 { close $::pf; exit }
