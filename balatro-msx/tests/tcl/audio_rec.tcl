source [file join [file dirname [info script]] lib.tcl]
# record title music, then select-card blips
at 9.0 "record start $::env(SHOT_OUT)/title_music.wav -audioonly"
at 13.0 "record stop"
finish 13.5
