#!/usr/bin/env python3
"""Perfil de PC (amostra a cada 0,5 ms emulados) do cenario perf012.
uso: [GATE=funcao] [TOP=n] prof13.py VARIANTE
GATE restringe as amostras ao interior (inclusivo) de uma funcao global do jogo."""
from pathlib import Path
import re,sys,os,subprocess
R=Path(__file__).resolve().parents[1];v=sys.argv[1];rows={}
nm={'V9990':'KAW9990','PREVIEW':'KAW68PV','V9968':'KAW9968'}[v]
mp=(R/'build'/v/(nm+'.map')).read_text()
MAIN_OFF=int(re.search(r'([0-9A-F]{8})\s+_video_wait\s+main',mp)[1],16)-int(re.search(r'_video_wait\s+([0-9A-F]{8})',(R/'build'/v/'main.sym').read_text())[1],16)
for bank,p in [(0,R/'build'/v/'main.sym')]+[(int(re.search('art_s(\\d+)',p.name)[1]),p)for p in (R/'build'/v).glob('art_s*_b3.sym')]:
 for area,n,a in re.findall(r'^\s*(\d+)\s+_(\w+)\s+([0-9A-F]{8})\s+[GL]*R\s*$',p.read_text(),re.M):
  if (bank==0 and area!='0')or(bank and area!='7'):continue
  rows[(bank,int(a,16)+(MAIN_OFF if not bank else 0xA000))]=n
# simbolos globais de bibliotecas (depois do modulo main)
for a,n,mod in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+(\w+)\s*$',mp,re.M):
 a=int(a,16)
 if a<0x10000 and mod not in('main',) and a>=0x4000:rows[(0,a)]=n
out=[]
for b in sorted({b for b,a in rows}):out.append('set ::ranges(%d) {%s}'%(b,' '.join('{%d %s}'%(a,n)for(bb,a),n in sorted(rows.items())if bb==b)))
(R/'tests'/('ranges_'+v+'.tcl')).write_text('\n'.join(out)+'\n')
gate=os.environ.get('GATE','')
gate_tcl=''
if gate:
 m=re.search(r'([0-9A-F]{8})\s+_%s\s'%gate,mp)
 if m:ad=int(m[1],16)
 else:ad=int(re.search(r'_%s\s+([0-9A-F]{8})'%gate,(R/'build'/v/'main.sym').read_text())[1],16)+MAIN_OFF
 bank=ad>>16;pc=ad&65535
 cond='[debug read memory 0xC006]==%d'%bank if bank else '1'
 gate_tcl='debug set_bp %d {$::perf_running && %s} {gate_enter}\n'%(pc,cond)
t=(R/'tests/perf012.tcl').read_text()
prefix='''source ranges_%s.tcl
set ::gate_open %d
proc gate_enter {} {set ::gate_open 1;set sp [reg SP];set ra [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}];set ::gate_bp [debug set_bp $ra "\\[reg SP\\]==[expr {$sp+2}]" {set ::gate_open 0;catch {debug remove_bp $::gate_bp}}]}
proc sample_prof {} {
 if {!$::perf_running} {return}
 if {$::gate_open} {
 set pc [reg PC];set bank [expr {$pc<40960?0:[debug read memory 0xC006]}];set key $bank:unknown
 if {[info exists ::ranges($bank)]} {foreach r $::ranges($bank) {lassign $r a n;if {$a>$pc} {break};set key $bank:$n}}
 dict incr ::samples_prof $key}
 after time 0.0005 sample_prof
}
proc sorted_prof {} {set a {};dict for {k v} $::samples_prof {lappend a [list $v $k]};return [lsort -decreasing -integer -index 0 $a]}
'''%(v,0 if gate else 1)
t=t.replace('set ::perf_times {};','set ::samples_prof [dict create];set ::perf_times {};',1).replace(' set ::start [machine_info time];',' sample_prof\n set ::start [machine_info time];',1).replace(' puts stderr "CALLS $phase',' puts stderr "PROFILE $phase [sorted_prof]"\n puts stderr "CALLS $phase',1)
t=t.replace(' perf_start;delay 2;perf_end idle',gate_tcl+' perf_start;delay 2;perf_end idle',1)
(R/'tests'/'prof13_run.tcl').write_text(prefix+t)
env=dict(os.environ,KAW_TEST_SCRIPT='prof13_run.tcl')
r=subprocess.run([sys.executable,str(R/'tests/run_tests.py'),v],env=env,capture_output=True,text=True)
txt=r.stdout+r.stderr;seen=set()
for m in re.finditer(r'^METRIC (\w+) (.*)$',txt,re.M):
 if m[0] not in seen:seen.add(m[0]);print(m[0])
done=set()
for m in re.finditer(r'^PROFILE (\w+) (.*)$',txt,re.M):
 ph=m[1]
 if ph in done:continue
 done.add(ph);lst=[(int(a),b) for a,b in re.findall(r'(\d+) (\d+:\w+)',m[2])];tot=sum(a for a,b in lst) or 1
 print('PROFILE',ph,'total',tot)
 for a,b in lst[:int(os.environ.get('TOP','22'))]:print('  %5.1f%% %5d %s'%(100*a/tot,a,b))
if 'FAIL' in txt:print(txt[-1500:])
