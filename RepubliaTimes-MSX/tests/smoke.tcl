# Smoke test: boot -> title -> briefing -> play screen (feed fills)
source [file join [file dirname [info script]] lib.tcl]
at 4   { shot 01_title }
at 4.5 { space }
at 6.5 { shot 02_morning }
at 7   { space }
at 20  { shot 03_play_feed }
at 21  { exit }
