# With the mouse active every "press SPACE" hint must read "click" (title, briefing prompt, panel, popup).
# ROM: -DDBG_MOUSE_TEST -DDBG_FAST=10 (short day)
source [file join [file dirname [info script]] lib.tcl]
proc sym {name} { set f [open $::env(RT_MAP)]; set d [read $f]; close $f; regexp "\\n\\s+(\[0-9A-F\]{8})\\s+_$name\\s" $d -> a; return [expr {"0x$a" & 0xFFFF}] }
proc ptr {x y} { poke [sym g_MouseOn] 1; poke [sym g_MouseX] $x; poke [sym g_MouseY] $y }
proc click {} { poke [sym g_MouseBtn] 1; after time 0.15 { poke [sym g_MouseBtn] 0 } }
at 3.6 { ptr 100 100 }
at 4.5 { shot 30_title_mouse }
at 5   { click }
at 7   { shot 31_morning_mouse }
at 7.5 { click }
at 9   { click }
at 14  { ptr 70 12 }
at 15  { shot 32_play_mouse_hint }
at 40  { shot 33_popup_mouse }
at 41  { exit }
