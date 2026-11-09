source kingdom_helpers.tcl
proc dump_overlay011 {name} {
 set prefix overlay011-$::env(KAW_VARIANT)-$name
 set device [expr {$::env(KAW_VARIANT) eq "V9990"?"Sunrise GFX9000 VRAM":"VRAM"}]
 set size [expr {$::env(KAW_VARIANT) eq "V9990"?524288:131072}]
 foreach {suffix dev start length} [list map memory $::S(art_map) 1024 vram $device 0 $size] {
  set f [open $prefix-$suffix.bin wb];fconfigure $f -translation binary;puts -nonewline $f [debug read_block $dev $start $length];close $f
 }
 foreach {suffix name len} {memory fog_memory 1024 seen fog_seen 128 visible fog_visible 128} {
  set f [open $prefix-$suffix.bin wb];fconfigure $f -translation binary;puts -nonewline $f [debug read_block memory $::S($name) $len];close $f
 }
 set f [open $prefix-meta.txt w];puts $f "0 [w cam_x] [w cam_y] [b placement_mode] [b place_valid]";close $f
 shot $prefix
}
proc overlays093 {} {
 delay 45
 choose 0;point 24 204;tap 5 128
 world_point 12 0;delay 1
 ck ghost_first {[b placement_mode]==1 && [b place_x]==12 && [b place_y]==0}
 dump_overlay011 first
 world_point 8 0;delay 1
 ck ghost_moved {[b placement_mode]==1 && [b place_x]==8 && [b place_y]==0}
 dump_overlay011 moved
 tap 7 4;delay 1
 ck ghost_cancelled {[b placement_mode]==0}
 dump_overlay011 cancelled
 ck no_general_hud_erase {[w hud_erase_count]==0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine overlay_case093 guard overlays093
after realtime 160 {puts stderr "FAIL overlay watchdog";exit}
