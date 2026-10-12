# Full-game benchmarks; restores all actors/doors/projectiles before each case.
source [file join [file dirname [info script]] test_e1m1_openmsx.tcl]
source [file join $root tools test_e1m1_gameplay_openmsx.tcl]
set area_listing [file join $root out e1m1.lst]
if {[info exists env(E1M1_LIST)]} {set area_listing $env(E1M1_LIST)}
set f [open $area_listing];set listing [read $f];close $f
regexp {\n\s*([0-9A-F]{6})\s+\d+\s+_px:} $listing all px_offset
set data_base [expr {$e1m1_symbol(px)-[scan $px_offset %x]}]
set f [open $map_path];set maptext [read $f];close $f
regexp {\n_INITIALIZED\s+([0-9A-F]{8})} $maptext all init_base
foreach {name base} [list fcnt $data_base rnd_s [scan $init_base %x]] {
    regexp [format {\n\s*([0-9A-F]{6})\s+\d+\s+_%s:} $name] $listing all offset
    set e1m1_symbol($name) [expr {$base+[scan $offset %x]}]
}
set area_cases {
    {start 1056 -3616 0 0}
    {doorway 1456 -2496 64 0}
    {combat 2192 -2432 64 0}
    {hall-east 3008 -3472 64 0}
    {platform-near 3248 -3504 64 0}
    {hall-north 3216 -3376 0 0}
    {hall-west 3216 -3376 192 0}
    {hall-walk 3008 -3472 64 32}
    {hall-turn 3008 -3472 64 128}
}
set area_case 0
set area_results [open [file join $output area-fps.tsv] w]
puts $area_results "scene\tx\ty\tyaw\tinput\tframes\tfps\tfinal_x\tfinal_y"
proc boot {} {
    check [expr {[debug read {S1990 regs} 6]==64}] "R800 profile"
    area_start
}
proc area_start {} {
    if {$::area_case==[llength $::area_cases]} {
        puts $::log "ALL AREA BENCHMARKS PASSED"
        close $::area_results;close $::log;exit;return
    }
    keymatrixup 8 0xF1
    wr8 over 1;keymatrixdown 8 1
    await_frames 3 area_place
}
proc area_place {} {
    keymatrixup 8 1
    lassign [lindex $::area_cases $::area_case] name x y yaw input
    wr16 px $x;wr16 py $y;wr8 yaw $yaw;wr8 dbg 0;wr16 hp 30000;wr8 over 0
    wr8 fcnt 0;wr8 tick 0;wr8 rnd_s 77
    await_frames 8 area_measure
}
proc area_measure {} {
    lassign [lindex $::area_cases $::area_case] name x y yaw input
    if {$input} {keymatrixdown 8 $input}
    set ::area_time [machine_info time];set ::area_flips $::flips
    await_frames 60 area_done
}
proc area_done {} {
    keymatrixup 8 0xF1
    lassign [lindex $::area_cases $::area_case] name x y yaw input
    set n [expr {$::flips-$::area_flips}]
    set fps [expr {$n/([machine_info time]-$::area_time)}]
    puts $::area_results "$name\t$x\t$y\t$yaw\t$input\t$n\t$fps\t[rd16 px]\t[rd16 py]"
    flush $::area_results
    gp_capture $name
    incr ::area_case;area_start
}
