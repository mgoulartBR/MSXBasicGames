# Extensao opt-in do teste openMSX: E1M1_GAMEPLAY=1.
# Cenas preparadas no debugger; tiros pelo teclado, CPU e renderizacao reais do fork.
foreach line [split $header "\n"] {
    if {[regexp {^#define (\w+) (-?\d+)$} $line -> name value]} {set gp_def($name) $value}
}
foreach table {digit face} {
    regexp [format {%s_tab[^=]+=([^\n]+);} $table] $header -> body
    set gp_table($table) {}
    foreach row [regexp -all -inline {\{[0-9,]+\}} $body] {
        lappend gp_table($table) [split [string range $row 1 end-1] ,]
    }
}
proc gp_read {i field} {debug read memory [expr {$::e1m1_symbol(th)+11*$i+$field}]}
proc gp_capture {name} {
    set page [expr {([debug read {VDP regs} 2]>>5)&1}]
    set f [open [file join $::output $name.vram] wb]
    puts -nonewline $f [debug read_block VRAM [expr {$page*65536}] 54272]
    close $f
}
proc gp_write {i field value {size 1}} {
    for {set n 0} {$n<$size} {incr n} {
        debug write memory [expr {$::e1m1_symbol(th)+11*$i+$field+$n}] [expr {($value>>(8*$n))&255}]
    }
}
proc gp_actor {i kind hp x y state timer} {
    gp_write $i 0 $x 2; gp_write $i 2 $y 2; gp_write $i 4 $hp 2
    gp_write $i 6 $kind; gp_write $i 7 $state; gp_write $i 8 $timer
    gp_write $i 9 1; gp_write $i 10 1
}
proc gp_reset {} {
    for {set i 0} {$i<$::gp_def(NTHING)} {incr i} {gp_write $i 9 0}
    wr16 px 1056; wr16 py -3616; wr16 eye 41
    wr16 hp 100; wr16 armor 0; wr16 ammo 50
    foreach n {over yaw cool fire_t kills dbg} {wr8 $n 0}
    keymatrixup 8 1
}
proc gp_blit {sx sy w h dx dy} {
    for {set y 0} {$y<$h} {incr y} {
        for {set x 0} {$x<$w} {incr x} {
            # SCREEN 8 + EVR: mesma transformacao de LogicalVRAMDebuggable; este so expoe 128K.
            set a [expr {($sy+$y)*256+$sx+$x}]
            set physical [expr {($a&0x20000)|(($a<<16)&0x10000)|(($a>>1)&0xFFFF)}]
            set c [string index $::gp_physical $physical]
            if {$c eq "\x00"} {continue}
            set at [expr {($dy+$y)*256+$dx+$x}]
            set ::gp_expected [string replace $::gp_expected $at $at $c]
        }
    }
}
proc gameplay_start {} {
    gp_reset
    wr16 hp 111; wr16 armor 111; wr16 ammo 111
    await_frames 3 gp_hud
}
proc gp_hud {} {
    set ::gp_physical [debug read_block {physical VRAM} 0 [debug size {physical VRAM}]]
    set ::gp_expected [string repeat "\x00" 8704]
    gp_blit $::gp_def(BAR_X) $::gp_def(BAR_Y) $::gp_def(BAR_W) $::gp_def(BAR_H) 0 0
    gp_blit 0 $::gp_def(ARMS_Y) $::gp_def(ARMS_W) $::gp_def(ARMS_H) 83 0
    foreach {value right percent} {111 72 1 111 177 1 111 35 0} {
        if {$percent} {gp_blit {*}[lindex $::gp_table(digit) 10] $right 3}
        foreach ch [lreverse [split $value ""]] {
            incr right -[lindex [lindex $::gp_table(digit) 0] 2]
            gp_blit {*}[lindex $::gp_table(digit) $ch] $right 3
        }
    }
    gp_blit [expr {$::gp_def(BAR_X)+114}] $::gp_def(BAR_Y) 26 $::gp_def(BAR_H) 114 0
    lassign [lindex $::gp_table(face) 0] sx sy w h dx dy
    gp_blit $sx $sy $w $h $dx $dy
    foreach page {0 1} {
        set actual [debug read_block VRAM [expr {$page*65536+178*256}] 8704]
        check [expr {$actual eq $::gp_expected}] "HUD 111/111/111 alinhado e sem pixels vizinhos, pagina $page"
    }
    screenshot -raw [file join $::output hud-111.png]
    gp_capture hud-111
    set ::gp_kind 0
    gp_death_start
}
proc gp_death_start {} {
    if {$::gp_kind==3} {gp_barrel_start; return}
    gp_reset
    gp_actor 0 $::gp_kind 0 1026 -3536 4 0
    set ::gp_seen {}; set ::gp_frames 0
    await_frames 1 gp_death_frame
}
proc gp_death_frame {} {
    set state [gp_read 0 7]; set t [gp_read 0 8]
    set phase [expr {$state==5 ? 4 : $t/2}]
    if {$phase ni $::gp_seen} {
        lappend ::gp_seen $phase
        gp_capture death-$::gp_kind-$phase
        after time 0.020 [list screenshot -raw [file join $::output death-$::gp_kind-$phase.png]]
    }
    incr ::gp_frames
    if {$::gp_frames<12} {await_frames 1 gp_death_frame; return}
    check [expr {$::gp_seen eq {0 1 2 3 4} && $state==5 && [gp_read 0 9]==1 && [gp_read 0 10]==1}] "inimigo $::gp_kind: cinco quadros de queda e cadaver visivel persistente ($::gp_seen, estado $state)"
    incr ::gp_kind
    gp_death_start
}
proc gp_barrel_start {} {
    gp_reset; wr16 armor 90
    gp_actor 0 14 20 1056 -3536 0 0
    gp_actor 1 14 20 1106 -3536 0 0
    gp_actor 2 0 20 1056 -3506 3 250
    set ::gp_seen {}; set ::gp_frames 0
    keymatrixdown 8 1
    await_frames 1 gp_barrel_frame
}
proc gp_barrel_frame {} {
    set state [gp_read 0 7]; set t [gp_read 0 8]
    if {$state>=4} {keymatrixup 8 1}
    if {$state==4} {
        set phase [expr {$t/2}]
        if {$phase ni $::gp_seen} {
            lappend ::gp_seen $phase
            gp_capture barrel-$phase
            after time 0.020 [list screenshot -raw [file join $::output barrel-$phase.png]]
        }
    }
    incr ::gp_frames
    if {$::gp_frames<70 && ([gp_read 0 9] || [gp_read 1 9])} {await_frames 1 gp_barrel_frame; return}
    check [expr {$::gp_seen eq {0 1 2 3 4}}] "barril atingido pelo teclado: cinco quadros de explosao"
    check [expr {![gp_read 0 9] && ![gp_read 1 9] && [rd8 kills]==1}] "explosao remove barris em cadeia e abate inimigo uma vez"
    check [expr {[rd16 hp]==36 && [rd16 armor]==58 && [rd16 ammo]<50}] "tiros gastam municao; explosoes ferem jogador e consomem armadura"
    wr8 over 1; keymatrixdown 8 1
    await_frames 1 gp_restart
}
proc gp_restart {} {
    keymatrixup 8 1
    check [expr {[rd8 over]==0 && [rd16 hp]==100}] "reinicio apos combate"
    await_frames 3 start_benchmark
}
