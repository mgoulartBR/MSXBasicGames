# uso: mkshots.py <projeto> <saida.tcl> : roteiro determinístico, despeja a página visível (VRAM) em instantes fixos
import sys,re
from pathlib import Path
r=Path(sys.argv[1]);out=Path(sys.argv[2])
s=(r/'build/keydoor-test.tcl').read_text().split('after time 10 {')[0]
d={n:int(a,16)for a,n in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+',(r/'build/keen_msx.map').read_text(),re.M)}
s+='\n'+'\n'.join(f'set {n} {d[n]}' for n in ['render_count','active_level','scene_ready','world_active','front_page'])+'\n'
s+=r'''
set ::od $::env(SHOT_DIR)
proc press {} {if {[byte $::world_active]==1} {return};keymatrixdown 8 1;after time 0.3 {keymatrixup 8 1};after time 1.5 press}
after time 6 press
proc dump {name} {
 # espera o desenho terminar; despeja as duas páginas de tela + atlas (128 KiB)
 if {[byte $::render_commit]} {after time 0.001 [list dump $name];return}
 set f [open $::od/$name.vram wb];puts -nonewline $f [debug read_block VRAM 0 131072];close $f
 screenshot $::od/$name.png
 set f [open $::od/$name.txt w];puts $f "front=[byte $::front_page] render=[word $::render_count] lvl=[byte $::active_level] pal=[join [list] ]";close $f
}
proc go_level {} {
 if {[byte $::world_active]!=1} {after time 0.2 go_level;return}
 after time 1 {
  putword $::world_x [lindex $::env(SHOT_WORLD) 0];putword $::world_y [lindex $::env(SHOT_WORLD) 1];keymatrixdown 8 1
  after time 0.3 {keymatrixup 8 1;waitfor {[byte $::active_level]==$::env(SHOT_LEVEL) && [byte $::scene_ready]==165} lvl_ready}
 }
}
proc lvl_ready {} {after time [expr {[info exists ::env(SHOT_LOADWAIT)]?$::env(SHOT_LOADWAIT):2}] {step 0}}
# cenas: (x,y,controles,duração)  — Keen parado em posições fixas, depois andando/pulando
set ::scenes $::env(SHOT_SCENES)
proc step {i} {
 if {$i>=[llength $::scenes]} {exit}
 lassign [lindex $::scenes $i] x y ctl wait
 if {$x>=0} {place $x $y}
 controls $ctl
 after time $wait [list snapdump $i]
}
proc snapdump {i} {
 controls 0
 after time 1 [list apply {{i} {dump s$i;step [expr {$i+1}]}} $i]
}
go_level
'''
out.write_text(s)
