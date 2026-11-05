source kingdom_helpers.tcl
proc idle13 {} {
 delay 45
 set s0 [list [w loops] [w hud_char_writes] [w hud_swaps] [w hud_erase_count] [w vframes] [w rendered_frames] [w sim_ticks]]
 delay 5
 set s1 [list [w loops] [w hud_char_writes] [w hud_swaps] [w hud_erase_count] [w vframes] [w rendered_frames] [w sim_ticks]]
 puts stderr "IDLE loops,charw,swaps,erase,vframes,rendered,ticks start=$s0 end=$s1 mouse=[b mouse_x],[b mouse_y]"
 puts stderr "RESULT failures=0";exit
}
coroutine idle13_case guard idle13
after realtime 120 {puts stderr "FAIL watchdog";exit}
