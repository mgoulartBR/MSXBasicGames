set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
proc w {a} { return [expr {[debug read memory $a] | ([debug read memory [expr {$a+1}]] << 8)}] }
proc pc {} { set f [open $::env(OUTF) a]; set sp [reg SP]; set s ""; for {set i 0} {$i<14} {incr i} { append s [format "%04X " [w [expr {$sp+2*$i}]]] }; puts $f "PC=[format %04X [reg PC]] SP=[format %04X $sp] stack: $s page2=[debug read memory 0x8000] "; close $f; after time 0.37 pc }
after time 45 pc
after time 47 { exit }
