# Navigation load: moves the feed selection up/down every 250 ms and samples the PC every ~14 ms.
# Samples inside the frame-wait loop (Ui_Frame) = idle; everything else = busy. Output: RT_OUT.
source [file join [file dirname [info script]] lib.tcl]
set ::pf [open $::env(RT_OUT) w]
proc sample {} { puts $::pf [format %04X [reg pc]]; after time 0.0137 sample }
at 4.5 { space }
at 7 { space }
at 30 { sample }
for {set i 0} {$i < [expr {[info exists ::env(RT_NONAV)] ? 0 : 36}]} {incr i} { at [expr {30.2 + 0.25*$i}] { if {$::dir} { up } else { down } ; set ::dir [expr {!$::dir}] } }
set ::dir 0
at 40 { close $::pf; exit }
