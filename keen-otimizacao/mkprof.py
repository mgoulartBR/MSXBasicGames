# uso: mkprof.py <projeto> <saida.tcl> : perfil inclusivo por função durante uma caminhada, com VDP ocupado
import sys,re
from pathlib import Path
r=Path(sys.argv[1]);out=Path(sys.argv[2])
s=(r/'build/keydoor-test.tcl').read_text().split('after time 10 {')[0]
d={n:int(a,16)for a,n in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+',(r/'build/keen_msx.map').read_text(),re.M)}
import glob,os
def local_syms():
    out=r/'src/out';gabs={}
    for a,n,mod in re.findall(r'^\s+([0-9A-F]{8})\s+(_\w+)\s+(\S+)\s*$',(out/'keen_msx.map').read_text(),re.M):gabs[(mod,n)]=int(a,16)
    res={}
    for f in glob.glob(str(out/'*.sym')):
        mod=os.path.basename(f)[:-4]
        if os.environ.get('BENCH_MOD') and mod!=os.environ['BENCH_MOD']:continue
        ent=[(int(m[1]),m[2],int(m[3],16)) for m in re.finditer(r'^\s+(\d+) (_\w+)\s+([0-9A-F]{8}) [A-Z]*R',open(f).read(),re.M)]
        base=None
        for ar,n,rel in ent:
            if (mod,n) in gabs and not re.search(r'_\d{4,}_\d+$',n):base=gabs[(mod,n)]-rel;break
        if base is None:continue
        for ar,n,rel in ent:
            if not re.search(r'_\d{4,}_\d+$',n):res.setdefault(n[1:],base+rel)
    return res
_loc=local_syms()
for _k,_v in _loc.items():
    if os.environ.get('BENCH_MOD'):d[_k]=_v
    else:d.setdefault(_k,_v)
import os
FUNCS=os.environ.get('BENCH_FUNCS','DrawScene,ActorsPrepare,RingScene,ActorsDraw,ActorDraw,HudDraw,Game_Tick,CombatTick,FollowCamera,ReadInput').split(',')
s+='\n'+'\n'.join(f'set {n} {d[n]}' for n in ['render_count','world_map','boobus_bombs','actors_old','front_page','active_level','scene_ready','world_active','g_CurrentSegment','enemy_mode','player_x','player_y','simulation_ticks','stable_x','stable_y','stable_ticks','defeats','game_lives','camera_px','camera_py','ring_camera_x','ring_camera_y','g_VDP_REGSAV','flip_flag','flip_r2','flip_r5','flip_r6','flip_r8','flip_r11'] if n in d)+'\n'
spec=' '.join('%s %d %d'%(f,d[f]&0xFFFF,(d[f]>>16)&0xFF) for f in FUNCS if f in d)
s+='set ::funcs {%s}\n'%spec
s+=r'''
set ::bench_out $::env(BENCH_OUT)
if {[info exists ::env(BENCH_Z80)]} {set z80_freq_locked false;set z80_freq $::env(BENCH_Z80)}
proc logline {t} {set f [open $::bench_out a];puts $f $t;close $f}
set ::cur_seg_addr $::g_CurrentSegment
foreach {nm addr seg} $::funcs {
 set ::n_$nm 0;set ::t_$nm 0.0;set ::t0_$nm 0;set ::bp_$nm {}
 debug set_bp $addr "\[debug read memory \$::cur_seg_addr\]==$seg" [list prof_enter $nm]
}
proc prof_enter {nm} {
 if {$nm eq "RingScene" && [info exists ::env(BENCH_TRACE)] && $::phase>=0 && [machine_info time]>$::env(BENCH_TRACE_T0) && [machine_info time]<$::env(BENCH_TRACE_T1)} {
  set pg [expr {1-[byte $::front_page]}];set o [open $::env(BENCH_OUT).trace a]
  puts $o "t=[format %.3f [machine_info time]] page=$pg cam=[word $::camera_px],[word $::camera_py] ringcam=[word $::ring_camera_x]/[word [expr {$::ring_camera_x+2}]] old14p0=[word [expr {$::actors_old+14*6}]] old14p1=[word [expr {$::actors_old+174+14*6}]] vis0=[byte [expr {$::actors_old+14*6+5}]] vis1=[byte [expr {$::actors_old+174+14*6+5}]]"
  close $o
 }
 set sp [reg SP];set ::t0_$nm [machine_info time]
 set ::bp_$nm [debug set_bp [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}] "\[reg SP\]==$sp+2" [list prof_exit $nm]]
}
proc prof_exit {nm} {
 upvar #0 bp_$nm bp t0_$nm t0 n_$nm n t_$nm t
 catch {debug remove_bp $bp}
 incr n;set t [expr {$t+[machine_info time]-$t0}]
}
set ::vbusy 0.0;set ::vt0 0;set ::vcmds 0
debug probe set_bp VDP.commandExecuting {} {
 if {[debug probe read VDP.commandExecuting]} {set ::vt0 [machine_info time];incr ::vcmds} else {set ::vbusy [expr {$::vbusy+[machine_info time]-$::vt0}]}
}
proc sweep {} {
 if {$::phase<0} {return}
 lassign $::env(BENCH_SWEEP) x0 y0 step period
 incr ::sk
 place [expr {$x0+($::sk*$step)%1200}] $y0
 after time $period sweep
}
set ::sk 0
proc walk {} {if {$::phase<0} {return};set ::dir [expr {1-$::dir}];controls [expr {$::dir?1:2}];after time $::env(BENCH_LEG) walk}
set ::dir 0;set ::phase -1
proc begin {} {
 set ::phase 0;set ::start [machine_info time];set ::rc0 [word $::render_count];set ::vb0 $::vbusy;set ::vc0 $::vcmds
 foreach {nm a s} $::funcs {set ::s_n_$nm [set ::n_$nm];set ::s_t_$nm [set ::t_$nm]}
 if {[info exists ::env(BENCH_PLACE)]} {place {*}$::env(BENCH_PLACE)}
 if {[info exists ::env(BENCH_ENEMY)]} {debug write memory $::enemy_mode $::env(BENCH_ENEMY)}
 if {[info exists ::env(BENCH_SWEEP)]} {sweep} elseif {![info exists ::env(BENCH_IDLE)]} {walk}
 if {[info exists ::env(BENCH_PC)]} {sample}
 if {[info exists ::env(BENCH_SHOT)]} {foreach t [expr {[info exists ::env(BENCH_SHOTT)]?$::env(BENCH_SHOTT):{3 6 9 12 15}}] {after time $t [list shotlog $t [string map [list .png _$t.png] $::env(BENCH_SHOT)]]}}
 if {[info exists ::env(REGOUT)]} {after time 1 regtest} else {after time $::env(BENCH_SECS) finish}
}
set ::pcs {}
proc sample {} {
 if {$::phase<0} {return}
 set pc [reg PC];set seg [debug read memory $::cur_seg_addr]
 lappend ::pcs "$pc,$seg,[expr {[debug read memory [reg SP]]+256*[debug read memory [expr {[reg SP]+1}]]}]"
 after time 0.0007 sample
}
proc stripes {page base} {
 set row ""
 for {set i 0} {$i<128} {incr i} {set c [expr {$base+(($i/16)%7)}];append row [binary format c [expr {($c<<4)|$c}]]}
 set all ""
 for {set y 0} {$y<256} {incr y} {append all $row}
 debug write_block VRAM [expr {$page*32768}] $all
}
set ::rcfg {{a 1 0 31} {b 1 32 31} {c 1 0 63} {d 1 32 63} {e 3 0 31} {f 3 32 31} {g 3 0 63} {h 3 32 63}}
set ::ri 0
proc regtest {} {
 if {$::ri>=[llength $::rcfg]} {exit}
 lassign [lindex $::rcfg $::ri] tag r25 r26 r2extra
 stripes 0 1;stripes 1 9
 debug write {VDP regs} 2 $r2extra
 debug write {VDP regs} 25 $r25
 debug write {VDP regs} 26 $r26
 debug write {VDP regs} 27 0
 debug write {VDP regs} 23 0
 after time 0.6 [list screenshot $::env(REGOUT)/cfg_$tag.png]
 incr ::ri
 after time 1.2 regtest
}
proc finish {} {
 set f [open $::env(BENCH_OUT).pc w];puts $f [join $::pcs \n];close $f
 controls 0
 set dt [expr {[machine_info time]-$::start}];set n [expr {[word $::render_count]-$::rc0}]
 set sv {};for {set i 0} {$i<28} {incr i} {lappend sv [byte [expr {$::g_VDP_REGSAV+$i}]]}
 logline "SHADOW=$sv"
 if {[info exists ::flip_flag]} {logline "FLIP flag=[byte $::flip_flag] r2=[byte $::flip_r2] r5=[byte $::flip_r5] r6=[byte $::flip_r6] r8=[byte $::flip_r8] r11=[byte $::flip_r11] addrs=$::flip_flag,$::flip_r2,$::flip_r5,$::flip_r6,$::flip_r8,$::flip_r11"}
 logline "R2=[debug read {VDP regs} 2] R23=[debug read {VDP regs} 23] R25=[debug read {VDP regs} 25] R26=[debug read {VDP regs} 26] R27=[debug read {VDP regs} 27] R1=[debug read {VDP regs} 1] R8=[debug read {VDP regs} 8] R5=[debug read {VDP regs} 5] R6=[debug read {VDP regs} 6] R11=[debug read {VDP regs} 11]"
 logline "cam=([word $::camera_px],[word $::camera_py]) ex_l=[word $::ring_camera_x] ex_r=[word [expr {$::ring_camera_x+2}]] ey_t=[word $::ring_camera_y] ey_b=[word [expr {$::ring_camera_y+2}]]"
 logline "defeats=[word $::defeats] lives=[byte $::game_lives]"
 logline "player: x=[word $::stable_x] y=[word $::stable_y] at_tick=[word $::stable_ticks]"
 logline "window=[format %.1f $dt]s renders=$n fps=[format %.2f [expr {$n/$dt}]] vdp_busy=[format %.0f [expr {100*($::vbusy-$::vb0)/$dt}]]% vdp_cmds=[expr {$::vcmds-$::vc0}]"
 foreach {nm a s} $::funcs {
  set c [expr {[set ::n_$nm]-[set ::s_n_$nm]}];set t [expr {[set ::t_$nm]-[set ::s_t_$nm]}]
  logline [format "  %-14s calls=%-5d total=%6.0f ms (%4.1f%% of window)  mean=%6.2f ms" $nm $c [expr {1000*$t}] [expr {100*$t/$dt}] [expr {$c?1000*$t/$c:0}]]
 }
 exit
}
proc go_level {} {
 if {[byte $::world_active]!=1} {after time 0.2 go_level;return}
 after time 1 {
  if {$::env(BENCH_LEVEL)==11 && [info exists ::boobus_bombs]} {putword $::boobus_bombs 12};putword $::world_x [lindex $::env(BENCH_WORLD) 0];putword $::world_y [lindex $::env(BENCH_WORLD) 1];keymatrixdown 8 1
  after time 0.3 {keymatrixup 8 1;poll_level}
 }
}
proc shotlog {t f} {screenshot $f;if {[info exists ::env(BENCH_VRAM)]} {set v [open [string map {.png .vram} $f] wb];puts -nonewline $v [debug read_block VRAM 0 131072];close $v;if {[info exists ::world_map]} {set w [open [string map {.png .wmap} $f] wb];puts -nonewline $w [debug read_block memory $::world_map 9835];close $w};set q [open [string map {.png .rec} $f] w];foreach p {0 1} {set a [expr {$::actors_old+$p*174+14*6}];puts $q "page$p hud x=[word $a] y=[word [expr {$a+2}]] frame=[byte [expr {$a+4}]] vis=[byte [expr {$a+5}]] ring_cam=[word [expr {$::ring_camera_x+2*$p}]]/[word [expr {$::ring_camera_x+4+2*$p}]]"};puts $q "front=[byte $::front_page] cam=[word $::camera_px],[word $::camera_py]";close $q};if {[info exists ::camera_px]} {set l [open $::env(BENCH_OUT).cam a];puts $l "$t [word $::camera_px] [word $::camera_py]";close $l}}
proc poll_level {} {if {[byte $::active_level]==$::env(BENCH_LEVEL) && [byte $::scene_ready]==165 && ![byte $::world_active]} {lvl_ready} else {after time 0.5 poll_level}}
proc lvl_ready {} {after time $::env(BENCH_LOADWAIT) begin}
proc press {} {if {[byte $::world_active]==1} {return};keymatrixdown 8 1;after time 0.3 {keymatrixup 8 1};after time 1.5 press}
after time 6 press
go_level
'''
out.write_text(s)
