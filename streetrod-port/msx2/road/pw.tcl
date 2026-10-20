set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 70 { set f [open $::env(OUTF) w]; puts $f "pw=[format %04X [expr {[debug read memory 0xF038]|([debug read memory 0xF039]<<8)}]] t1=[debug read memory 0xF03A]"; close $f; exit }
