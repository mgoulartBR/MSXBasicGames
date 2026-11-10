source helpers.tcl
proc boot {} {
 delay 55
 ck enabled {[b video_enabled]==1 && [w loops]>5}
 ck four_units {[b unit_count]==4 && [b unit_kind]==0 && [b unit_kind 2]==1}
 ck keyclick_off {[debug read memory 0xF3DB]==0}
 ck bios_restored {[debug read memory 0]==243}
 ck real_simulation {[w sim_ticks]>30 && [w game_seconds]>0}
 ck castle_footprint {[b terrain [expr {9*32+5}]]==5 && [b terrain [expr {10*32+5}]]==0}
 ck larger_art_ids {[b art_map [expr {6*32+2}]]==64}
 ck no_auto_bridge {[b terrain [expr {6*32+12}]]==2}
 shot boot07
 puts stderr "BOOT loops=[w loops] ticks=[w sim_ticks] sec=[w game_seconds] rendered=[w rendered_frames] upload=[w frame_uploads]"
 puts stderr "RESULT failures=$::failures";exit
}
proc guard {} {if {[catch {boot} msg opts]} {puts stderr "FAIL $msg\n[dict get $opts -errorinfo]";exit}}
coroutine case07 guard
after realtime 90 {puts stderr "FAIL boot watchdog pc=[reg PC]";exit}
