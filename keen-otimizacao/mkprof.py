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
s+='\n'+'\n'.join(f'set {n} {d[n]}' for n in ['ca_x','ca_map','ca_tile','ca_w','ca_h','ca_wx','ca_wy','key_owned','combat_dirty','game_lives','restart_pending','render_count','bar_on','bar_ph','cur_r2','cur_r23','cur_r26','cur_r27','vdp_fast','player_x','player_y','hero_frame','world_map','boobus_bombs','actors_old','front_page','active_level','scene_ready','world_active','g_CurrentSegment','enemy_mode','player_x','player_y','simulation_ticks','stable_x','stable_y','stable_ticks','defeats','game_lives','camera_px','camera_py','ring_camera_x','ring_camera_y','g_VDP_REGSAV','flip_flag','flip_r2','flip_r5','flip_r6','flip_r8','flip_r11'] if n in d)+'\n'
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
 if {[debug probe read VDP.commandExecuting]} {set ::vt0 [machine_info time];incr ::vcmds;if {[info exists ::env(BENCH_VDPLOG)] && [machine_info time]>$::env(BENCH_TRACE_T0) && [machine_info time]<$::env(BENCH_TRACE_T1)} {set r [debug read_block "VDP regs" 32 15];set o [open $::env(BENCH_OUT).vdp a];binary scan $r cu* bl;puts $o "[format %.4f [machine_info time]] [join $bl { }]";close $o}} else {set ::vbusy [expr {$::vbusy+[machine_info time]-$::vt0}]}
}
proc sweep {} {
 if {$::phase<0} {return}
 lassign $::env(BENCH_SWEEP) x0 y0 step period
 incr ::sk
 place [expr {$x0+($::sk*$step)%1200}] $y0
 after time $period sweep
}
set ::sk 0
set ::jp 0
proc walk {} {if {$::phase<0} {return};set ::dir [expr {1-$::dir}];controls [expr {($::dir?1:2)|($::jp?16:0)}];after time $::env(BENCH_LEG) walk}
proc jumper {} {if {$::phase<0} {return};set ::jp [expr {1-$::jp}];controls [expr {($::dir?1:2)|($::jp?16:0)}];after time $::env(BENCH_JUMPT) jumper}
set ::dir 0;set ::phase -1
proc begin {} {
 set ::phase 0;set ::start [machine_info time];set ::rc0 [word $::render_count];set ::vb0 $::vbusy;set ::vc0 $::vcmds
 foreach {nm a s} $::funcs {set ::s_n_$nm [set ::n_$nm];set ::s_t_$nm [set ::t_$nm]}
 if {[info exists ::env(BENCH_PLACE)]} {place {*}$::env(BENCH_PLACE)}
 if {[info exists ::env(BENCH_ENEMY)]} {debug write memory $::enemy_mode $::env(BENCH_ENEMY)}
 if {[info exists ::env(BENCH_SWEEP)]} {sweep} elseif {![info exists ::env(BENCH_IDLE)]} {walk;if {[info exists ::env(BENCH_JUMPT)]} {jumper}}
 if {[info exists ::env(BENCH_GOD)]} {debug set_watchpoint write_mem $::death_ticks {} {if {[debug read memory $::death_ticks]!=0} {debug write memory $::death_ticks 0;incr ::god_hits}}}
 if {[info exists ::env(BENCH_GOD)] && $::env(BENCH_GOD)==2} {debug set_watchpoint write_mem $::restart_pending {} {if {[debug read memory $::restart_pending]!=0} {debug write memory $::restart_pending 0;debug write memory $::game_lives 3;incr ::god_hits}}}
 if {[info exists ::env(BENCH_TRACK)]} {track}
 if {[info exists ::env(BENCH_CKPT)]} {set ::ck_list $::env(BENCH_CKPT);set ::ck_k 0;set ::ck_log {};ckpt}
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
proc bcall_hit {} {
 set ra [expr {[debug read memory [reg SP]]+256*[debug read memory [expr {[reg SP]+1}]]}]
 if {[info exists ::env(BENCH_RA)]} {set k [format "ret%04X:%d" $ra [debug read memory 0xC004]]} else {set k [format "%04X:%d:ret%04X" [reg HL] [reg E] $ra]}
 if {[info exists ::bcn($k)]} {incr ::bcn($k)} else {set ::bcn($k) 1}
}
proc cd_io {} {set pc [reg PC];if {$pc>=$::env(BENCH_ISRLO) && $pc<=$::env(BENCH_ISRHI)} {return};if {$::cdph ne ""} {lappend ::cdlog($::cdn,$::cdph) "[expr {$::wp_last_value}]"}}
proc cd_enter {ph} {if {[debug read memory 0xC004]!=114} {return};if {$ph eq "C"} {incr ::cdn};set ::cdph $ph}
proc cd_exit {} {if {[debug read memory 0xC004]!=114} {return};if {[info exists ::env(BENCH_COLEXITALWAYS)] || ([reg F]&1)==0} {set ::cdph ""}}
proc w16 {a} {return [expr {[debug read memory $a]+256*[debug read memory [expr {$a+1}]]}]}
proc hs_snap {} {
 set ::hs_s {}
 foreach base [list {*}[lmap i {0 1 2 3 4 5 6 7 8 9 10} {expr {$::world_map+9600+16*$i}}] $::hs_enemy] {lappend ::hs_s [list $base [w16 $base] [w16 [expr {$base+2}]] [debug read memory [expr {$base+10}]]]}
 set ::hs_cx [w16 $::camera_px];set ::hs_cy [w16 $::camera_py]
}
proc hs_check {} {
 set lx [expr {(($::hs_cx-96)<<4)&0xFFFF}];set ly [expr {(($::hs_cy-96)<<4)&0xFFFF}]
 foreach r $::hs_s {
  lassign $r base x y act
  set dx [expr {($x-$lx)&0xFFFF}];set dy [expr {($y-$ly)&0xFFFF}]
  if {$act} {if {$dx>7183 || $dy>6159} {set act 0}} else {if {(($dx-768)&0xFFFF)<=5647 && (($dy-768)&0xFFFF)<=4623} {set act 1}}
  incr ::hs_n;if {$act!=[lindex $r 3]} {incr ::hs_tr}
  if {[debug read memory [expr {$base+10}]]!=$act} {incr ::hs_bad}
 }
}
proc hs_enter {} {
 if {[debug read memory 0xC004]!=15} {return}
 hs_snap
 set ra [w16 [reg SP]]
 set ::hs_bp [debug set_bp $ra {} {hs_check;catch {debug remove_bp $::hs_bp}}]
}
proc s16 {v} {return [expr {$v>=32768?$v-65536:$v}]}
proc hc_enter {} {
 if {[debug read memory 0xC004]!=15} {return}
 set ::hc_out [reg HL]
 set ::hc_s {}
 foreach i {0 1 2 3 4 5 6 7 8 9 10} {set b [expr {$::world_map+9600+16*$i}];lappend ::hc_s [list $i [w16 $b] [w16 [expr {$b+2}]] [debug read memory [expr {$b+10}]] [debug read memory [expr {$b+11}]] [debug read memory [expr {$b+13}]]]}
 set ::hc_cx [w16 $::camera_px];set ::hc_cy [w16 $::camera_py];set ::hc_em [debug read memory $::enemy_mode]
 set ra [w16 [reg SP]]
 set ::hc_bp [debug set_bp $ra {} {hc_check;catch {debug remove_bp $::hc_bp}}]
}
proc hc_check {} {
 foreach r $::hc_s {
  lassign $r i x y act fr st
  set o [expr {$::hc_out+6*$i}];incr ::hc_n
  set vis 0;set ex 0;set ey 0
  if {$act && !($::hc_em&2)} {
   set dx [expr {($x>>4)-$::hc_cx}];set dy [expr {($y>>4)-$::hc_cy}]
   if {!($dx<-64 || $dx>=280 || $dy<-32 || $dy>=192)} {
    set vis 1;set off [expr {$i>=4 && $st!=1}]
    set ex [expr {(($x>>4)-($off?24:0))&0xFFFF}];set ey [expr {(($y>>4)-($off?4:0))&0xFFFF}]
   }
  }
  set got [debug read memory [expr {$o+5}]]
  if {$got!=$vis} {incr ::hc_bad;continue}
  if {$vis} {incr ::hc_vis;if {[w16 $o]!=$ex || [w16 [expr {$o+2}]]!=$ey || [debug read memory [expr {$o+4}]]!=$fr} {incr ::hc_bad}}
 }
}
proc gv_actors {} {set l {};for {set i 0} {$i<$::gv_cnt} {incr i} {set b [expr {$::world_map+$::gv_base+16*$i}];lappend l [list $b [w16 $b] [w16 [expr {$b+2}]] [debug read memory [expr {$b+10}]] [debug read memory [expr {$b+13}]]]};return $l}
proc gv_ret {script} {set ra [w16 [reg SP]];set ::gv_bp [debug set_bp $ra {} "$script;catch {debug remove_bp \$::gv_bp}"]}
proc gvs_enter {} {if {[debug read memory 0xC004]!=$::gv_bs} {return};set ::gvs_s [gv_actors];set ::gvs_cx [w16 $::camera_px];set ::gvs_cy [w16 $::camera_py];gv_ret gvs_check}
proc gvs_check {} {
 foreach r $::gvs_s {lassign $r b x y act st
  set dx [expr {(($x>>4)-$::gvs_cx)&0xFFFF}];set dy [expr {(($y>>4)-$::gvs_cy)&0xFFFF}]
  if {$st==1} {set act 1} elseif {$act} {if {(($dx+96)&0xFFFF)>448 || (($dy+96)&0xFFFF)>384} {set act 0}} else {if {(($dx+48)&0xFFFF)<=352 && (($dy+48)&0xFFFF)<=288} {set act 1}}
  incr ::gvs_n;if {$act!=[lindex $r 3]} {incr ::gvs_tr}
  if {[debug read memory [expr {$b+10}]]!=$act} {incr ::gvs_bad}}
}
proc gvn_enter {} {if {[debug read memory 0xC004]!=$::gv_bn} {return};set ::gvn_i [reg A];set ::gvn_s [gv_actors];gv_ret gvn_check}
proc gvn_check {} {
 set e $::gv_cnt;for {set k $::gvn_i} {$k<$::gv_cnt} {incr k} {if {[lindex [lindex $::gvn_s $k] 3]} {set e $k;break}}
 incr ::gvn_n;if {[reg A]!=$e} {incr ::gvn_bad}
}
proc gvb_enter {} {
 set ::gvb_out [reg HL];set tab [w16 $::gv_cax];set ::gvb_n34 [w16 $::gv_car]
 set tp [w16 $::gv_cwy];set ::gvb_stride [debug read memory $::gv_cwx];set ::gvb_taken [expr {[w16 $tp]+65536*[w16 [expr {$tp+2}]]}];set ::gvb_hi [debug read memory [expr {$::world_collision+1151}]]
 set ::gvb_cx [w16 $::camera_px];set ::gvb_cy [w16 $::camera_py];set ::gvb_tab $tab
 gv_ret gvb_check
}
proc gvb_check {} {
 set exp {}
 for {set i 0} {$i<$::gvb_n34} {incr i} {
  set tk [expr {$i<32 ? ($::gvb_taken>>$i)&1 : ($::gvb_hi>>($i-32))&1}]
  if {$tk} {continue}
  set a [expr {$::gvb_tab+$::gvb_stride*$i}];set x [w16 $a];set y [w16 [expr {$a+2}]]
  set dx [expr {($x-$::gvb_cx)&0xFFFF}];set dy [expr {($y-$::gvb_cy)&0xFFFF}]
  if {(($dx+382)&0xFFFF)<638 && (($dy+382)&0xFFFF)<574} {lappend exp $i}
 }
 incr ::gvb_n;incr ::gvb_items [llength $exp]
 set got {};for {set k 0} {$k<[reg A]} {incr k} {lappend got [debug read memory [expr {$::gvb_out+$k}]]}
 if {$got ne $exp} {incr ::gvb_bad}
}
proc m40_enter {} {set ::m40_t [reg HL];set ra [w16 [reg SP]];set ::m40_bp [debug set_bp $ra {} {incr ::m40_n;if {[reg A]!=($::m40_t%40)} {incr ::m40_bad};catch {debug remove_bp $::m40_bp}}]}
set ::god_hits 0
proc track {} {
 if {$::phase<0} {return}
 set o [open $::env(BENCH_OUT).trk a];puts $o "[expr {[word $::raw_player_x]/16}] [expr {[word $::raw_player_y]/16}]";close $o
 after time $::env(BENCH_TRACK) track
}
proc ckpt {} {
 if {$::phase<0} {return}
 set n [expr {[llength $::ck_list]/2}]
 set rc [word $::render_count];set t [machine_info time]
 if {$::ck_k>0} {lappend ::ck_log [list [expr {$::ck_k-1}] [expr {$rc-$::ck_rc}] [expr {$t-$::ck_t}] [expr {$rc-$::ck_rc2}] [expr {$t-$::ck_t2}]]}
 if {$::ck_k>=$n} {
  set o [open $::env(BENCH_OUT).ck w]
  foreach r $::ck_log {puts $o "seg [lindex $r 0] renders [lindex $r 1] secs [format %.2f [lindex $r 2]] fps [format %.2f [expr {[lindex $r 1]/[lindex $r 2]}]] steady_renders [lindex $r 3] steady_secs [format %.2f [lindex $r 4]]"}
  close $o;return}
 set ::ck_rc $rc;set ::ck_t $t;set ::ck_rc2 $rc;set ::ck_t2 $t
 after time 1.5 {set ::ck_rc2 [word $::render_count];set ::ck_t2 [machine_info time]}
 place [lindex $::ck_list [expr {2*$::ck_k}]] [lindex $::ck_list [expr {2*$::ck_k+1}]]
 incr ::ck_k
 after time $::env(BENCH_SEG) ckpt
}
proc an_enter {} {
 if {[debug read memory 0xC004]!=$::an_bank} {return}
 set i [reg A];set base [w16 $::an_cax];set cnt [w16 $::an_car]
 set act {};for {set k 0} {$k<$cnt} {incr k} {lappend act [debug read memory [expr {$base+16*$k+10}]]}
 set e $cnt;for {set k $i} {$k<$cnt} {incr k} {if {[lindex $act $k]} {set e $k;break}}
 set ::an_e $e;set ra [w16 [reg SP]];set ::an_bp [debug set_bp $ra {} {incr ::an_n;if {[reg A]!=$::an_e} {incr ::an_bad};catch {debug remove_bp $::an_bp}}]
}
proc ot_enter {} {
 if {[debug read memory 0xC004]!=$::ot_bank} {return}
 set x [reg HL];set y [reg DE];set w [w16 $::ot_cwx];set h [w16 $::ot_cwy]
 set px [w16 $::raw_player_x];set py [w16 $::raw_player_y]
 set d1 [s16 [expr {($px+64-$x)&0xFFFF}]];set d2 [s16 [expr {($x-$px-64)&0xFFFF}]];set e1 [s16 [expr {($py+48-$y)&0xFFFF}]];set e2 [s16 [expr {($y-$py-48)&0xFFFF}]]
 set ::ot_e [expr {$d1<$w && $d2<256 && $e1<$h && $e2<448}]
 set ra [w16 [reg SP]];set ::ot_bp [debug set_bp $ra {} {incr ::ot_n;if {[reg A]!=$::ot_e} {incr ::ot_bad};if {$::ot_e} {incr ::ot_true};catch {debug remove_bp $::ot_bp}}]
}
proc bt_enter {} {
 if {[debug read memory 0xC004]!=$::bt_bank} {return}
 set ::bt_old [debug read memory $::bt_bombs];set ::bt_rise {};for {set i 0} {$i<6} {incr i} {lappend ::bt_rise [debug read memory [expr {$::bt_rise_a+$i}]]}
 set ::bt_bb [w16 0xEF15];set ::bt_px [w16 $::raw_player_x];set ::bt_py [w16 $::raw_player_y]
 set ra [w16 [reg SP]];set ::bt_bp [debug set_bp $ra {} {bt_check;catch {debug remove_bp $::bt_bp}}]
}
proc bt_check {} {
 set exp $::bt_old;set nb 0;set er {}
 for {set i 0} {$i<6} {incr i} {
  set r [lindex $::bt_rise $i];if {$r>0} {incr r -1}
  if {!(($::bt_old>>$i)&1)} {
   lassign [lindex $::bt_sp $i] tx ty;set x [expr {$tx*256}];set y [expr {$ty*256}]
   set d [s16 [expr {($::bt_px+64-$x)&0xFFFF}]];set d2 [s16 [expr {($x-$::bt_px-64)&0xFFFF}]];set e [s16 [expr {($::bt_py+48-$y)&0xFFFF}]];set e2 [s16 [expr {($y-$::bt_py-48)&0xFFFF}]]
   if {$d<256 && $d2<256 && $e<256 && $e2<448} {set exp [expr {$exp|(1<<$i)}];incr nb;set r 40}
  }
  lappend er $r
 }
 incr ::bt_n;set got {};for {set i 0} {$i<6} {incr i} {lappend got [debug read memory [expr {$::bt_rise_a+$i}]]}
 if {[debug read memory $::bt_bombs]!=$exp || $got ne $er || [w16 0xEF15]!=(($::bt_bb+$nb)&0xFFFF)} {incr ::bt_bad;if {$::bt_bad<4} {set o [open $::env(BENCH_OUT).btd a];puts $o "old=$::bt_old exp=$exp gotb=[debug read memory $::bt_bombs] rise_old=$::bt_rise er=$er got=$got bb=$::bt_bb now=[w16 0xEF15] nb=$nb px=$::bt_px py=$::bt_py";close $o}}
 if {$nb} {incr ::bt_hits $nb}
}
proc finish {} {
 if {[info exists ::env(BENCH_VRAMDUMP)]} {set f [open $::env(BENCH_OUT).vram wb];puts -nonewline $f [debug read_block "VRAM" 0 131072];close $f}
 if {[info exists ::env(BENCH_OLORC)]} {set f [open $::env(BENCH_OUT).ol w];puts $f "calls=$::ol_n hits=$::ol_hit bad=$::ol_bad $::ol_info";close $f}
 if {[info exists ::env(BENCH_FRAMESTAT)]} {set f [open $::env(BENCH_OUT).fr w];foreach k [lsort -integer [array names ::fr_n]] {puts $f "$k $::fr_n($k)"};close $f}
 if {[info exists ::env(BENCH_CMDSTAT)]} {set f [open $::env(BENCH_OUT).cs w];foreach k [lsort [array names ::cs_n]] {puts $f "$k $::cs_n($k) $::cs_px($k) $::cs_t($k)"};foreach k [array names ::c2_n] {puts $f "C2 $k $::c2_n($k) $::c2_t($k)"};close $f}
 if {[info exists ::env(BENCH_BBORC)]} {set f [open $::env(BENCH_OUT).bb a];puts $f "END calls=$::bb_n";close $f}
 if {[info exists ::env(BENCH_MTORC)]} {set f [open $::env(BENCH_OUT).mt w];puts $f "calls=$::mt_n bad=$::mt_bad $::mt_info";close $f}
 if {[info exists ::env(BENCH_FLIPCHK)]} {set f [open $::env(BENCH_OUT).fl w];puts $f "flips=$::fl_n busy=$::fl_busy";close $f}
 if {[info exists ::env(BENCH_BTDIAG)]} {set f [open $::env(BENCH_OUT).bt w];puts $f "bombtick calls=$::bt_n pickups=$::bt_hits mismatches=$::bt_bad";close $f}
 if {[info exists ::env(BENCH_ANDIAG)] || [info exists ::env(BENCH_OTDIAG)]} {set f [open $::env(BENCH_OUT).ao w];puts $f "actornext calls=$::an_n mismatches=$::an_bad | objtouch calls=$::ot_n true=$::ot_true mismatches=$::ot_bad";close $f}
 if {[info exists ::env(BENCH_M40)]} {set f [open $::env(BENCH_OUT).m4 w];puts $f "mod40 calls=$::m40_n mismatches=$::m40_bad";close $f}
 if {[info exists ::env(BENCH_GVDIAG)]} {set f [open $::env(BENCH_OUT).gv w];puts $f "select: actor_checks=$::gvs_n state_changes=$::gvs_tr mismatches=$::gvs_bad | next: calls=$::gvn_n mismatches=$::gvn_bad | bonuslist: calls=$::gvb_n listed_items=$::gvb_items mismatches=$::gvb_bad";close $f}
 if {[info exists ::env(BENCH_HCDIAG)]} {set f [open $::env(BENCH_OUT).hc w];puts $f "records=$::hc_n visible=$::hc_vis mismatches=$::hc_bad";close $f}
 if {[info exists ::env(BENCH_HSDIAG)]} {set f [open $::env(BENCH_OUT).hs w];puts $f "actor_checks=$::hs_n state_changes=$::hs_tr mismatches=$::hs_bad";close $f}
 if {[info exists ::env(BENCH_COLDIAG)]} {set f [open $::env(BENCH_OUT).cd w];foreach k [lsort -dictionary [array names ::cdlog]] {puts $f "$k $::cdlog($k)"};close $f}
 if {[info exists ::env(BENCH_BC)]} {set f [open $::env(BENCH_OUT).bc w];foreach k [array names ::bcn] {puts $f "$k $::bcn($k)"};close $f}
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
 logline "FH ms-after-vblank: $::dl"
 if {$::ln>0} {logline [format "  LineISR calls=%d mean=%.3f ms max=%.3f ms total=%.0f ms fh_runs=%d"  $::ln [expr {1000*$::ldur/$::ln}] [expr {1000*$::lmax}] [expr {1000*$::ldur}] $::b2cnt]}
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
set ::lcnt 0;set ::kcnt 0
set ::w99 {};set ::w99on 0;set ::w99n 0
if {[info exists ::env(BENCH_W99)]} {debug set_watchpoint write_io 0x99 {} {incr ::w99n;if {$::w99on} {lappend ::w99 [list [machine_info time] $::wp_last_value]}};after time $::env(BENCH_W99) {set ::w99on 1;after time 0.034 {set ::w99on 0;set f [open $::env(BENCH_OUT).w99 w];puts $f [list $::vbt0 $::w99n $::w99];close $f}}}
set ::b2cnt 0;set ::vbt 0;set ::dl {}
if {[info exists ::env(BENCH_VB)]} {debug set_bp $::env(BENCH_VB) {} {set ::vbt [machine_info time];set ::vbt0 $::vbt}}
if {[info exists ::env(BENCH_BP2)]} {debug set_bp $::env(BENCH_BP2) {} {incr ::b2cnt;if {[llength $::dl]<60} {lappend ::dl [format %.2f [expr {1000*([machine_info time]-$::vbt)}]]}}}
set ::ldur 0.0;set ::lmax 0.0;set ::ln 0
proc lisr_enter {} {
 set sp [reg SP];set ::lt0 [machine_info time]
 set ::lbp [debug set_bp [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}] "\[reg SP\]==$sp+2" lisr_exit]
}
proc lisr_exit {} {
 catch {debug remove_bp $::lbp}
 set d [expr {[machine_info time]-$::lt0}];set ::ldur [expr {$::ldur+$d}];incr ::ln;if {$d>$::lmax} {set ::lmax $d}
}
if {[info exists ::env(BENCH_BC)]} {array set ::bcn {};debug set_bp $::env(BENCH_BC) {} {bcall_hit}}
if {[info exists ::env(BENCH_COLDIAG)]} {lassign $::env(BENCH_COLDIAG) aC aA aX;set ::cdn 0;set ::cdph "";debug set_watchpoint write_io 0x9B {} {cd_io};debug set_watchpoint write_io 0x99 {} {cd_io};debug set_bp $aC {} {cd_enter C};debug set_bp $aA {} {cd_enter A};debug set_bp $aX {} {cd_exit}}
if {[info exists ::env(BENCH_HSDIAG)]} {lassign $::env(BENCH_HSDIAG) hsa hse;set ::hs_enemy $hse;set ::hs_n 0;set ::hs_bad 0;set ::hs_tr 0;debug set_bp $hsa {} {hs_enter}}
if {[info exists ::env(BENCH_HCDIAG)]} {set ::hc_n 0;set ::hc_bad 0;set ::hc_vis 0;debug set_bp $::env(BENCH_HCDIAG) {} {hc_enter}}
if {[info exists ::env(BENCH_GVDIAG)]} {lassign $::env(BENCH_GVDIAG) gva gvn gvb bta cax car cwx cwy gvbase gvcnt gvbs gvbn;set ::gv_base [expr {$gvbase+0}];set ::gv_cnt $gvcnt;set ::gv_bs $gvbs;set ::gv_bn $gvbn;set ::gv_cwx $cwx;set ::gv_cwy $cwy;set ::gv_cax $cax;set ::gv_car $car;set ::bonus_taken_a $bta;foreach v {gvs_n gvs_tr gvs_bad gvn_n gvn_bad gvb_n gvb_items gvb_bad} {set ::$v 0};debug set_bp $gva {} {gvs_enter};debug set_bp $gvn {} {gvn_enter};debug set_bp $gvb {} {gvb_enter}}
if {[info exists ::env(BENCH_M40)]} {set ::m40_n 0;set ::m40_bad 0;debug set_bp $::env(BENCH_M40) {} {m40_enter}}
foreach v {an_n an_bad ot_n ot_bad ot_true} {set ::$v 0}
if {[info exists ::env(BENCH_ANDIAG)]} {lassign $::env(BENCH_ANDIAG) aaddr abank acax acar;set ::an_bank $abank;set ::an_cax $acax;set ::an_car $acar;debug set_bp $aaddr {} {an_enter}}
if {[info exists ::env(BENCH_OTDIAG)]} {lassign $::env(BENCH_OTDIAG) oaddr obank ocwx ocwy;set ::ot_bank $obank;set ::ot_cwx $ocwx;set ::ot_cwy $ocwy;debug set_bp $oaddr {} {ot_enter}}
if {[info exists ::env(BENCH_SNAP)]} {lassign $::env(BENCH_SNAP) sna snseg snoff snlen snmod
 proc sn_hit {} {set t [expr {[debug read memory $::simulation_ticks]+256*[debug read memory [expr {$::simulation_ticks+1}]]}]
  if {$t%$::sn_mod!=0 || [info exists ::sn_seen($t)]} {return};set ::sn_seen($t) 1
  binary scan [debug read_block "memory" [expr {$::world_map+$::sn_off}] $::sn_len] H* h;set f [open $::env(BENCH_OUT).sn a];puts $f "$t $h";close $f}
 set ::sn_off $snoff;set ::sn_len $snlen;set ::sn_mod $snmod
 debug set_bp $sna "\[debug read memory \$::g_CurrentSegment\]==$snseg" {sn_hit}}
if {[info exists ::env(BENCH_OLORC)]} {set ::ol_n 0;set ::ol_hit 0;set ::ol_bad 0;set ::ol_info {}
 proc ol_w {a} {expr {[debug read memory $a]+256*[debug read memory [expr {$a+1}]]}}
 proc ol_s16 {v} {set v [expr {$v&65535}];if {$v>=32768} {expr {$v-65536}} else {set v}}
 proc ol_enter {} {set sp [reg SP];set ::ol_ret [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}]
  set ::ol_sp [ol_w $::ca_x];set ::ol_rp [ol_w $::ca_map];set ::ol_tp [ol_w $::ca_tile];set ::ol_cnt [debug read memory $::ca_w];set ::ol_mode [debug read memory $::ca_h]
  set ::ol_wx [ol_w $::ca_wx];set ::ol_wy [ol_w $::ca_wy];set ::ol_px [ol_w $::player_x];set ::ol_py [ol_w $::player_y]
  set ::ol_rise {};set ::ol_spx {};set ::ol_spy {}
  for {set i 0} {$i<$::ol_cnt} {incr i} {lappend ::ol_rise [debug read memory [expr {$::ol_rp+$i}]];lappend ::ol_spx [debug read memory [expr {$::ol_sp+3*$i}]];lappend ::ol_spy [debug read memory [expr {$::ol_sp+3*$i+1}]]}
  set ::ol_mask [debug read memory $::ol_tp];set ::ol_bb [ol_w $::boobus_bombs];set ::ol_ko [debug read memory $::key_owned]
  if {[info exists ::ol_bp]} {catch {debug remove_bp $::ol_bp}};set ::ol_bp [debug set_bp $::ol_ret {} {ol_exit}]}
 proc ol_exit {} {if {![info exists ::ol_bp]} {return};catch {debug remove_bp $::ol_bp};unset ::ol_bp;incr ::ol_n
  set mask $::ol_mask;set bb $::ol_bb;set ko $::ol_ko;set rise {};set hit 0
  for {set i 0} {$i<$::ol_cnt} {incr i} {set r [lindex $::ol_rise $i];if {$r} {incr r -1};set bit [expr {1<<$i}]
   if {!($mask&$bit)} {set x [expr {[lindex $::ol_spx $i]*256}];set y [expr {[lindex $::ol_spy $i]*256}]
    if {[ol_s16 [expr {$::ol_px+64-$x}]]<$::ol_wx && [ol_s16 [expr {$x-$::ol_px-64}]]<256 && [ol_s16 [expr {$::ol_py+48-$y}]]<$::ol_wy && [ol_s16 [expr {$y-$::ol_py-48}]]<448} {
     set mask [expr {$mask|$bit}];set r 40;incr hit;if {$::ol_mode} {set ko [expr {($ko+1)&255}]} else {set bb [expr {($bb+1)&65535}]}}}
   lappend rise $r}
  incr ::ol_hit $hit
  set arise {};for {set i 0} {$i<$::ol_cnt} {incr i} {lappend arise [debug read memory [expr {$::ol_rp+$i}]]}
  if {$rise ne $arise || $mask!=[debug read memory $::ol_tp] || $bb!=[ol_w $::boobus_bombs] || $ko!=[debug read memory $::key_owned]} {incr ::ol_bad;if {$::ol_bad<4} {append ::ol_info " exp=($rise m$mask b$bb k$ko) got=($arise m[debug read memory $::ol_tp] b[ol_w $::boobus_bombs] k[debug read memory $::key_owned]) px=$::ol_px py=$::ol_py"}}}
 debug set_bp $::env(BENCH_OLORC) {} {ol_enter}}
