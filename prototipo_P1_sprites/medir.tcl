set z80_freq_locked false;set z80_freq 3579545;set z80_freq_locked true
set videosource GFX9000;set maxframeskip 0
set FR 0xC003
set PM 0xC005
proc rd16 {a} {expr {[peek $a]+256*[peek [expr {$a+1}]]}}
after time 7 {
 poke $::PM $::env(MODE)
 after time 1 {
  set ::t0 [machine_info time];set ::f0 [rd16 $::FR]
  after time 4 {
   set t1 [machine_info time];set f1 [rd16 $::FR]
   set n [expr {($f1-$::f0)&65535}];set s [expr {$t1-$::t0}]
   if {$n==0} {puts stderr "RESULT mode=$::env(MODE) frames=0"} else {puts stderr "RESULT mode=$::env(MODE) frames=$n ms_per_frame=[expr {1000.0*$s/$n}] cmd_res=[peek 0xC006]"}
   set ::throttle on;set ::speed 100
   after realtime 1.0 {after frame {screenshot -raw -doublesize /tmp/proto_$::env(MODE).png;exit}}
  }
 }
}
