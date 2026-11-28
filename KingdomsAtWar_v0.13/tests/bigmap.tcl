# Mapa grande (campanha, missao 2 = 64x64): janela de nomes, camera, caminho longo em rotas parciais e fog.
# Rodar com KAW_MENU=1 KAW_OBJ=1.
source helpers011.tcl
proc tapkey {row mask} {keymatrixdown $row $mask;delay 0.25;keymatrixup $row $mask;delay 0.4}
proc quick_win {} {debug write memory [expr {$::S(mis)+7}] 1;debug write memory [expr {$::S(mis)+8}] 3;debug write memory [expr {$::S(mis)+9}] 0}
proc wait_game {} {delay 1;set t0 [machine_info time];while {[w game_seconds]<1 && [machine_info time]-$t0<40} {delay 0.25}}
proc order_far {id x y} {
 set a [expr {$::S(units)+16*$id}]
 debug write memory [expr {$a+6}] $x;debug write memory [expr {$a+7}] $y        ;# tx,ty
 debug write memory [expr {$::S(goal_x)+$id}] $x;debug write memory [expr {$::S(goal_y)+$id}] $y
 debug write memory [expr {$::S(nav_partial)+$id}] 0;debug write memory [expr {$::S(retry_left)+$id}] 8
 debug write memory [expr {$a+15}] 1  ;# state = PLANNING
 debug write memory [expr {$::S(requested)+$id}] 1
}
proc bm_case {} {
 delay 38
 tapkey 7 128;tapkey 7 128;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 quick_win;await {[b game_over]==1} 15
 delay 4;tapkey 7 128;delay 6;tapkey 7 128;wait_game
 ck map64 {[b map_w]==64 && [b map_h]==64 && [b mission_n]==1}
 puts stderr "INFO base=[b pb_x],[b pb_y] enemy=[b eb_x],[b eb_y] route=[b ai_route_n] cam=[w cam_x],[w cam_y] window=[b wv_x0],[b wv_y0]-[b wv_x1],[b wv_y1]"
 ck camera_at_base {[w cam_y]>=([b pb_y]-8)*16-16 && [w cam_x]<=([b pb_x]*16)}
 shot bigmap-start
 # rolagem: seta para a direita por alguns segundos
 keymatrixdown 8 128;delay 4;keymatrixup 8 128;delay 1
 ck camera_scrolled {[w cam_x]>300}
 ck window_follows_camera {[b wv_x0]<=[expr {[w cam_x]>>4}] && [b wv_x1]>=[expr {([w cam_x]+255)>>4}]}
 shot bigmap-scrolled
 # caminho longo de um soldado (rotas parciais): ~28 celulas a leste e 20 ao norte
 set gx [expr {[b pb_x]+28}];set gy [expr {[b pb_y]-20}]
 order_far 2 $gx $gy
 set t0 [machine_info time]
 while {([unit 2 x]!=$gx || [unit 2 y]!=$gy) && [machine_info time]-$t0<150} {delay 1}
 puts stderr "INFO soldier at [unit 2 x],[unit 2 y] goal $gx,$gy after [expr {[machine_info time]-$t0}]s state=[unit 2 state]"
 ck long_path_reached {[unit 2 x]==$gx && [unit 2 y]==$gy}
 ck explored_far {[fog_state $gx $gy]==2}
 # economia em mapa grande: a mina e uma arvore perto do castelo
 set mx [expr {[b pb_x]+7}];set my [expr {[b pb_y]+1}]
 ck mine_in_layer {[vrd [expr {0x66000+($my<<7)+$mx}]]==4}
 set g0 [w gold]
 debug write memory $::S(node_x) $mx;debug write memory $::S(node_y) $my;debug write memory $::S(worker_kind) 2;debug write memory $::S(worker_task) 4
 await {[w gold]>$g0} 150
 ck mining_on_big_map {[w gold]>$g0}
 set tx -1;set ty -1;set bd 9999
 for {set y [expr {[b pb_y]-12}]} {$y<[expr {[b pb_y]+8}]} {incr y} {for {set x [expr {[b pb_x]-4}]} {$x<[expr {[b pb_x]+14}]} {incr x} {if {[vrd [expr {0x66000+($y<<7)+$x}]]==1 && [vrd [expr {0x66000+($y<<7)+$x+1}]]==0} {set d [expr {abs($x-[b pb_x])+abs($y-[b pb_y])}];if {$d<$bd} {set bd $d;set tx $x;set ty $y}}}}
 puts stderr "INFO tree at $tx,$ty"
 ck found_tree {$tx>=0}
 set w0 [w wood]
 debug write memory [expr {$::S(node_x)+1}] $tx;debug write memory [expr {$::S(node_y)+1}] $ty;debug write memory [expr {$::S(worker_kind)+1}] 1;debug write memory [expr {$::S(worker_task)+1}] 4
 await {[w wood]>$w0} 150
 ck wood_on_big_map {[w wood]>$w0}
 puts stderr "RESULT failures=$::failures";exit
}
coroutine bm_run guard bm_case
after realtime 700 {puts stderr "FAIL bigmap watchdog";exit}
