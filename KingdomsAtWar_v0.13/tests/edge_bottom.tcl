# Borda inferior: o mouse no fim da tela (abaixo do HUD) rola o mapa para baixo; sobre o HUD nao rola.
source mouse_helpers.tcl
proc eb_case {} {
 delay 45;mouse_open
 mp 100 100
 set y0 [w cam_y]
 mp 100 180;delay 3
 ck hud_does_not_scroll {[w cam_y]==$y0}
 mp 100 204;delay 3
 ck bottom_edge_scrolls_down {[w cam_y]>$y0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine eb_run guard eb_case
after realtime 300 {puts stderr "FAIL eb watchdog";exit}
