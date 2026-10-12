# Direct fork openMSX tests: R800 MULUW/fallback equivalence and enemy dropoffs.
source [file join [file dirname [info script]] test_e1m1_openmsx.tcl]
source [file join $root tools test_e1m1_gameplay_openmsx.tcl]
proc boot {} {
    check [expr {[debug read {S1990 regs} 6]==64}] "R800 profile"
    check [expr {[rd8 math_fast]==1}] "native R800 math detected"
    set ::math_pairs {}
    foreach a {-32768 -16384 -1 0 1 16384 32767} {
        foreach b {-32768 -16384 -1 0 1 16384 32767} {lappend ::math_pairs [list $a $b]}
    }
    expr {srand(1)}
    for {set i 0} {$i<551} {incr i} {
        lappend ::math_pairs [list [expr {int(rand()*65536)-32768}] [expr {int(rand()*65536)-32768}]]
    }
    set ::math_index 0;set ::math_mode 0
    set ::saved_regs {}
    foreach r {AF BC DE HL IX IY SP PC} {dict set ::saved_regs $r [reg $r]}
    set ::math_bp [debug set_bp 0xF100 {} {math_return}]
    debug break
    math_call
}
proc math_call {} {
    lassign [lindex $::math_pairs $::math_index] a b
    wr8 math_fast $::math_mode
    reg HL [expr {$a&65535}];reg DE [expr {$b&65535}];reg BC 0xA55A
    reg SP 0xF1FE
    debug write memory 0xF1FE 0;debug write memory 0xF1FF 0xF1
    reg PC $::e1m1_symbol(mulq14)
    debug cont
}
proc math_return {} {
    lassign [lindex $::math_pairs $::math_index] a b
    set expected [expr {((($a<0)!=($b<0) ? -1 : 1)*(abs($a)*abs($b)/16384))&65535}]
    if {[reg DE]!=$expected || [reg BC]!=0xA55A || [reg SP]!=0xF200} {
        check 0 "mulq14 mode=$::math_mode a=$a b=$b result=[reg DE] expected=$expected"
    }
    incr ::math_index
    if {$::math_index<[llength $::math_pairs]} {math_call;return}
    check 1 "600 signed math vectors mode=$::math_mode, result/BC/SP correct"
    if {!$::math_mode} {set ::math_mode 1;set ::math_index 0;math_call;return}
    debug remove_bp $::math_bp
    dict for {r v} $::saved_regs {reg $r $v}
    wr8 math_fast 1
    set ::platform_kind 0
    platform_start
    debug cont
}
proc actor16 {field} {
    set v [expr {[gp_read 0 $field]+256*[gp_read 0 [expr {$field+1}]]}]
    expr {$v>=32768 ? $v-65536 : $v}
}
proc platform_start {} {
    gp_reset
    wr16 px 3200;wr16 py -3504;wr16 eye -7;wr8 yaw 64;wr16 hp 30000
    gp_actor 0 $::platform_kind 60 3360 -3504 1 0
    set ::platform_frame 0
    await_frames 1 platform_frame
}
proc platform_frame {} {
    check [expr {[actor16 0]>=3324 && [actor16 2]==-3504}] "enemy $::platform_kind remains above dropoff frame=$::platform_frame"
    incr ::platform_frame
    if {$::platform_frame<100} {await_frames 1 platform_frame;return}
    check [expr {[actor16 0]<3352}] "enemy $::platform_kind moves on same platform"
    gp_capture "platform-enemy-$::platform_kind"
    incr ::platform_kind
    if {$::platform_kind<3} {platform_start;return}
    gp_reset
    wr16 px 3360;wr16 py -3504;wr16 eye 137;wr8 yaw 192
    keymatrixdown 8 0x20
    await_frames 20 player_drop
}
proc player_drop {} {
    keymatrixup 8 0x20
    check [expr {[rd16 px]<3312 && [rd16 eye]<137}] "player still descends high platform"
    set ::step_cases {{3152 3264 3200} {3232 3120 3176}}
    set ::step_case 0;set ::platform_kind 0
    step_start
}
proc step_start {} {
    gp_reset
    lassign [lindex $::step_cases $::step_case] mx px bound
    wr16 px $px;wr16 py -3588;wr16 hp 30000
    gp_actor 0 $::platform_kind 60 $mx -3588 1 0
    await_frames 100 step_done
}
proc step_done {} {
    lassign [lindex $::step_cases $::step_case] mx px bound
    check [expr {$::step_case==0 ? [actor16 0]>=$bound : [actor16 0]<=$bound}] "enemy $::platform_kind traverses 24-unit step direction=$::step_case"
    incr ::step_case
    if {$::step_case<2} {step_start;return}
    set ::step_case 0;incr ::platform_kind
    if {$::platform_kind<3} {step_start;return}
    puts $::log "ALL PLATFORM TESTS PASSED (OPENMSX TESTED)"
    close $::log;exit
}
