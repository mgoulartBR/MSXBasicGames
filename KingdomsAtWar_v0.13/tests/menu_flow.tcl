# Titulo, menu e idioma: o titulo aparece durante a carga, ENTER abre o menu, a lista de idiomas muda o idioma (PT/ES/JA com
# acentos e kana), NOVO JOGO inicia o jogo. Rodar com KAW_MENU=1.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc mf_case {} {
 delay 20;shot menu-loading
 delay 18
 shot menu-title
 tapkey 7 128;shot menu-main
 tapkey 8 64;tapkey 8 64;tapkey 7 128;shot menu-language-list   ;# NOVO JOGO / OPCOES / IDIOMA
 tapkey 8 64;tapkey 8 64;tapkey 7 128
 ck language_spanish {[b lang]==2}
 shot menu-main-es
 tapkey 7 128;tapkey 8 64;tapkey 7 128
 ck language_japanese {[b lang]==3}
 shot menu-main-ja
 tapkey 8 32;tapkey 8 32;tapkey 7 128;tapkey 7 128   ;# NOVO JOGO -> HUMANOS
 delay 6;shot menu-briefing
 tapkey 7 128
 delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}
 ck game_started {[w loops]>0 && [w game_seconds]>=1}
 shot menu-game
 puts stderr "RESULT failures=$::failures";exit
}
coroutine mf_run guard mf_case
after realtime 400 {puts stderr "FAIL menu watchdog";exit}
