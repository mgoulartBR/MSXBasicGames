# Audio smoke test: samples PSG registers while the morning music loop plays and on SFX
source [file join [file dirname [info script]] lib.tcl]
proc psg {t} {
    set f [open /tmp/claude-0/psg.txt a]
    set s "$t:"; for {set i 0} {$i < 14} {incr i} { append s " [format %02X [debug read {PSG regs} $i]]" }
    puts $f $s; close $f
}
file delete /tmp/claude-0/psg.txt
at 4.5 { space }
for {set t 6} {$t < 14} {set t [expr {$t + 0.4}]} { at $t "psg $t" }
at 14 { exit }
