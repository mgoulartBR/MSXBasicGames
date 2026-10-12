# Amostra quadros (vbc) e voltas ociosas (idlecnt, 67 T cada) em janelas de 1 s EMULADO
# e estima a folga de CPU. Uso: openmsx -machine C-BIOS_MSX2+ -cart zones.rom -script zbench.tcl
set throttle off
set f [open zbench.txt w]
set ::v0 0; set ::i0 0
proc a {} { set ::v0 [peek16 0xC100]; set ::i0 [peek16 0xC102] }
proc b {n} {
    global f
    set dv [expr {([peek16 0xC100] - $::v0) & 0xFFFF}]
    set di [expr {([peek16 0xC102] - $::i0) & 0xFFFF}]
    puts $f "janela$n: quadros=$dv voltas_ociosas=$di folga_cpu=[format %.1f [expr {$di * 67.0 / 3579545 * 100}]]%"
}
foreach t {10 13 16} {
    after time $t a
    after time [expr {$t + 1}] [list b $t]
}
after time 18 { close $f; exit }