if {[info exists ::env(BENCH_FRAMESTAT)]} {lassign $::env(BENCH_FRAMESTAT) fra frs
 proc fr_hit {} {set sp [reg SP];set ob [expr {[debug read memory [expr {$sp+6}]]+256*[debug read memory [expr {$sp+7}]]}];set f [debug read memory [expr {$ob+4}]];if {![info exists ::fr_n($f)]} {set ::fr_n($f) 0};incr ::fr_n($f)}
 debug set_bp $fra "\[debug read memory \$::g_CurrentSegment\]==$frs" {fr_hit}}
if {[info exists ::env(BENCH_CMDSTAT)]} {lassign $::env(BENCH_CMDSTAT) csa csb csg;set ::cs_g $csg;set ::cs_last {};set ::cs_last2 {};set ::cs_lt 0
 proc cs_hit {} {binary scan [debug read_block "memory" $::cs_g 15] cu* c;set t [machine_info time]
  if {$::cs_last ne {}} {set k $::cs_last;if {![info exists ::cs_n($k)]} {set ::cs_n($k) 0;set ::cs_px($k) 0;set ::cs_t($k) 0.0};}
  if {$::cs_last ne {} && ($t-$::cs_lt)<0.2} {set ::cs_t($::cs_last) [expr {$::cs_t($::cs_last)+$t-$::cs_lt}]}
  set sp [reg SP];set ret [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}];set seg [debug read memory $::g_CurrentSegment]
  if {$::cs_last2 ne {} && ($t-$::cs_lt)<0.2} {if {![info exists ::c2_t($::cs_last2)]} {set ::c2_t($::cs_last2) 0.0;set ::c2_n($::cs_last2) 0};set ::c2_t($::cs_last2) [expr {$::c2_t($::cs_last2)+$t-$::cs_lt}]}
  set k2 "[format %02x [lindex $c 14]] $ret $seg";if {![info exists ::c2_n($k2)]} {set ::c2_n($k2) 0;set ::c2_t($k2) 0.0};incr ::c2_n($k2);set ::cs_last2 $k2
  set cmd [lindex $c 14];set nx [expr {[lindex $c 8]+256*[lindex $c 9]}];set ny [expr {[lindex $c 10]+256*[lindex $c 11]}]
  set k [format "%02x" $cmd]
  if {![info exists ::cs_n($k)]} {set ::cs_n($k) 0;set ::cs_px($k) 0;set ::cs_t($k) 0.0}
  incr ::cs_n($k);incr ::cs_px($k) [expr {$nx*$ny}];set ::cs_last $k;set ::cs_lt $t}
 debug set_bp $csa {} {cs_hit};debug set_bp $csb {} {cs_hit}}
