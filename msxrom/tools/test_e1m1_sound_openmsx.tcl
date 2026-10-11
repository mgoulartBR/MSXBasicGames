# Opt-in E1M1_AUDIO_TEST=1. Real fork output; samples are triggered by the Z80 driver.
proc audio_start {} {
    set ::audio_expected 1
    if {[info exists ::env(E1M1_EXPECT_MOON)]} {set ::audio_expected $::env(E1M1_EXPECT_MOON)}
    check [expr {[rd8 audio_moon]==$::audio_expected}] "MoonSound detection=$::audio_expected"
    gp_reset
    if {$::audio_expected} {
        set ::wave "E1M1 MoonSound wave regs"
        set ::wave_mem "E1M1 MoonSound wave mem"
        check [expr {([debug read $::wave 2]&0x1F)==0x10}] "RAM header bank 4; CPU memory access disabled"
        set f [open [file join $::root private e1m1.rom] rb]
        seek $f [expr {$::gp_def(SFX_BANK0)*16384}]
        set expected [read $f [expr {$::gp_def(NSFX_BANKS)*16384}]]; close $f
        check [expr {[debug read_block $::wave_mem 0x200000 [string length $expected]] eq $expected}] "sample RAM loaded byte-identically from ROM"
        check [expr {[debug read {PSG regs} 8]==0}] "PSG silent with MoonSound"
    }
    record start -audioonly -stereo [file join $::output keyboard-shot.wav]
    keymatrixdown 8 1
    await_frames 1 audio_keyboard
}
proc audio_keyboard {} {
    keymatrixup 8 1
    check [expr {[rd16 ammo]==49}] "keyboard shot consumes ammo"
    if {$::audio_expected} {
        check [expr {[rd8 audio_next]==1 && [debug read $::wave 0x08]==128}] "keyboard shot starts PCM pistol in voice 0"
    } else {
        await_frames 1 audio_psg_loudness
        return
    }
    after time 2 [list checked_callback audio_single_start]
}
proc audio_psg_loudness {} {
    check [expr {[debug read {PSG regs} 8]<=6}] "fallback PSG volume capped at 6/15"
    await_frames 4 audio_psg_finished
}
proc audio_psg_finished {} {
    check [expr {[rd8 sfx]==0 && [debug read {PSG regs} 8]==0}] "fallback silent after three ticks"
    record stop
    gameplay_start
}
proc audio_single_start {} {
    record stop
    set ::audio_id 0
    audio_single
}
proc audio_single {} {
    if {$::audio_id==$::gp_def(NSFX)} {
        record start -audioonly -stereo [file join $::output simultaneous.wav]
        audio_invoke 0 {audio_invoke 6 {audio_invoke 4 audio_overlap}}
        return
    }
    record start -audioonly -stereo [file join $::output sample-$::audio_id.wav]
    audio_invoke $::audio_id audio_single_check
}
proc audio_single_check {} {
    set voice [expr {([rd8 audio_next]-1)&7}]
    check [expr {[debug read $::wave [expr {8+$voice}]]==128+$::audio_id &&
                 [debug read $::wave [expr {0x20+$voice}]]==1}] "RAM wave 384+$::audio_id selected"
    check [expr {[debug read $::wave [expr {0x38+$voice}]]==0xF0 &&
                 [debug read $::wave [expr {0x50+$voice}]]==41}] "11025 Hz pitch, -15 dB voice attenuation"
    after time 2.5 [list checked_callback audio_next_single]
}
proc audio_next_single {} {
    record stop
    incr ::audio_id
    audio_single
}
proc audio_overlap {} {
    set active 0
    for {set i 0} {$i<8} {incr i} {
        if {[debug read $::wave [expr {0x68+$i}]]&128} {incr active}
    }
    check [expr {$active>=3}] "pistol, barrel and death keyed on distinct voices"
    after time 2.5 [list checked_callback audio_stop_overlap]
}
proc audio_stop_overlap {} {
    record stop
    # Test all eight voices and ring wrap in the actual driver.
    set ::audio_ring 0
    audio_ring
}
proc audio_ring {} {
    if {$::audio_ring==17} {
        check [expr {[rd8 audio_next]<8}] "eight-voice allocator wraps within reserved voices"
        # A normal in-game restart must damp all reserved voices.
        wr8 over 1
        keymatrixdown 8 1
        await_frames 2 audio_restart
        return
    }
    incr ::audio_ring
    audio_invoke 7 audio_ring
}
proc audio_restart {} {
    keymatrixup 8 1
    check [expr {[rd8 audio_next]==0 && [rd16 hp]==100}] "restart clears allocator and player state"
    for {set i 0} {$i<8} {incr i} {
        check [expr {([debug read $::wave [expr {0x68+$i}]]&0xC0)==0x40}] "restart damps voice $i"
    }
    gameplay_start
}

# Call the real SDCC function between frames, preserving CPU context and stack.
proc audio_invoke {id callback} {
    set ::audio_regs [debug read_block {CPU regs} 0 28]
    set ::audio_sp [reg SP]
    set ::audio_return [reg PC]
    set addr [expr {$::audio_sp-2}]
    set ::audio_stack [debug read_block memory $addr 2]
    debug write memory $addr [expr {$::audio_return&255}]
    debug write memory [expr {$addr+1}] [expr {$::audio_return>>8}]
    reg SP $addr; reg A $id
    set ::audio_callback $callback
    debug set_bp -once $::audio_return {} {
        debug write_block {CPU regs} 0 $::audio_regs
        debug write_block memory [expr {$::audio_sp-2}] $::audio_stack
        after time 0 [list checked_callback $::audio_callback]
        debug cont
    }
    reg PC $::e1m1_symbol(sound_play)
}
