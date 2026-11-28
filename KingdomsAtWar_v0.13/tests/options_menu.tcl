# Menu de opcoes: musica liga/desliga, efeitos, volume (afeta o OPLL), voltar. Rodar com KAW_MENU=1 e KAW_MACHINE=C-BIOS_MSX2+.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc fm {r} {return [debug read "C-BIOS MSX-MUSIC regs" $r]}
proc om_case {} {
 delay 36
 ck defaults {[b opt_music]==1 && [b opt_sfx]==1 && [b opt_vol]==4}
 tapkey 7 128               ;# titulo -> menu principal
 shot opt-main
 keymatrixdown 8 64;delay 0.12;keymatrixup 8 64;delay 0.5   ;# baixo: Opcoes
 tapkey 7 128               ;# entra em Opcoes
 shot opt-menu
 # item 0: musica desliga
 tapkey 7 128
 ck music_off {[b opt_music]==0 && [b ms 0]==0}
 shot opt-music-off
 tapkey 7 128
 ck music_on {[b opt_music]==1 && [b ms 0]==1}
 # item 1: efeitos
 keymatrixdown 8 64;delay 0.12;keymatrixup 8 64;delay 0.4
 tapkey 7 128
 ck sfx_off {[b opt_sfx]==0}
 tapkey 7 128
 ck sfx_on {[b opt_sfx]==1}
 # item 2: volume: 4 -> 3 com seta esquerda; o atenuador do canal 0 sobe 2
 keymatrixdown 8 64;delay 0.12;keymatrixup 8 64;delay 0.4
 set a0 -1
 set t0 [machine_info time];while {[machine_info time]-$t0<6 && [expr {[fm 0x30]&15}]==0} {delay 0.05}
 set vol4 [expr {[fm 0x30]&15}]
 tapkey 8 16                ;# seta esquerda
 ck volume_3 {[b opt_vol]==3}
 delay 1.2
 set vol3 [expr {[fm 0x30]&15}]
 puts stderr "INFO att ch0: vol4=$vol4 vol3=$vol3"
 ck volume_changes_opll {$vol3>$vol4}
 shot opt-volume
 # voltar (item 3) e idioma continua funcionando
 keymatrixdown 8 64;delay 0.12;keymatrixup 8 64;delay 0.4
 tapkey 7 128
 shot opt-back-main
 ck back_to_main {1}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine om_run guard om_case
after realtime 200 {puts stderr "FAIL options watchdog";exit}
