# A real (emulated) MSX mouse on port 1 at rest: it must be detected on the title screen without
# phantom clicks/pointer, and the keyboard must keep working. g_MouseOn must stay 0.
source [file join [file dirname [info script]] lib.tcl]
proc sym {name} { set f [open $::env(RT_MAP)]; set d [read $f]; close $f; regexp "\\n\\s+(\[0-9A-F\]{8})\\s+_$name\\s" $d -> a; return [expr {"0x$a" & 0xFFFF}] }
plug joyporta mouse
proc rep {t} { set f [open $::env(RT_OUT) a]; puts $f "$t on=[peek [sym g_MouseOn]] held=[peek [sym g_Held]]"; close $f }
file delete $::env(RT_OUT)
at 3 { rep title }
at 4.5 { space }
at 8 { rep morning }
at 8.5 { space }
at 12 { rep play }
at 12.5 { shot 25_mouse_plugged_play }
at 13 { exit }
