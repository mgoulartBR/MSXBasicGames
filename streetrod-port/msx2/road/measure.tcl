# Mede fps do prototipo no openMSX (tempo emulado) e tira screenshot.
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
set shot $::env(SHOT)
after time 6 {
    set ::f0 [w 0xF000]; set ::j0 [w 0xFC9E]
    set ::t0 [machine_info time]
}
after time 26 {
    set f1 [w 0xF000]; set j1 [w 0xFC9E]; set t1 [machine_info time]
    set df [expr {($f1 - $::f0) & 0xFFFF}]; set dj [expr {($j1 - $::j0) & 0xFFFF}]
    set fd [open $::env(OUT) w]; puts $fd "RESULT frames=$df jiffies=$dj fps_per_jiffy=[format %.2f [expr {double($df) / $dj * 60.0}]] cmds=[w 0xF002] speed=[w 0xF006] pos=[w 0xF008] hud_fps10=[w 0xF004] raw=[w 0xF000],[w 0xF002],[w 0xF004],[w 0xF006],[w 0xF008] t60=[w 0xF00A] t100=[w 0xF00C] top=[w 0xF00E] gear_pos=[w 0xF008] emu_t=[format %.2f [expr {$t1-$::t0}]]"; close $fd
    save_debuggable VRAM $::env(VRAM)
    set fd2 [open $::env(OUT).r2 w]; puts $fd2 [debug read "VDP regs" 2]; close $fd2
    exit
}
