# Compara a pagina exibida apos scroll incremental com a mesma pagina apos repintura total forcada.
source kingdom_helpers.tcl
proc page_bytes {} {
 set v $::env(KAW_VARIANT);set p [b shown];set cy [w cam_y];set out {}
 if {$v eq "V9990"} {
  for {set y 0} {$y<160} {incr y} {append out [debug read_block {Sunrise GFX9000 VRAM} [expr {($p*256+$y)*256}] 128]}
 } else {
  for {set y 0} {$y<160} {incr y} {set r [expr {$p*256+(($cy+$y)&255)}];append out [debug read_block VRAM [expr {$r*128}] 128]}
 }
 return $out
}
proc scroll_for {row mask secs} {keymatrixdown $row $mask;delay $secs;keymatrixup $row $mask;delay 0.8}
proc refresh13 {} {
 delay 45;choose 2
 scroll_for 8 0x80 1.2   ;# direita
 scroll_for 8 0x40 0.9
 scroll_for 8 0x10 0.5   ;# esquerda
 scroll_for 8 0x20 0.4
 delay 1
 shot refA13;set ca "[w cam_x],[w cam_y]"
 debug write memory $::S(field_valid) 0;debug write memory [expr {$::S(field_valid)+1}] 0
 for {set i 0} {$i<6} {incr i} {debug write memory $::S(sprites_dirty) 3;delay 0.7}
 shot refB13;set cb "[w cam_x],[w cam_y]"
 puts stderr "REFRESH cams $ca $cb"
 ck same_cam {$ca eq $cb}
 puts stderr "RESULT failures=$::failures";exit
}
proc guard13 {} {if {[catch {refresh13} m opts]} {puts stderr "FAIL $m\n[dict get $opts -errorinfo]";exit}}
coroutine refresh13_case guard13
after realtime 200 {puts stderr "FAIL watchdog";exit}
