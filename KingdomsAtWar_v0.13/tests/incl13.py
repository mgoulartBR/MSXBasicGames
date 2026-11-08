#!/usr/bin/env python3
"""Tempo inclusivo (emulado) por funcao nas fases idle/one/four do cenario perf012.
uso: incl13.py VARIANTE [funcoes...]"""
from pathlib import Path
import re,sys,os,subprocess
R=Path(__file__).resolve().parents[1];v=sys.argv[1]
funcs=sys.argv[2:] or 'simulate input camera_update fog_update hud video_sprites video_present cache_frame draw_actor paint_world restore_field fog_apply fog_cover structure_bars damage_mask navigation nav_prepare combat_tick economy production tower_tick repair_tick projectile_tick workers asset_read poll_input arrow_draw video_stream'.split()
nm={'V9990':'KAW9990','V9990P1':'KAW9990P1','PREVIEW':'KAW68PV','V9968':'KAW9968'}[v]
mp=(R/'build'/v/(nm+'.map')).read_text();ms=(R/'build'/v/'main.sym').read_text()
off=int(re.search(r'([0-9A-F]{8})\s+_video_wait\s+main',mp)[1],16)-int(re.search(r'_video_wait\s+([0-9A-F]{8})',ms)[1],16)
extra={}
for n,a in re.findall(r'^\s*0 _(\w+)\s+([0-9A-F]{8})\s',ms,re.M):extra[n]=int(a,16)+off
t=(R/'tests'/os.environ.get('BASE','perf012.tcl')).read_text()
pre='''
array set ::extra {%s}
'''%' '.join('%s %d'%(k,x) for k,x in extra.items())+'''
set ::incl [dict create];set ::calls [dict create];set ::pend [dict create]
proc trk_enter {name} {
 if {!$::perf_running} return
 set sp [reg SP];set ra [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}]
 dict set ::pend $sp [list $name [machine_info time]]
 set id [debug set_bp $ra "\\$::perf_running && \\[reg SP\\]==[expr {$sp+2}]" "trk_leave $sp"]
 dict set ::pendbp $sp $id
}
proc trk_leave {sp} {
 if {![dict exists $::pend $sp]} return
 lassign [dict get $::pend $sp] name t0
 dict incr ::calls $name
 if {[dict exists $::incl $name]} {dict set ::incl $name [expr {[dict get $::incl $name]+[machine_info time]-$t0}]} else {dict set ::incl $name [expr {[machine_info time]-$t0}]}
 catch {debug remove_bp [dict get $::pendbp $sp]}
 dict unset ::pend $sp
}
set ::pendbp [dict create]
proc dump_incl {phase} {
 set l {};dict for {k v} $::incl {lappend l [list $k $v [dict get $::calls $k]]}
 foreach e [lsort -decreasing -real -index 1 $l] {puts stderr [format "INCL %s %-16s %8.1f ms  calls=%d  avg=%.2f ms" $phase [lindex $e 0] [expr {[lindex $e 1]*1000}] [lindex $e 2] [expr {[lindex $e 1]*1000/[lindex $e 2]}]]}
 set ::incl [dict create];set ::calls [dict create]
}
'''
t=t.replace('proc perf_end {phase} {','proc perf_end {phase} {\n dump_incl $phase',1)
hook='''
 set variant $::env(KAW_VARIANT);set nm [dict get {V9990 KAW9990 V9990P1 KAW9990P1 PREVIEW KAW68PV V9968 KAW9968} $variant]
 set f [open ../build/$variant/$nm.map];set mtext [read $f];close $f
 foreach fn {%s} {
  set address -1
  if {[info exists ::extra($fn)]} {set address $::extra($fn);set bank 0;set pc $address;set condition "\\$::perf_running";debug set_bp $pc $condition "trk_enter $fn";continue}
  if {[regexp "(\\[0-9A-F\\]{8})\\\\s+_$fn\\\\s" $mtext whole address]} {
   scan $address %%x address;set bank [expr {$address>>16}];set pc [expr {$address&65535}]
   set condition [expr {$bank?"\\$::perf_running && \\[debug read memory 0xC006\\]==$bank":"\\$::perf_running"}]
   debug set_bp $pc $condition "trk_enter $fn"
  }
 }
'''%(' '.join(funcs))
if ' perf_start;delay 2;perf_end idle' in t:t=t.replace(' perf_start;delay 2;perf_end idle',hook+' perf_start;delay 2;perf_end idle',1)
else:t=t.replace(' delay 45;choose 2;world_point 0 0\n',' delay 45;choose 2;world_point 0 0\n'+hook,1)
(R/'tests/incl13_run.tcl').write_text(pre+t)
env=dict(os.environ,KAW_TEST_SCRIPT='incl13_run.tcl')
r=subprocess.run([sys.executable,str(R/'tests/run_tests.py'),v],env=env,capture_output=True,text=True)
txt=r.stdout+r.stderr;seen=set()
for l in txt.splitlines():
    if (l.startswith('INCL') or l.startswith('METRIC') or 'FAIL' in l or 'rror' in l) and l not in seen:seen.add(l);print(l)
