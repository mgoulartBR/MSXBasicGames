
array set ::extra {__str_0 32808 __str_1 32814 __str_10 33978 __str_11 33984 __str_12 33990 __str_13 33998 __str_14 34004 __str_15 34010 __str_16 34017 __str_17 34024 __str_2 32820 __str_3 32826 __str_4 33935 __str_5 33942 __str_6 33948 __str_7 33957 __str_8 33965 __str_9 33971 art_tile 19173 asset_begin 16494 asset_read 18452 bank_support 19146 bld_visible 23528 build_base 17098 build_gold 16530 build_sizes 17106 build_wood 16522 building_size 19157 cell_tab 21711 cell_write 21793 clip_copy 19802 combat_alive 32920 cursor 16550 emit_block 22810 fog_art 19247 fog_at 19190 fog_bit_table 19536 font_asset 16591 free_at 33048 gpu_wait 19592 hud_name 20730 intersect 19299 io_lmmv 21128 io_rw 20332 io_vset 20318 irq_p1 22092 kind_sizes 17114 load_progress 30042 lz_decode 18257 main 33260 names_put 20338 occ_get 23057 occ_scan 32952 order_one 33099 otir_vram 22080 p1_cell 20417 p1_copy 19630 palette68 18049 palette9 18081 passable 32892 pat_write 19851 poll_input 18514 pose_bounds 17377 pose_pos 21491 poses 16594 px4 21731 recruit_gold 16544 recruit_wood 16538 render_bit 19544 restore_field 19834 ring_pixels 18129 rl_add 21216 site_tiles 16518 sp_cull 22894 spr_attr 22038 static_assets 16582 tile 32875 tile_patterns 20081 tile_source 17121 unit_cooldown 19529 unit_damage 19522 unit_max_hp 19166 unit_range 19515 v9_busy 19564 v9_lmmm 19569 video_camera 19790 video_camera_ready 19798 video_char 20813 video_clock 19715 video_cursor 22809 video_enable 32832 video_fill 20923 video_hud_begin 19784 video_hud_end 19785 video_hud_flush 19786 video_hud_swap 19722 video_init 30073 video_irq_install 22777 video_known_tile 20666 video_present 19789 video_sprites 23821 video_stream 19801 video_text 20864 video_tile 20677 video_wait 19752 video_world_cell 20625 video_world_dither 20631 video_world_tile 20628 world_fill 19820}

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
 delay 65;choose 2;world_point 0 0
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
 foreach fn {simulate input camera_update fog_update hud video_sprites irq_p1 video_wait unit_prep emit_block sp_cull p1_copy} {
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
