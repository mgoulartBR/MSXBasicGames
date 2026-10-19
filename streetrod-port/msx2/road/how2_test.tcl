set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
after time 20 { set ::a [debug read memory 0xF03A]; tap 7 0x04 }
after time 24 { set f [open $::env(OUTF) w]; puts $f "before=$::a after=[debug read memory 0xF03A]"; close $f; exit }
