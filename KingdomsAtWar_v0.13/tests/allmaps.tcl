# As 8 missoes da campanha: dimensoes, bases, fortaleza inimiga e a onda inimiga chegando a base humana em cada mapa.
# Rodar com KAW_MENU=1 KAW_OBJ=1.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc quick_win {} {debug write memory [expr {$::S(mis)+7}] 1;debug write memory [expr {$::S(mis)+8}] 3;debug write memory [expr {$::S(mis)+9}] 0}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc dist8 {} {return [expr {abs([unit 8 x]-[b pb_x])+abs([unit 8 y]-[b pb_y])}]}
# largura, altura, base humana, fortaleza inimiga
set ::E {{64 64 8 50 50 14} {64 64 5 50 46 18} {64 64 52 48 8 18} {64 64 28 50 30 16} {128 128 6 112 108 22} {128 128 60 112 66 20} {128 128 12 16 112 104} {128 128 112 108 10 26}}
proc am_case {} {
 delay 38
 tapkey 7 128;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 for {set m 0} {$m<8} {incr m} {
  lassign [lindex $::E $m] ew eh px py ex ey
  ck m${m}_size {[b map_w]==$ew && [b map_h]==$eh && [b mission_n]==$m}
  ck m${m}_bases {[b pb_x]==$px && [b pb_y]==$py && [b eb_x]==$ex && [b eb_y]==$ey && [bd 0 x]==$px && [bd 1 x]==$ex && [bd 1 y]==$ey}
  ck m${m}_ai {[b ai_enabled]==1 && [b ai_route_n]>0 && [b unit_hp 8]>0}
  # a onda (unidade 8) chega perto da base humana
  debug write memory $::S(ai_launch) 0;debug write memory [expr {$::S(ai_launch)+1}] 0
  set d0 [dist8];set t0 [machine_info time];set best $d0
  while {[machine_info time]-$t0<420 && [dist8]>16 && [b unit_hp 8]>0} {delay 2;set d [dist8];if {$d<$best} {set best $d}}
  puts stderr "INFO m$m wave dist $d0 -> best $best after [expr {[machine_info time]-$t0}]s"
  ck m${m}_wave {$best<=16 || [b unit_hp 8]==0}
  if {$m<7} {
   quick_win;await {[b game_over]==1} 15
   delay 4;tapkey 7 128;delay 6;tapkey 7 128;wait_game
  }
 }
 puts stderr "RESULT failures=$::failures";exit
}
coroutine am_run guard am_case
after realtime 3600 {puts stderr "FAIL allmaps watchdog";exit}
