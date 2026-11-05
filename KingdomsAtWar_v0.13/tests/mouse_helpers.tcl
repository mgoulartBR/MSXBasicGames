source kingdom_helpers.tcl
set osd_menu_button off
set grabinput on
# Fail immediately if any keyboard fallback sneaks into a mouse-only scenario.
rename tap keyboard_tap_unused
proc tap {args} {error "keyboard forbidden in mouse-only test"}
rename point keyboard_point_unused
proc point {args} {error "keyboard cursor forbidden in mouse-only test"}
proc mouse_open {} {
 set ::throttle on;set ::speed 100
 delay 0.5
 set ::mf [open $::env(KAW_MOUSE_FIFO) w];fconfigure $::mf -buffering line
}
proc host {command a b} {puts $::mf "$command $a $b";flush $::mf;after realtime 0.04 [info coroutine];yield}
proc mp {tx ty} {
 for {set i 0} {$i<80} {incr i} {
  set dx [expr {$tx-[b mouse_x]}];set dy [expr {$ty-[b mouse_y]}]
  if {abs($dx)<=1 && abs($dy)<=1} {return}
  set dx [expr {$dx*2}];set dy [expr {$dy*2}]
  if {$dx>64} {set dx 64};if {$dx< -64} {set dx -64}
  if {$dy>64} {set dy 64};if {$dy< -64} {set dy -64}
  set loop_before [w loops]
  host M $dx $dy
  await {(([w loops]-$loop_before)&65535)>=2} 3
 }
 error "mouse position timeout $tx,$ty -> [b mouse_x],[b mouse_y]"
}
proc mc {button} {
 set start [w loops];host B $button 1;delay 0.12
 await {(([w loops]-$start)&65535)>=2} 8
 host B $button 0;set start [w loops];delay 0.12
 await {(([w loops]-$start)&65535)>=2} 8
}
proc mu {id} {mp [expr {[unit $id px]+8-[w cam_x]}] [expr {[unit $id py]+8-[w cam_y]}];mc 1}

proc bgerror {message} {puts stderr "FAIL Tcl: $message";exit}
# Screenshots temporarily fast-forward keyboard tests; mouse tests retain real time.
rename shot keyboard_shot_unused
proc shot {name} {keyboard_shot_unused $name;set ::throttle on;set ::speed 100}