if {[info exists ::env(BENCH_BBORC)]} {lassign $::env(BENCH_BBORC) bba bbr bbs bbg bbo;set ::bb_set $bbs;set ::bb_g $bbg;set ::bb_old $bbo;set ::bb_n 0;set ::bb_act 0
 proc bb_log {} {if {!$::bb_act} {return};binary scan [debug read_block "memory" $::bb_g 15] cu* c;lappend c [machine_info time];lappend ::bb_cmds $c}
 proc bb_enter {} {set ::bb_sp [reg SP];set ::bb_ent [reg PC];set ::bb_pass 0;set ::bb_act 1;set ::bb_cmds {};set ::bb_c0 {};if {$::bb_old} {debug write memory $::bb_old 1}
  set pg [debug read memory [expr {$::bb_sp+4}]];set ob [expr {[debug read memory [expr {$::bb_sp+6}]]+256*[debug read memory [expr {$::bb_sp+7}]]}]
  if {[info exists ::env(BENCH_FORCEFRAME)]} {debug write memory [expr {$ob+4}] $::env(BENCH_FORCEFRAME)};binary scan [debug read_block "memory" $ob 6] cu* d;set ::bb_hdr "$pg $d [word $::camera_py]"}
 proc bb_ret {} {if {!$::bb_act} {return};if {$::bb_pass==0 && $::bb_old} {set ::bb_c0 $::bb_cmds;set ::bb_cmds {};set ::bb_pass 1;debug write memory $::bb_old 0;reg SP $::bb_sp;reg PC $::bb_ent;return}
  incr ::bb_n;set ::bb_act 0;set f [open $::env(BENCH_OUT).bb a];puts $f "CALL $::bb_hdr";puts $f "OLD $::bb_c0";puts $f "NEW $::bb_cmds";close $f}
 debug set_bp $bba {} {bb_enter};debug set_bp $bbr {} {bb_ret};debug set_bp $bbs {} {bb_log}}
