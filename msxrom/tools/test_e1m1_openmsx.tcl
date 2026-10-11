# Teste automatizado e FPS por trocas de pagina no fork Geo3D do openMSX.
# Executar com a ROM privada, maquina Geo3D e -script tools/test_e1m1_openmsx.tcl.
set root [file dirname [file dirname [info script]]]
source [file join $root tools enable_cbios_r800.tcl]
set output [file join $root private openmsx]
if {[info exists env(E1M1_TEST_OUT)]} {set output $env(E1M1_TEST_OUT)}
file mkdir $output
set log [open [file join $output test.log] w]
set map_path [file join $root out e1m1.map]
if {[info exists env(E1M1_MAP)]} {set map_path $env(E1M1_MAP)}
set f [open $map_path r]
foreach line [split [read $f] "\n"] {
    if {[regexp {^\s+([0-9A-F]{8})\s+_(\w+)\s} $line -> addr name]} {
        set e1m1_symbol($name) [expr 0x$addr]
    }
}
close $f
set header_path [file join $root private e1m1_assets.h]
if {[info exists env(E1M1_HEADER)]} {set header_path $env(E1M1_HEADER)}
set f [open $header_path r]
set header [read $f]
close $f
if {[info exists env(E1M1_GAMEPLAY)] && $env(E1M1_GAMEPLAY)} {
    source [file join $root tools test_e1m1_gameplay_openmsx.tcl]
}
regexp {#define FONT_GRAY (\d+)} $header -> fontrow
regexp {#define C_BLACK (\d+)} $header -> black
regexp {#define FONT_CHARS "([^"]+)"} $header -> fontchars
set flips 0
debug set_watchpoint write_io 0x99 {$::wp_last_value == 0x82 && [debug read {VDP register latch status} 0]==0} {
    incr ::flips
    if {[info exists ::callback] && $::flips >= $::target} {
        set command $::callback; unset ::callback
        after time 0 [list checked_callback $command]
    }
}
set throttle off
set maxframeskip 0
set power on

proc rd8 {name} {debug read memory $::e1m1_symbol($name)}
proc rd16 {name} {
    set v [expr {[rd8 $name]+256*[debug read memory [expr {$::e1m1_symbol($name)+1}]]}]
    expr {$v>=32768 ? $v-65536 : $v}
}
proc wr8 {name value} {debug write memory $::e1m1_symbol($name) [expr {$value&255}]}
proc wr16 {name value} {
    wr8 $name $value
    debug write memory [expr {$::e1m1_symbol($name)+1}] [expr {($value>>8)&255}]
}
proc check {condition description} {
    puts $::log "[expr {$condition ? {PASS} : {FAIL}}] $description"
    flush $::log
    if {!$condition} {close $::log; exit; error $description}
}
proc await_frames {n callback} {
    set ::target [expr {$::flips+$n}]
    set ::callback $callback
}
proc checked_callback {command} {
        if {[catch {eval $command} error options]} {
            puts $::log "FAIL callback: [dict get $options -errorinfo]"
            close $::log; exit
        }
}
proc panel {} {
    set base [expr {(([debug read {VDP regs} 2]>>5)&3)*65536+12*256+2}]
    set all [debug read_block VRAM $base 2816]
    set out ""
    for {set y 0} {$y<11} {incr y} {append out [string range $all [expr {$y*256}] [expr {$y*256+147}]]}
    return $out
}
proc check_text {} {
    set angle [expr {(90-[rd8 yaw]*45/32+360)%360}]
    set line "X[rd16 px] Y[rd16 py] A$angle"
    set expected [string repeat [binary format c $::black] 1628]
    set atlas [debug read_block VRAM [expr {$::fontrow*256}] 1792]
    for {set i 0} {$i<[string length $line]} {incr i} {
        set gx [expr {[string first [string index $line $i] $::fontchars]*6}]
        for {set y 0} {$y<7} {incr y} {
            for {set x 0} {$x<5} {incr x} {
                binary scan [string index $atlas [expr {$y*256+$gx+$x}]] c color
                if {$color==0} {set color $::black}
                set at [expr {($y+2)*148+2+$i*6+$x}]
                set expected [string replace $expected $at $at [binary format c $color]]
            }
        }
    }
    check [expr {[panel] eq $expected}] "pixels do overlay: $line"
}
proc boot {} {
    puts $::log "CPU register: [debug read {S1990 regs} 6]; MSXVER: [debug read memory 0x2D]"
    check [expr {$::flips>3 && [rd16 hp]==100}] "boot e quadros completos"
    check [expr {[debug read {S1990 regs} 6]==64}] "modo R800 ROM"
    check [expr {[rd8 dbg]==0}] "TAB inicialmente desligado"
    screenshot -raw [file join $::output start.png]
    set ::baseline [panel]
    keymatrixdown 7 8
    await_frames 3 tab_on
}
proc tab_on {} {
    check [expr {[rd8 dbg]==1}] "TAB liga overlay"
    check_text
    screenshot -raw [file join $::output tab-on.png]
    await_frames 5 held
}
proc held {} {
    check [expr {[rd8 dbg]==1}] "TAB segurado nao alterna novamente"
    keymatrixup 7 8
    await_frames 3 press_off
}
proc press_off {} {keymatrixdown 7 8; await_frames 3 tab_off}
proc tab_off {} {
    check [expr {[rd8 dbg]==0 && [panel] eq $::baseline}] "TAB desliga e limpa pagina"
    await_frames 1 other_page
}
proc other_page {} {
    check [expr {[panel] eq $::baseline}] "segunda pagina limpa"
    screenshot -raw [file join $::output tab-off.png]
    keymatrixup 7 8
    set ::old_y [rd16 py]
    keymatrixdown 8 32
    await_frames 6 moved
}
proc moved {} {
    keymatrixup 8 32
    check [expr {[rd16 py]>$::old_y}] "seta para cima move jogador"
    set ::old_yaw [rd8 yaw]
    keymatrixdown 8 128
    await_frames 3 turned
}
proc turned {} {
    keymatrixup 8 128
    check [expr {[rd8 yaw]!=$::old_yaw}] "seta para direita gira jogador"
    wr8 dbg 1
    await_frames 3 coords_updated
}
proc coords_updated {} {
    check_text
    set ::cardinal 0
    next_cardinal
}
proc next_cardinal {} {
    if {$::cardinal==4} {
        if {[llength [info commands gameplay_start]]} {gameplay_start} else {start_benchmark}
        return
    }
    wr8 yaw [expr {$::cardinal*64}]
    await_frames 3 cardinal_test
}
proc cardinal_test {} {
    check_text
    incr ::cardinal
    next_cardinal
}
proc start_benchmark {} {
    set ::results [open [file join $::output fps.tsv] w]
    puts $::results "scene\tx\ty\tyaw\tdbg\tframes\tseconds\tfps"
    set ::scenes {{start-tab-off 1056 -3616 0 0} {start-tab-on 1056 -3616 0 1} {start-east 1056 -3616 64 0} {start-south 1056 -3616 128 0} {start-west 1056 -3616 192 0} {doorway 1456 -2496 64 0} {combat 2192 -2432 64 0}}
    if {[info exists ::env(E1M1_SCENES)]} {set ::scenes $::env(E1M1_SCENES)}
    set ::scene 0
    begin_scene
}
proc begin_scene {} {
    if {$::scene==[llength $::scenes]} {
        puts $::log "ALL TESTS PASSED"
        close $::results; close $::log; exit; return
    }
    lassign [lindex $::scenes $::scene] name x y yaw dbg
    wr16 px $x; wr16 py $y; wr8 yaw $yaw; wr8 dbg $dbg; wr16 hp 100
    await_frames 4 measure
}
proc measure {} {
    set ::begin_time [machine_info time]
    set ::begin_flips $::flips
    await_frames 60 finish_scene
}
proc finish_scene {} {
    lassign [lindex $::scenes $::scene] name x y yaw dbg
    set n [expr {$::flips-$::begin_flips}]
    set seconds [expr {[machine_info time]-$::begin_time}]
    puts $::results "$name\t$x\t$y\t$yaw\t$dbg\t$n\t$seconds\t[expr {$n/$seconds}]"
    flush $::results
    screenshot -raw [file join $::output $name.png]
    incr ::scene
    begin_scene
}
after time 12 boot
after time 300 {puts $::log "FAIL timeout"; close $::log; exit}
