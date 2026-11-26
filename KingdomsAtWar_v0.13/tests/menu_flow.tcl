# Titulo, menu e idioma: o titulo aparece durante a carga, ENTER abre o menu, a seta muda o item, NOVO JOGO inicia o jogo.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc mf_case {} {
 delay 20;shot menu-loading
 delay 18
 shot menu-title
 tapkey 7 128;shot menu-main
 tapkey 8 64;shot menu-main-down
 tapkey 7 128;shot menu-language-list
 tapkey 8 64;tapkey 7 128
 ck language_english {[b lang]==1}
 shot menu-main-english
 tapkey 8 32;tapkey 7 128
 shot menu-after
 delay 4
 ck game_started {[w loops]>0}
 shot menu-game
 puts stderr "RESULT failures=$::failures";exit
}
coroutine mf_run guard mf_case
after realtime 400 {puts stderr "FAIL menu watchdog";exit}
