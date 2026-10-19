set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
proc poll {} { set f [open $::env(OUTF) a]; puts $f "t1=[expr {[debug read memory 0xF03A]|([debug read memory 0xF03B]<<8)}] pr=[format %04x [expr {[debug read memory 0xF03C]|([debug read memory 0xF03D]<<8)}]] s=[format %04x [expr {[debug read memory 0xF03E]|([debug read memory 0xF03F]<<8)}]]"; close $f; after time 2 poll }
after time 21 poll
after time 80 { exit }
