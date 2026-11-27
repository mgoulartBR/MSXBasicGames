source helpers.tcl
proc dump07 {name} {
 set prefix camera07-$::env(KAW_VARIANT)-$name
 set f [open $prefix-seen.bin wb];fconfigure $f -translation binary;puts -nonewline $f [layer1k 0x72000];close $f;dump_sources $prefix-sources.txt
 set f [open $prefix-map.bin wb];fconfigure $f -translation binary;puts -nonewline $f [layer1k 0x6A000];close $f
 if {$::env(KAW_VARIANT) eq "V9990"} {set device {Sunrise GFX9000 VRAM};set size 524288} else {set device VRAM;set size 131072}
 set f [open $prefix-vram.bin wb];fconfigure $f -translation binary;puts -nonewline $f [debug read_block $device 0 $size];close $f
 set f [open $prefix-meta.txt w];puts $f "[w cam_x] [w cam_y] 0";close $f
 shot $prefix
}
proc camera07 {} {
 delay 45
 camera_until cam_x 256 128;camera_until cam_y 352 64;delay 1;dump07 corner
 ck camera_corner {[w cam_x]==256 && [w cam_y]==352}
 camera_until cam_x 144 16;camera_until cam_y 176 32;delay 1;dump07 middle
 ck camera_middle {[w cam_x]>=136 && [w cam_x]<=144 && [w cam_y]>=168 && [w cam_y]<=176}
 camera_until cam_x 0 16;camera_until cam_y 0 32;delay 1
 ck camera_return {[w cam_x]==0 && [w cam_y]==0}
 ck fine_scroll_step {[b max_camera_step]<=8}
 ck zero_hud_clears {[w hud_erase_count]==0}
 shot camera07-return
 puts stderr "RESULT failures=$::failures";exit
}
proc guard07 {} {if {[catch {camera07} m opts]} {puts stderr "FAIL $m\n[dict get $opts -errorinfo]";exit}}
coroutine camera_case07 guard07
after realtime 130 {puts stderr "FAIL camera07 watchdog";exit}
