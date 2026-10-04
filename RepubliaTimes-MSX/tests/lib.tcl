# Helpers for scripted openMSX runs (all times are emulated seconds from machine start)
set ::shotdir [file normalize [file join [file dirname [info script]] .. screenshots]]
set throttle on
set frameskip 0
proc at {t script} { after time $t $script }
proc key {row mask {hold 0.12}} {
    keymatrixdown $row $mask
    after time $hold "keymatrixup $row $mask"
}
proc space {} { key 8 1 }
proc esc {} { key 7 4 }
proc left {} { key 8 16 }
proc up {} { key 8 32 }
proc down {} { key 8 64 }
proc right {} { key 8 128 }
proc shot {name} { screenshot $::shotdir/$name.png }
proc finish {} { exit }
