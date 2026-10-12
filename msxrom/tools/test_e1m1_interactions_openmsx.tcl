# Opt-in extension: E1M1_GAMEPLAY=1 E1M1_INTERACTIONS=1.
# Debugger-prepared scenes; actual ROM, R800, keyboard and Geo3D fork rendering.
rename start_benchmark interactions_original_benchmark
proc start_benchmark {} {
    if {[info exists ::ig_complete]} {interactions_original_benchmark} else {ig_start}
}
proc ig_read {name offset {size 1}} {
    set a [expr {$::e1m1_symbol($name)+$offset}]
    set v [debug read memory $a]
    if {$size==2} {
        incr v [expr {256*[debug read memory [expr {$a+1}]]}]
        if {$v>=32768} {incr v -65536}
    }
    return $v
}
proc ig_write {name offset value {size 1}} {
    for {set n 0} {$n<$size} {incr n} {
        debug write memory [expr {$::e1m1_symbol($name)+$offset+$n}] [expr {($value>>(8*$n))&255}]
    }
}
proc ig_capture {name} {
    gp_capture $name
    screenshot -raw [file join $::output $name.png]
}
proc ig_start {} {
    gp_reset
    set ::ig_mode hit
    ig_launch
}
proc ig_launch {} {
    gp_reset
    gp_actor 0 2 60 1056 -3400 1 0
    set ::ig_steps 0
    await_frames 1 ig_await_shot
}
proc ig_await_shot {} {
    incr ::ig_steps
    for {set i 0} {$i<4} {incr i} {
        if {[ig_read shots [expr {$i*16+12}]]} {
            set ::ig_shot [expr {$i*16}]
            gp_write 0 9 0
            check [expr {[rd16 hp]==100}] "Imp $::ig_mode: lancamento sem dano instantaneo"
            ig_capture imp-$::ig_mode-flight
            set ::ig_shot_y [ig_read shots [expr {$::ig_shot+2}] 2]
            if {$::ig_mode eq "hit"} {
                await_frames 4 ig_flight
            } else {
                wr8 yaw 64;keymatrixdown 8 0x20
                await_frames 20 ig_dodged
            }
            return
        }
    }
    check [expr {$::ig_steps<80}] "Imp aguardando oportunidade de lancamento"
    await_frames 1 ig_await_shot
}
proc ig_flight {} {
    check [expr {[rd16 hp]==100 && [ig_read shots [expr {$::ig_shot+2}] 2]!=$::ig_shot_y}] "bola de fogo move-se antes do impacto"
    ig_capture imp-flight-second-pose
    set ::ig_steps 0
    await_frames 1 ig_impact
}
proc ig_impact {} {
    incr ::ig_steps
    if {[ig_read shots [expr {$::ig_shot+15}]]} {
        set ::ig_impact_hp [rd16 hp]
        check [expr {$::ig_impact_hp>=76 && $::ig_impact_hp<=91}] "dano ocorre no impacto do projetil"
        ig_capture imp-impact
        await_frames 8 ig_expired
        return
    }
    check [expr {$::ig_steps<40}] "aguardando impacto do projetil"
    await_frames 1 ig_impact
}
proc ig_expired {} {
    check [expr {![ig_read shots [expr {$::ig_shot+12}]] && [rd16 hp]==$::ig_impact_hp}] "explosao termina, sem repetir o dano"
    set ::ig_mode dodge
    ig_launch
}
proc ig_dodged {} {
    keymatrixup 8 0x20
    check [expr {[rd16 px]>1100 && [rd16 hp]==100}] "movimento pelo teclado esquiva a bola de fogo"
    ig_write shots [expr {$::ig_shot+12}] 1
    ig_write shots [expr {$::ig_shot+15}] 0
    ig_write shots [expr {$::ig_shot+14}] 1
    await_frames 2 ig_ttl
}
proc ig_ttl {} {
    check [expr {![ig_read shots [expr {$::ig_shot+12}]]}] "limite de vida remove projetil"
    set ::ig_door 0;set ::ig_side -1
    ig_door_start
}
proc ig_door_start {} {
    if {$::ig_door==4} {ig_reopen;return}
    gp_reset
    set at [expr {$::ig_door*13}]
    set ax [ig_read doors $at 2];set ay [ig_read doors [expr {$at+2}] 2]
    set bx [ig_read doors [expr {$at+4}] 2];set by [ig_read doors [expr {$at+6}] 2]
    set ::ig_cx [expr {($ax+$bx)/2}];set ::ig_cy [expr {($ay+$by)/2}]
    set ::ig_nx [expr {$ax==$bx}];set ::ig_ny [expr {!$::ig_nx}]
    wr16 px [expr {$::ig_cx+48*$::ig_side*$::ig_nx}]
    wr16 py [expr {$::ig_cy+48*$::ig_side*$::ig_ny}]
    wr16 eye [expr {$::ig_door==0 ? 41 : 17}]
    wr8 yaw [expr {$::ig_nx ? ($::ig_side==-1 ? 64 : 192) : ($::ig_side==-1 ? 0 : 128)}]
    await_frames 3 ig_door_view
}
proc ig_door_view {} {
    ig_capture door-$::ig_door-side-$::ig_side-closed
    keymatrixdown 8 0x20
    await_frames 8 ig_door_blocked
}
proc ig_distance {} {expr {([rd16 px]-$::ig_cx)*$::ig_nx+([rd16 py]-$::ig_cy)*$::ig_ny}}
proc ig_door_blocked {} {
    keymatrixup 8 0x20
    check [expr {[ig_distance]*$::ig_side>15 && [ig_read door_pos $::ig_door]==0}] "porta $::ig_door lado $::ig_side bloqueia passagem fechada"
    keymatrixdown 2 0x40
    await_frames 1 ig_door_opening
}
proc ig_door_opening {} {
    keymatrixup 2 0x40
    check [expr {[ig_read door_state $::ig_door]==1 && [ig_read door_pos $::ig_door]==8}] "E inicia abertura da porta $::ig_door pelo lado $::ig_side"
    await_frames 4 ig_door_mid
}
proc ig_door_mid {} {
    ig_capture door-$::ig_door-side-$::ig_side-opening
    await_frames 5 ig_door_open
}
proc ig_door_open {} {
    check [expr {[ig_read door_state $::ig_door]==2 && [ig_read door_pos $::ig_door]>=56}] "porta $::ig_door abriu"
    ig_capture door-$::ig_door-side-$::ig_side-open
    keymatrixdown 8 0x20
    await_frames 16 ig_door_crossed
}
proc ig_door_crossed {} {
    keymatrixup 8 0x20
    check [expr {[ig_distance]*$::ig_side< -15}] "movimento cruza porta $::ig_door aberta pelo lado $::ig_side"
    wr16 px 1056;wr16 py -3616
    ig_write door_hold $::ig_door 1
    await_frames 24 ig_door_closed_again
}
proc ig_door_closed_again {} {
    puts $::log "DOOR $::ig_door pos=[ig_read door_pos $::ig_door] state=[ig_read door_state $::ig_door] hold=[ig_read door_hold $::ig_door]"
    check [expr {[ig_read door_pos $::ig_door]==0 && [ig_read door_state $::ig_door]==0}] "porta $::ig_door fecha apos sair da area"
    if {$::ig_side==-1} {set ::ig_side 1} else {set ::ig_side -1;incr ::ig_door}
    ig_door_start
}
proc ig_reopen {} {
    gp_reset
    set ::ig_cx [expr {([ig_read doors 0 2]+[ig_read doors 4 2])/2}]
    set ::ig_cy [expr {([ig_read doors 2 2]+[ig_read doors 6 2])/2}]
    wr16 px [expr {$::ig_cx+10}];wr16 py $::ig_cy
    ig_write door_pos 0 64;ig_write door_state 0 3
    await_frames 2 ig_safe_close
}
proc ig_safe_close {} {
    check [expr {[ig_read door_pos 0]>=72 && [ig_read door_state 0] in {1 2} && [rd16 hp]==100}] "porta reabre se jogador entra durante fechamento"
    gp_reset
    wr16 px [expr {$::ig_cx+100}];wr16 py $::ig_cy
    foreach {offset size value} [list 0 2 [expr {$::ig_cx-30}] 2 2 $::ig_cy 4 2 40 6 2 10 8 2 0 10 2 0 12 1 1 13 1 0 14 1 100 15 1 0] {
        ig_write shots $offset $value $size
    }
    await_frames 3 ig_fire_door
}
proc ig_fire_door {} {
    check [expr {[ig_read shots 15] && [rd16 hp]==100}] "porta fechada bloqueia bola de fogo sem atingir jogador atras"
    set ::ig_views {{grade-front 2816 -4256 64 17} {grade-oblique 2816 -4216 90 17} {radiation-13 1636 -3268 0 -39} {radiation-55 2812 -3500 0 -7} {radiation-57 3140 -3260 0 -7} {radiation-61 3012 -3332 0 -7}}
    set ::ig_view 0
    ig_view_start
}
proc ig_view_start {} {
    if {$::ig_view==[llength $::ig_views]} {
        puts $::log "ALL INTERACTION TESTS PASSED (OPENMSX TESTED)"
        flush $::log
        set ::ig_complete 1
        wr8 over 1
        keymatrixdown 8 1
        await_frames 2 gp_restart
        return
    }
    gp_reset
    lassign [lindex $::ig_views $::ig_view] name x y yaw eye
    wr16 px $x;wr16 py $y;wr8 yaw $yaw;wr16 eye $eye
    await_frames 4 ig_view_capture
}
proc ig_view_capture {} {
    lassign [lindex $::ig_views $::ig_view] name x y yaw eye
    ig_capture $name
    if {[string match radiation-* $name]} {
        wr16 hp 100;wr16 armor 0;wr8 tick 31
        await_frames 1 ig_radiation
    } else {
        incr ::ig_view
        ig_view_start
    }
}
proc ig_radiation {} {
    check [expr {[rd16 hp]==95}] "dano periodico de radiacao preservado na vista $::ig_view"
    incr ::ig_view
    ig_view_start
}