if {[info exists ::env(BENCH_MTORC)]} {lassign $::env(BENCH_MTORC) mta mtbase;set ::mt_base $mtbase;set ::mt_n 0;set ::mt_bad 0;set ::mt_info {}
 proc mt_enter {} {set ::mt_i [expr {[reg HL]}];set sp [reg SP];set ::mt_ret [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}];set ::mt_hlr [reg HL];if {[info exists ::mt_bp]} {catch {debug remove_bp $::mt_bp}};set ::mt_bp [debug set_bp $::mt_ret {} {mt_exit}]}
 proc mt_word {a} {expr {[debug read memory $a]+256*[debug read memory [expr {$a+1}]]}}
 proc mt_raw {base i} {set n [expr {$i+($i>>3)}];expr {([mt_word [expr {$::world_map+$base+$n}]]>>($i&7))&511}}
 proc mt_exit {} {if {![info exists ::mt_bp]} {return};catch {debug remove_bp $::mt_bp};unset ::mt_bp;incr ::mt_n;set got [expr {[reg DE]}];set i $::mt_i
  if {$::mt_base==0} {set exp [mt_raw 0 $i]} else {set j [expr {[mt_raw 0 [expr {$i>>3}]]*8+($i&7)}];set exp [mt_raw $::mt_base $j]}
  if {$got!=$exp} {incr ::mt_bad;if {$::mt_bad<5} {append ::mt_info " i=$i exp=$exp got=$got"}}}
 debug set_bp $mta {} {mt_enter}}
