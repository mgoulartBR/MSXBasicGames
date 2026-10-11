# Controlled audio/host benchmark, NOT full-map gameplay FPS.
# Reuses the Geo3D fork's page-flip counter and actor/debugger helpers.
set root [file dirname [file dirname [info script]]]
source [file join $root tools test_e1m1_openmsx.tcl]
source [file join $root tools test_e1m1_gameplay_openmsx.tcl]
set perf_out [open [file join $output performance.tsv] w]
puts $perf_out "scene\tframes\temulated_seconds\twall_seconds\temulated_fps\twall_fps\tcpu\tmoon\tx\ty\tyaw"
set perf_scenes {idle shooting turn walk}
set perf_index 0
set perf_frames 900
if {[info exists env(E1M1_PERF_FRAMES)]} {set perf_frames $env(E1M1_PERF_FRAMES)}
proc boot {} {
    check [expr {[debug read {S1990 regs} 6]==64}] "R800 enabled"
    check [expr {[rd16 hp]==100 && $::flips>3}] "boot ready"
    record start -audioonly -stereo [file join $::output performance.wav]
    perf_scene
}
proc perf_scene {} {
    if {$::perf_index==[llength $::perf_scenes]} {
        record stop
        puts $::log "ALL PERFORMANCE SCENES COMPLETED"
        close $::perf_out; close $::log; exit; return
    }
    gp_reset
    keymatrixup 8 0xF1
    wr8 tick 0; wr8 hurt 0; wr8 face_t 0
    wr16 hp 30000; wr16 ammo 30000
    set ::perf_name [lindex $::perf_scenes $::perf_index]
    if {$::perf_name eq "shooting"} {
        gp_actor 0 0 30000 1056 -3460 1 0
        gp_actor 1 1 30000 1116 -3416 1 0
        gp_actor 2 2 30000 956 -3400 1 0
        keymatrixdown 8 1
    }
    if {$::perf_name eq "turn"} {keymatrixdown 8 0x80}
    if {$::perf_name eq "walk"} {keymatrixdown 8 0x20}
    await_frames 10 perf_measure
}
proc perf_measure {} {
    set ::perf_begin [machine_info time]
    set ::perf_wall [clock milliseconds]
    set ::perf_flips $::flips
    await_frames $::perf_frames perf_end
}
proc perf_end {} {
    set n [expr {$::flips-$::perf_flips}]
    set emu [expr {[machine_info time]-$::perf_begin}]
    set wall [expr {([clock milliseconds]-$::perf_wall)/1000.0}]
    set moon 0
    if {[info exists ::e1m1_symbol(audio_moon)]} {set moon [rd8 audio_moon]}
    puts $::perf_out "$::perf_name\t$n\t$emu\t$wall\t[expr {$n/$emu}]\t[expr {$n/$wall}]\t[debug read {S1990 regs} 6]\t$moon\t[rd16 px]\t[rd16 py]\t[rd8 yaw]"
    flush $::perf_out
    incr ::perf_index
    perf_scene
}
