# Mouse-mode test (ROM built with -DDBG_MOUSE_TEST: the ISR skips the hardware mouse and the pointer
# state is poked from here). Exercises hover, size select, pick up, move, drop, right-click discard.
source [file join [file dirname [info script]] lib.tcl]
proc sym {name} {
    set f [open $::env(RT_MAP)]; set d [read $f]; close $f
    regexp "\\n\\s+(\[0-9A-F\]{8})\\s+_$name\\s" $d -> a
    return [expr {"0x$a" & 0xFFFF}]
}
proc ptr {x y} { poke [sym g_MouseOn] 1; poke [sym g_MouseX] $x; poke [sym g_MouseY] $y }
proc btn {v} { poke [sym g_MouseBtn] $v }
proc click {{b 1}} { btn $b; after time 0.15 "btn 0" }
at 4.5 { space }
at 7   { space }
at 24  { ptr 70 12 }          ;# hover the 1st feed entry (tile row 1)
at 25  { shot 20_mouse_hover }
at 26  { click }              ;# pick up (BIG): footprint follows the pointer
at 27  { ptr 200 80 }
at 28  { shot 21_mouse_place }
at 29  { click }              ;# drop on the paper
at 30  { ptr 70 60 }
at 31  { shot 22_mouse_placed }
at 32  { ptr 120 180 ; click }  ;# pick MED in the size selector? (row 22 = y 176..183) -> block 2 starts x=80
at 34  { ptr 200 80 }
at 35  { click }              ;# click the placed article: pick it up again
at 36  { ptr 230 150 }
at 37  { shot 23_mouse_repick }
at 38  { click 2 }            ;# right button: discard
at 39  { shot 24_mouse_discard }
at 40  { exit }
