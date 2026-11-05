# Isolated Z80 unit test, never used for gameplay timing.
source kingdom_helpers.tcl
set kernel_case 0
set seed 12012
proc rnd012 {} {set ::seed [expr {(1664525*$::seed+1013904223)&0xFFFFFFFF}];return [expr {($::seed>>24)&255}]}
proc prepare_kernel {} {
 global S kernel_case expected expected_result
 foreach n {fog_seen fog_visible fog_next fog_trail fog_changed} {
  set a {};set count [expr {$n eq "fog_changed"?256:128}]
  for {set i 0} {$i<$count} {incr i} {lappend a [expr {$kernel_case==0?0:[rnd012]}]}
  set input($n) $a;debug write_block memory $S($n) [binary format c* $a]
 }
 set expected_result 0
 foreach n {fog_seen fog_visible fog_trail fog_need fog_delta fog_changed} {set expected($n) {}}
 set c0 {};set c1 {}
 for {set i 0} {$i<128} {incr i} {
  set seen [lindex $input(fog_seen) $i];set old [lindex $input(fog_visible) $i];set v [lindex $input(fog_next) $i];set trail [lindex $input(fog_trail) $i];set fresh [expr {$trail&(~$seen)&255}];set d [expr {($old^$v)|$fresh}]
  lappend expected(fog_seen) [expr {$seen|$v|$trail}];lappend expected(fog_visible) $v;lappend expected(fog_trail) 0;lappend expected(fog_need) [expr {($v&(~$old))|$fresh}];lappend expected(fog_delta) $d
  lappend c0 [expr {[lindex $input(fog_changed) $i]|$d}];lappend c1 [expr {[lindex $input(fog_changed) [expr {$i+128}]]|$d}];set expected_result [expr {$expected_result|$d}]
 }
 set expected(fog_changed) [concat $c0 $c1]
 reg SP 62192;reg IX 4660;reg IY 22136;reg PC 61440
}
proc check_kernel {} {
 global S expected kernel_case expected_result
 foreach n [array names expected] {if {[debug read_block memory $S($n) [llength $expected($n)]] ne [binary format c* $expected($n)]} {puts stderr "FAIL Z80 case=$kernel_case plane=$n";exit}}
 if {[reg A]!=$expected_result || [reg IX]!=4660 || [reg IY]!=22136} {puts stderr "FAIL Z80 registers case=$kernel_case";exit}
 incr kernel_case
 if {$kernel_case==257} {puts stderr "PASS 257 Z80 cases, 230272 output bytes, IX/IY preserved";puts stderr "RESULT failures=0";exit}
 prepare_kernel
}
proc start_kernel {} {
 delay 45
 set name [expr {$::env(KAW_VARIANT) eq "V9990"?"KAW9990":"KAW68PV"}];set f [open ../build/$::env(KAW_VARIANT)/$name.map];set text [read $f];close $f
 if {![regexp {([0-9A-F]{8})\s+_fog_merge\s} $text whole address]} {error "kernel missing"};scan $address %x address;set address [expr {$address&65535}]
 debug write_block memory 61440 [binary format c* [list 62 12 50 0 120 50 6 192 205 [expr {$address&255}] [expr {$address>>8}] 195 11 240]]
 debug set_bp 61451 {} {check_kernel};prepare_kernel
}
coroutine kernel012_case guard start_kernel
after realtime 180 {puts stderr "FAIL kernel watchdog";exit}
