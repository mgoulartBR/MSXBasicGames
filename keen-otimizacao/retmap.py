# uso: retmap.py <projeto> <saida.tcl> : entra em Hill, completa a fase (borda direita) e volta ao mapa
import sys,re
from pathlib import Path
r=Path(sys.argv[1])
s=(r/'build/game-test.tcl').read_text().split('after time 10 {')[0]
syms={n:int(a,16)for a,n in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+',(r/'build/keen_msx.map').read_text(),re.M)}
s+="\nif {[info exists ::env(BENCH_Z80)]} {set z80_freq_locked false;set z80_freq $::env(BENCH_Z80)}\n"
for n in ['player_x','player_y','player_vx','player_vy','bar_on','bar_ok','level_done','world_active','world_completed','hardware_scroll']:s+=f'\nset raw_{n} {syms[n]}\n'
s+=r'''
proc putword {a n} {debug write memory $a [expr {$n&255}];debug write memory [expr {$a+1}] [expr {($n>>8)&255}]}
proc rep {tag} {
 binary scan [debug read_block "VDP regs" 0 32] cu* rg
 set o [open $::env(OUT) a];puts $o "$tag px=[word $::raw_player_x] py=[word $::raw_player_y] bar_on=[byte $::raw_bar_on] bar_ok=[byte $::raw_bar_ok] level_done=[byte $::raw_level_done] world_active=[byte $::raw_world_active] completed=[byte $::raw_world_completed] R0=[lindex $rg 0] R2=[lindex $rg 2] R8=[lindex $rg 8] R19=[lindex $rg 19] R23=[lindex $rg 23] R26=[lindex $rg 26] R27=[lindex $rg 27]";close $o
}
proc shot {tag} {screenshot -raw $::env(OUT).$tag.png}
proc waitfor {cond then} {if {[expr $cond]} {eval $then} else {after time 0.5 [list waitfor $cond $then]}}
proc t0 {} {
 rep boot
 keymatrixdown 8 16
 after time 0.6 {keymatrixup 8 16}
 after time 1 {keymatrixdown 8 1}
 after time 1.3 {keymatrixup 8 1;after time 1 {waitfor {![byte $::raw_world_active] && [byte $::scene_ready]==165} t1}}
}
proc t1 {} {
 after time 4 {
  rep in_level;shot level
  putword $::raw_player_x 700;putword $::raw_player_y [expr {480*16}]
  putword $::raw_player_vx 0;putword $::raw_player_vy 0
  keymatrixdown 8 16
  after time 1.5 {rep after_right;waitfor {[byte $::raw_level_done]==2} t2}
 }
}
proc t2 {} {
 keymatrixup 8 16;rep level_done;shot done
 keymatrixdown 8 1
 after time 0.3 {keymatrixup 8 1;waitfor {[byte $::raw_world_active]==1} t3}
}
proc t3 {} {
 after time 3 {rep back_on_map;shot map1;keymatrixdown 8 16;after time 0.5 {keymatrixup 8 16;after time 2 {rep after_walk;shot map2;exit}}}
}
proc press {} {if {[byte $::raw_world_active]==1} {after time 2 t0;return};keymatrixdown 8 1;after time 0.3 {keymatrixup 8 1};after time 1.5 press}
after time 6 press
'''
Path(sys.argv[2]).write_text(s)
