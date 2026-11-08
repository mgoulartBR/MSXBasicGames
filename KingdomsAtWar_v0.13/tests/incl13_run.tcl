
array set ::extra {__str_0 24575 __str_1 24581 __str_10 25751 __str_11 25757 __str_12 25763 __str_13 25771 __str_14 25777 __str_15 25783 __str_16 25790 __str_17 25797 __str_2 24587 __str_3 24593 __str_4 25708 __str_5 25715 __str_6 25721 __str_7 25730 __str_8 25738 __str_9 25744 art_tile 19202 asset_begin 16494 asset_read 18257 bank_support 19175 build_base 17098 build_gold 16530 build_sizes 17106 build_wood 16522 building_size 19186 cell_write 21138 clip_copy 19831 combat_alive 24686 cursor 16550 fog_art 19276 fog_at 19219 fog_bit_table 19565 font_asset 16591 free_at 24718 gpu_wait 19621 hud_name 20746 intersect 19328 kind_sizes 17114 load_progress 22851 main 25085 name_set2 20347 order_one 24924 p1_cell 20382 p1_copy 19659 palette68 18049 palette9 18081 passable 24658 pat_write 19880 poll_input 18543 pose_bounds 17377 poses 16594 recruit_gold 16544 recruit_wood 16538 render_bit 19573 restore_field 19863 ring_pixels 18129 site_tiles 16518 spr_attr 21383 static_assets 16582 tile 24641 tile_patterns 20110 tile_source 17121 unit_cooldown 19558 unit_damage 19551 unit_max_hp 19195 unit_range 19544 v9_busy 19593 v9_lmmm 19598 video_camera 19819 video_camera_ready 19827 video_char 20826 video_clock 19744 video_cursor 21425 video_enable 24599 video_fill 20935 video_hud_begin 19813 video_hud_end 19814 video_hud_flush 19815 video_hud_swap 19751 video_init 22882 video_known_tile 20682 video_present 19818 video_sprites 21485 video_stream 19830 video_text 20876 video_tile 20693 video_wait 19781 video_world_cell 20641 video_world_dither 20647 video_world_tile 20644 world_fill 19849}

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
 set variant $::env(KAW_VARIANT);set name [dict get {V9990 KAW9990 V9990P1 KAW9990P1 PREVIEW KAW68PV V9968 KAW9968} $variant]
 set f [open ../build/$variant/$name.map];set text [read $f];close $f
 foreach name {fog_update fog_stamp fog_merge projectile_tick repair_tick video_sprites} {
  if {[regexp "(\[0-9A-F\]{8})\\s+_$name\\s" $text whole address]} {
   scan $address %x address;set bank [expr {$address>>16}];set pc [expr {$address&65535}]
   set condition [expr {$bank?"\$::perf_running && \[debug read memory 0xC006\]==$bank":"\$::perf_running"}]
   debug set_bp $pc $condition "dict incr ::counts $name"
  }
 }

 set variant $::env(KAW_VARIANT);set nm [dict get {V9990 KAW9990 V9990P1 KAW9990P1 PREVIEW KAW68PV V9968 KAW9968} $variant]
 set f [open ../build/$variant/$nm.map];set mtext [read $f];close $f
 foreach fn {simulate input poll_input fog_update hud video_sprites navigation combat_tick economy production tower_tick camera_update} {
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
