# Aperta ESPACO no titulo e salva a VRAM em T segundos (env: VRAM, SHOTS="t1 t2 ...", OUT prefixo)
set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.1 [list keymatrixup $row $mask] }
after time 8 { tap 8 0x01 }
set i 0
foreach t $::env(SHOTS) {
    after time $t [list apply {{i} {
        save_debuggable VRAM $::env(VRAM).$i
        set fd [open $::env(VRAM).$i.r2 w]; puts $fd [debug read "VDP regs" 2]; close $fd
    }} $i]
    incr i
}
after time [expr {[lindex $::env(SHOTS) end] + 1}] { exit }
