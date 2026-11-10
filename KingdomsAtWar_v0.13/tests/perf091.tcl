source kingdom_helpers.tcl
proc perf091 {} {
 delay 55
 choose 2;world_point 0 0
 set t [machine_info time];set f [w rendered_frames];set l [w loops];set ticks [w sim_ticks]
 tap 5 32
 set samples [dict create]
 while {[unit 2 state]!=3 || [unit 2 x]!=0 || [unit 2 y]!=0} {
  set pc [reg PC];set bank [debug read memory 0xC006]
  if {$pc<40960} {set key fixed} else {set key bank$bank}
  dict incr samples $key
  delay 0.01
  if {[machine_info time]-$t>120} {error "movement timeout"}
 }
 set elapsed [expr {[machine_info time]-$t}]
 puts stderr "PERF seconds=$elapsed frames=[expr {([w rendered_frames]-$f)&65535}] loops=[expr {([w loops]-$l)&65535}] ticks=[expr {([w sim_ticks]-$ticks)&65535}] samples=$samples"
 ck one_unit_arrives {[unit 2 x]==0 && [unit 2 y]==0}
 # deixa o desenho assentar: com o laco livre o instante da arrival pode cair no meio de um upload de pose.
 await {[b sprites_dirty]==0} 10;delay 1;await {[b sprites_dirty]==0} 10
 shot perf091
 set variant $::env(KAW_VARIANT)
 set device [expr {$variant eq "V9990"?"Sunrise GFX9000 VRAM":"VRAM"}]
 set size [expr {$variant eq "V9990"?524288:131072}]
 set f [open perf091-$variant-vram.bin wb];fconfigure $f -translation binary;puts -nonewline $f [debug read_block $device 0 $size];close $f
 set f [open perf091-$variant-meta.txt w];puts $f "0 [w cam_x] [w cam_y]";close $f
 puts stderr "RESULT failures=$::failures";exit
}
coroutine profile_case091 guard perf091
after realtime 180 {puts stderr "FAIL profile watchdog";exit}
