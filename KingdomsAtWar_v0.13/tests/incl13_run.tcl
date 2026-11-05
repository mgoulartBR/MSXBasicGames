
array set ::extra {__str_0 35468 __str_1 35484 __str_10 36635 __str_11 36641 __str_12 36648 __str_13 36654 __str_14 36660 __str_15 36668 __str_16 36674 __str_17 36680 __str_18 36687 __str_19 36694 __str_2 35495 __str_3 35501 __str_4 35507 __str_5 35513 __str_6 36605 __str_7 36612 __str_8 36618 __str_9 36627 address 19847 art_tile 19301 asset_begin 16494 asset_read 18257 bank_support 19274 box_draw 24391 build_base 17098 build_gold 16530 build_sizes 17106 build_wood 16522 building_size 19285 cache_frame 24788 cache_xy 20567 clip_copy 22003 combat_alive 35604 command_busy 20071 command_submit 20099 cursor 16550 draw_actor 25783 erase_border 24139 field_registers 20665 fog_art 19739 fog_at 19682 fog_bit_table 19839 font_asset 16591 free_at 35636 gpu_copy 20152 gpu_fill 20294 gpu_wait 20123 gpu_write_tile 20419 hud_y 20983 intersect 21741 irq_video 20759 kind_sizes 17114 large_tx 21985 large_ty 21994 loading_text 33911 main 36003 order_one 35842 paint_world 22661 palette68 18049 palette9 18081 passable 35576 poll_input 18618 pose_bounds 17377 poses 16594 raw_read 18543 raw_reg 19799 recruit_gold 16544 recruit_wood 16538 reg 19812 render_bit 21957 restore_field 24083 ring_pixels 18129 site_tiles 16518 small_tx 21977 small_ty 21981 static_assets 16582 tile 35559 tile_source 17121 unit_cooldown 19832 unit_damage 19825 unit_max_hp 19294 unit_position 19318 unit_range 19818 upload_stream 19915 video_camera 33466 video_camera_ready 33463 video_char 21004 video_clock 20952 video_cursor 21465 video_enable 35519 video_fill 21156 video_hud_begin 21217 video_hud_end 21325 video_hud_flush 21462 video_init 34045 video_known_tile 33528 video_present 21568 video_sprites 28368 video_stream 33474 video_text 21097 video_tile 33475 video_wait 20959 vp_in 19795 vp_out 19791 vwrite 19933 vwrite_row 19984 world_fill 23670}

set ::incl [dict create];set ::calls [dict create];set ::pend [dict create]
proc trk_enter {name} {
 if {!$::perf_running} return
 set sp [reg SP];set ra [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}]
 dict set ::pend $sp [list $name [machine_info time]]
 set id [debug set_bp $ra "\$::perf_running && \[reg SP\]==[expr {$sp+2}]" "trk_leave $sp"]
 dict set ::pendbp $sp $id
}
proc trk_leave {sp} {
 if {![dict exists $::pend $sp]} return
 lassign [dict get $::pend $sp] name t0
 dict incr ::calls $name
 if {[dict exists $::incl $name]} {dict set ::incl $name [expr {[dict get $::incl $name]+[machine_info time]-$t0}]} else {dict set ::incl $name [expr {[machine_info time]-$t0}]}
 catch {debug remove_bp [dict get $::pendbp $sp]}
 dict unset ::pend $sp
}
set ::pendbp [dict create]
proc dump_incl {phase} {
 set l {};dict for {k v} $::incl {lappend l [list $k $v [dict get $::calls $k]]}
 foreach e [lsort -decreasing -real -index 1 $l] {puts stderr [format "INCL %s %-16s %8.1f ms  calls=%d  avg=%.2f ms" $phase [lindex $e 0] [expr {[lindex $e 1]*1000}] [lindex $e 2] [expr {[lindex $e 1]*1000/[lindex $e 2]}]]}
 set ::incl [dict create];set ::calls [dict create]
}
source kingdom_helpers.tcl
set perf_running 0
set counts [dict create]
proc perf_start {} {
 set ::perf_times {};set ::counts [dict create];set ::perf_running 1
 set ::start [machine_info time];set ::start_frames [w rendered_frames];set ::start_ticks [w sim_ticks]
}
proc perf_end {phase} {
 dump_incl $phase
 set ::perf_running 0
 puts stderr "METRIC $phase seconds=[expr {[machine_info time]-$::start}] frames=[expr {([w rendered_frames]-$::start_frames)&65535}] ticks=[expr {([w sim_ticks]-$::start_ticks)&65535}]"
 puts stderr "CADENCE $phase $::perf_times"
 puts stderr "CALLS $phase $::counts"
}
proc perf012 {} {
 delay 45;choose 2;world_point 0 0
 debug set_watchpoint write_mem $::S(rendered_frames) {$::perf_running} {lappend ::perf_times [machine_info time]}
 set variant $::env(KAW_VARIANT);set name [dict get {V9990 KAW9990 PREVIEW KAW68PV V9968 KAW9968} $variant]
 set f [open ../build/$variant/$name.map];set text [read $f];close $f
 foreach name {fog_update fog_stamp fog_merge projectile_tick repair_tick video_sprites} {
  if {[regexp "(\[0-9A-F\]{8})\\s+_$name\\s" $text whole address]} {
   scan $address %x address;set bank [expr {$address>>16}];set pc [expr {$address&65535}]
   set condition [expr {$bank?"\$::perf_running && \[debug read memory 0xC006\]==$bank":"\$::perf_running"}]
   debug set_bp $pc $condition "dict incr ::counts $name"
  }
 }

 set variant $::env(KAW_VARIANT);set nm [expr {$variant eq "V9990"?"KAW9990":"KAW68PV"}]
 set f [open ../build/$variant/$nm.map];set mtext [read $f];close $f
 foreach fn {video_sprites paint_world clip_copy fog_apply fog_cover damage_mask draw_actor restore_field video_present gpu_copy structure_bars world_fill} {
  set address -1
  if {[info exists ::extra($fn)]} {set address $::extra($fn);set bank 0;set pc $address;set condition "\$::perf_running";debug set_bp $pc $condition "trk_enter $fn";continue}
  if {[regexp "(\[0-9A-F\]{8})\\s+_$fn\\s" $mtext whole address]} {
   scan $address %x address;set bank [expr {$address>>16}];set pc [expr {$address&65535}]
   set condition [expr {$bank?"\$::perf_running && \[debug read memory 0xC006\]==$bank":"\$::perf_running"}]
   debug set_bp $pc $condition "trk_enter $fn"
  }
 }
 perf_start;delay 2;perf_end idle
 perf_start;tap 5 32;await {[unit 2 state]==3 && [unit 2 x]==0 && [unit 2 y]==0} 120;perf_end one
 ck one_unit_arrives {[unit 2 x]==0 && [unit 2 y]==0}
 tap 2 64;world_point 0 0;ck four_selected {[b selection_mask]==15}
 perf_start;tap 5 32
 await {[unit 0 state]==3 && [unit 1 state]==3 && [unit 2 state]==3 && [unit 3 state]==3 && [unit 0 progress]==0 && [unit 1 progress]==0 && [unit 2 progress]==0 && [unit 3 progress]==0} 120
 perf_end four
 set goals {};for {set i 0} {$i<4} {incr i} {lappend goals [list [unit $i x] [unit $i y]]}
 ck four_distinct_goals {[llength [lsort -unique $goals]]==4}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine perf012_case guard perf012
after realtime 300 {puts stderr "FAIL perf watchdog";exit}
