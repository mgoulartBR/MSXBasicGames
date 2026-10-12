# mede quadros/s em tempo EMULADO entre t=8 e t=18 (a C-BIOS leva ~3 s para iniciar)
set throttle off
set f [open bench.txt w]
set ::c0 0
after time 8  { set ::c0 [peek16 0xC100] }
after time 18 {
    set c1 [peek16 0xC100]
    puts $f "frames_em_10s=[expr {$c1 - $::c0}] fps=[expr {($c1 - $::c0) / 10.0}]"
    close $f
    exit
}
