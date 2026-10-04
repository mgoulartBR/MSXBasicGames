# Gameplay test: day 1 - place articles, inspect paper mode, end the day, night + day 2 briefing
source [file join [file dirname [info script]] lib.tcl]
at 5  { space }
at 6.5 { shot 02_morning }
at 7  { space }
at 30 { shot 03_play_feed }
at 31 { space }            ;# pick the selected item (BIG)
at 32 { shot 04_place_big }
at 33 { right }
at 33.5 { space }          ;# drop
at 34.5 { shot 05_placed }
at 35 { down }
at 35.5 { right }
at 36 { space }            ;# pick 2nd item as MED
at 36.5 { down }
at 37 { space }
at 38 { shot 06_two_articles }
at 39 { esc }              ;# paper mode
at 40 { shot 07_paper_mode }
for {set i 0} {$i < 12} {incr i} { at [expr {42 + 0.3*$i}] { down } }
at 47 { shot 08_endday_selected }
at 48 { space }            ;# End Day (10x)
at 78 { shot 09_popup }
at 79 { space }
at 83 { shot 10_night_p1 }
at 85 { space }
at 86 { shot 10_night_p2 }
at 87 { space }
at 90 { shot 11_morning_day2 }
at 91 { finish }
