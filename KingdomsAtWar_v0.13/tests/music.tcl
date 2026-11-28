# Musica FM: o OPLL e detectado, a musica do titulo toca com o andamento e as notas compiladas, a troca por vitoria e o efeito sonoro no canal 5.
# Rodar com KAW_MACHINE=C-BIOS_MSX2+ (a ROM de MSX-MUSIC do C-BIOS) e KAW_MENU=1.
source helpers011.tcl
proc fm {r} {return [debug read "C-BIOS MSX-MUSIC regs" $r]}
proc keyon {c} {return [expr {([fm [expr {0x20+$c}]]>>4)&1}]}
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc mus_case {} {
 delay 36
 ck fm_detected {[b fm_ok]==1}
 ck title_song {[b music_cur]==0 && [b ms 0]==1}
 # reinicia a musica do titulo pelo fluxo (ms: on,delay,bank,lbank,ptr) para medir desde o compasso 1
 proc ms_restart {} {
  set a $::S(ms)
  debug write memory $a 0;debug write memory [expr {$a+1}] 0;debug write memory [expr {$a+2}] 63
  debug write memory [expr {$a+4}] 0;debug write memory [expr {$a+5}] 0xA0;debug write memory $a 1
 }
 ms_restart
 set t0 [machine_info time];set seen {};set last -1;set times {}
 while {[machine_info time]-$t0<4} {
  delay 0.02
  set f [fm 0x10]
  if {$f!=$last && [keyon 0]} {lappend seen $f;lappend times [expr {[machine_info time]-$t0}]};set last $f
 }
 puts stderr "INFO ch0 fnum low [lrange $seen 0 5] at [lrange $times 0 5]"
 set fn [expr {round(293.6648*pow(2,16)/49716.0)}]
 ck first_note_d4 {[lindex $seen 0]==($fn&255)}
 set fn [expr {round(349.2282*pow(2,16)/49716.0)}]
 ck second_note_f4 {[lindex $seen 1]==($fn&255)}
 set dt [expr {[lindex $times 1]-[lindex $times 0]}]
 ck tempo_78bpm {abs($dt-1.538)<0.12}
 # canais de baixo e bateria ativos
 set bass 0;set drum 0;set t0 [machine_info time]
 while {[machine_info time]-$t0<4} {delay 0.03;if {[keyon 2]} {set bass 1};if {[fm 0x0E]&0x1F} {set drum 1}}
 ck bass_plays {$bass==1}
 ck drums_play {$drum==1}
 # efeito sonoro: toca o efeito 'alarm' (id 9) pela estrutura sx
 # efeito 'alarm' (id 9) no canal 5 pela estrutura sx
 set tbl [regexp -inline {sfx_ptr\[\]=\{([0-9,]+)\}} [exec cat ../src/music_data.h]]
 set ptrs [split [lindex $tbl 1] ,];set bank [lindex [regexp -inline {SFX_BANK ([0-9]+)} [exec cat ../src/music_data.h]] 1]
 set p [lindex $ptrs 9];set a $::S(sx)
 debug write memory $a 0;debug write memory [expr {$a+1}] 0;debug write memory [expr {$a+2}] $bank
 debug write memory [expr {$a+4}] [expr {$p&255}];debug write memory [expr {$a+5}] [expr {$p>>8}];debug write memory $a 1
 set on 0;set t0 [machine_info time]
 while {[machine_info time]-$t0<1.2} {delay 0.02;if {[keyon 5]} {set on 1}}
 ck sfx_plays {$on==1}
 delay 0.5
 ck sfx_ends {[keyon 5]==0 && [b sx 0]==0}
 # em jogo: calma; inimigo visivel -> batalha; vitoria -> fanfarra
 tapkey 7 128;tapkey 7 128;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 delay 2
 puts stderr "INFO cur=[b music_cur] sec=[w game_seconds] uc=[b unit_count] [lmap i {8 9 10 11} {list [b unit_hp $i] [unit $i x] [unit $i y]}] base=[b pb_x],[b pb_y]"
 ck calm_in_game {[b music_cur]==1}
 set a [expr {$::S(units)+16*8}]
 set x [expr {[b pb_x]+3}];set y [expr {[b pb_y]+6}]
 debug write memory [expr {$a+4}] $x;debug write memory [expr {$a+5}] $y
 debug write memory [expr {$a+8}] $x;debug write memory [expr {$a+9}] $y;debug write memory [expr {$a+6}] $x;debug write memory [expr {$a+7}] $y
 debug write memory [expr {$a}] [expr {($x*16)&255}];debug write memory [expr {$a+1}] [expr {($x*16)>>8}]
 debug write memory [expr {$a+2}] [expr {($y*16)&255}];debug write memory [expr {$a+3}] [expr {($y*16)>>8}]
 await {[b music_cur]==2} 6
 ck battle_when_enemy_seen {[b music_cur]==2}
 debug write memory [expr {$::S(mis)+7}] 1;debug write memory [expr {$::S(mis)+8}] 3;debug write memory [expr {$::S(mis)+9}] 0
 await {[b game_over]==1} 15
 await {[b music_cur]==3} 3
 ck victory_theme {[b music_cur]==3}
 delay 6
 ck victory_ends {[b ms 0]==0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine mus_run guard mus_case
after realtime 400 {puts stderr "FAIL music watchdog";exit}
