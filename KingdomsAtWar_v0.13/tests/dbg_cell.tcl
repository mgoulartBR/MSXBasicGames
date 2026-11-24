# Inspetor de celula (F1): mostra X,Y,S,A,M,T da celula sob o mouse na linha livre do HUD.
source helpers011.tcl
proc dc_case {} {
 delay 45
 keymatrixdown 6 32;delay 0.3;keymatrixup 6 32;delay 1.5
 shot dbg-cell-on
 set a [expr {0x7E000+3*256+3*4}];puts stderr "INFO vram name=[expr {[debug read {Sunrise GFX9000 VRAM} $a]+256*[debug read {Sunrise GFX9000 VRAM} [expr {$a+1}]]}]"
 ck inspector_on {[b dbg_don]==1}
 keymatrixdown 6 32;delay 0.3;keymatrixup 6 32;delay 1.5
 keymatrixdown 6 64;delay 0.3;keymatrixup 6 64;delay 1.5;shot dbg-cell-redraw
 ck inspector_off {[b dbg_don]==0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine dc_run guard dc_case
after realtime 300 {puts stderr "FAIL dbg watchdog";exit}
