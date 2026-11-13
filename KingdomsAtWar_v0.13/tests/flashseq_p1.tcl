# Golpe e dano: o clarao e o gesto de ataque aparecem, depois somem (so renderiza nas transicoes).
source kingdom_helpers.tcl
proc fs_case {} {
 delay 65
 debug write memory $::S(unit_hp) 20
 debug write memory [expr {$::S(unit_hp)+2}] 24
 debug write memory $::S(sprites_dirty) 3
 delay 1
 debug write memory $::S(attack_flash) 8
 debug write memory [expr {$::S(hurt_flash)+2}] 8
 debug write memory $::S(combat_active) 1 ;# como o golpe real: mantem o combate ativo
 debug write memory $::S(sprites_dirty) 3
 delay 0.07
 shot flashseq-strike
 delay 0.8
 ck flashes_expired {[b attack_flash]==0 && [b hurt_flash 2]==0}
 shot flashseq-after
 puts stderr "RESULT failures=$::failures";exit
}
coroutine fs_run guard fs_case
after realtime 200 {puts stderr "FAIL flashseq watchdog";exit}
