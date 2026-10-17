# Acompanha uma corrida: imprime estado/posicoes a cada 3 s emulados; no fim salva a VRAM.
set throttle off
proc w {a} { return [expr {[peek $a] + 256 * [peek [expr {$a + 1}]]}] }
set ::log [open $::env(OUT) w]
proc tick {} {
    puts $::log "t=[format %5.1f [machine_info time]] state=[w 0xF014] pos=[w 0xF008] opos=[w 0xF016] mph=[w 0xF006] opp_mph=[w 0xF01E] gear=[w 0xF012] rpm=[w 0xF010] res=[w 0xF018] time10=[w 0xF01A] avg=[w 0xF01C]"
    flush $::log
    after time 1.5 tick
}
after time 4 tick
after time [expr {$::env(DUR)}] {
    save_debuggable VRAM $::env(VRAM)
    set fd2 [open $::env(OUT).r2 w]; puts $fd2 [debug read "VDP regs" 2]; close $fd2
    close $::log; exit
}
