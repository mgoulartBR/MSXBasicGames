# Interface: minimapa (desenho, clique move a camera, TAB esconde), grupos de controle (CTRL+n / n), B vai ao castelo, P liga/desliga o som.
# Rodar com KAW_MENU=1 KAW_OBJ=1 KAW_MACHINE=C-BIOS_MSX2+.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc layerA {y xb} {return [vrd [expr {$y*128+$xb}]]}
proc ui_case {} {
 delay 38
 tapkey 7 128;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 delay 3
 ck map64 {[b map_w]==64}
 shot ui-minimap
 ck frame_top {[layerA 109 103]==0x0A && [layerA 109 110]==0xAA}
 # castelo humano (8..11, 50..53): px 6, linha 37 => byte 104+3, nibble alto = 15
 set py [expr {110+38}]
 ck castle_on_minimap {([layerA $py 107]>>4)==15}
 # clique no minimapa: centro (231,134) => celula (30,32)
 debug write memory $::S(mouse_x) 231;debug write memory [expr {$::S(mouse_x)+1}] 0
 debug write memory $::S(mouse_y) 134;debug write memory [expr {$::S(mouse_y)+1}] 0
 delay 0.3
 keymatrixdown 5 128;delay 0.3;keymatrixup 5 128;delay 0.6
 puts stderr "INFO cam after click [w cam_x],[w cam_y]"
 ck minimap_click_moves_camera {abs([w cam_x]-360)<=24 && abs([w cam_y]-440)<=24}
 delay 1
 shot ui-minimap-jumped
 # TAB esconde
 tapkey 7 8;delay 0.3
 ck minimap_hidden {[layerA 109 103]==0 && [layerA 130 110]==0}
 tapkey 7 8;delay 1.5
 ck minimap_back {[layerA 109 103]==0x0A}
 # B: camera ao castelo
 tapkey 2 128
 puts stderr "INFO cam after B [w cam_x],[w cam_y]"
 ck b_centers_castle {abs([w cam_x]-40)<=24 && [w cam_y]>=([b pb_y]-8)*16-40}
 # grupos: CTRL+1 guarda a selecao 3; limpa; 1 recupera
 debug write memory $::S(selection_mask) 3
 keymatrixdown 6 2;keymatrixdown 0 2;delay 0.3;keymatrixup 0 2;keymatrixup 6 2;delay 0.5
 debug write memory $::S(selection_mask) 0
 tapkey 0 2
 ck group_recalled {[b selection_mask]==3}
 # P: som liga/desliga
 ck fm_on {[b fm_ok]==1}
 tapkey 4 32
 ck sound_off {[b fm_ok]==0 && [b ms 0]==0}
 tapkey 4 32
 ck sound_on {[b fm_ok]==1 && [b ms 0]==1}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine ui_run guard ui_case
after realtime 400 {puts stderr "FAIL ui watchdog";exit}