if {[info exists ::env(BENCH_FLIPCHK)]} {set ::fl_n 0;set ::fl_busy 0;proc fl_chk {} {incr ::fl_n;binary scan [debug read_block "VDP status regs" 2 1] cu st;if {$st&1} {incr ::fl_busy}};debug set_bp $::env(BENCH_FLIPCHK) {} {fl_chk}}
if {[info exists ::env(BENCH_BTDIAG)]} {set bl $::env(BENCH_BTDIAG);lassign $bl bta btb btbombs btrise;set btsp {};foreach {tx ty} [lrange $bl 4 end] {lappend btsp [list $tx $ty]};set ::bt_bank $btb;set ::bt_bombs [expr {$::world_map+$btbombs}];set ::bt_rise_a [expr {$::world_map+$btrise}];set ::bt_sp $btsp;set ::bt_n 0;set ::bt_bad 0;set ::bt_hits 0;debug set_bp $bta {} {bt_enter}}
if {[info exists ::env(BENCH_LISR)]} {debug set_bp $::env(BENCH_LISR) {} {incr ::lcnt;lisr_enter};debug set_bp 0xFD9A {} {incr ::kcnt}}
proc shotlog {t f} {screenshot -raw $f;if {[info exists ::env(BENCH_VRAM)]} {set v [open [string map {.png .vram} $f] wb];puts -nonewline $v [debug read_block VRAM 0 131072];close $v;if {[info exists ::world_map]} {set w [open [string map {.png .wmap} $f] wb];puts -nonewline $w [debug read_block memory $::world_map 9835];close $w};set q [open [string map {.png .rec} $f] w];foreach p {0 1} {for {set k 0} {$k<29} {incr k} {set b [expr {$::actors_old+$p*174+$k*6}];if {[byte [expr {$b+5}]]&1} {puts $q "act p$p slot$k x=[word $b] y=[word [expr {$b+2}]] frame=[byte [expr {$b+4}]]"}};set a [expr {$::actors_old+$p*174+14*6}];puts $q "page$p hud x=[word $a] y=[word [expr {$a+2}]] frame=[byte [expr {$a+4}]] vis=[byte [expr {$a+5}]] ring_cam=[word [expr {$::ring_camera_x+2*$p}]]/[word [expr {$::ring_camera_x+4+2*$p}]]"};binary scan [debug read_block "VDP palette" 0 32] cu* pl;puts $q "palette [join $pl { }]";binary scan [debug read_block "VDP regs" 0 32] cu* rg;puts $q "regs [join $rg { }]";puts $q "vdp_fast=[expr {[info exists ::vdp_fast]?[byte $::vdp_fast]:-1}]";puts $q "ph=[expr {[info exists ::bar_ph]?[byte $::bar_ph]:-1}]";puts $q "bar=[expr {[info exists ::bar_on]?[byte $::bar_on]:-1}] cur=[expr {[info exists ::cur_r2]?"[byte $::cur_r2],[byte $::cur_r23],[byte $::cur_r26],[byte $::cur_r27]":"-"}]";puts $q "lisr=$::lcnt keyi=$::kcnt";puts $q "front=[byte $::front_page] cam=[word $::camera_px],[word $::camera_py] player=[expr {[word $::player_x]>>4}],[expr {[word $::player_y]>>4}] hero_frame=[byte $::hero_frame]";close $q};if {[info exists ::camera_px]} {set l [open $::env(BENCH_OUT).cam a];puts $l "$t [word $::camera_px] [word $::camera_py]";close $l}}
proc poll_level {} {if {[byte $::active_level]==$::env(BENCH_LEVEL) && [byte $::scene_ready]==165 && ![byte $::world_active]} {lvl_ready} else {after time 0.5 poll_level}}
proc lvl_ready {} {after time $::env(BENCH_LOADWAIT) begin}
proc press {} {if {[byte $::world_active]==1} {return};keymatrixdown 8 1;after time 0.3 {keymatrixup 8 1};after time 1.5 press}
after time 6 press
go_level
'''
out.write_text(s)
