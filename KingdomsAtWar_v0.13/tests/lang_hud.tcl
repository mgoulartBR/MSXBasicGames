# HUD e mensagens no idioma escolhido (KAW_LANG=0..3): contexto, linha de botoes e rotulos; capturas hud-<idioma>-*.
source helpers011.tcl
proc lh_case {} {
 set l $::env(KAW_LANG)
 delay 45
 shot hud-$l-kingdom
 choose 0;delay 1;shot hud-$l-worker
 choose 2;delay 1;shot hud-$l-sword
 debug write memory $::S(selected) 255;debug write memory $::S(selection_mask) 0
 debug write memory $::S(building_selected) 1;debug write memory $::S(hud_dirty) 3;delay 1;shot hud-$l-castle
 debug write memory $::S(building_selected) 0;choose 0;debug write memory $::S(placement_mode) 1;debug write memory $::S(build_kind) 3;debug write memory $::S(hud_dirty) 3;delay 1;shot hud-$l-place
 puts stderr "RESULT failures=$::failures";exit
}
coroutine lh_run guard lh_case
after realtime 400 {puts stderr "FAIL lang hud watchdog";exit}
