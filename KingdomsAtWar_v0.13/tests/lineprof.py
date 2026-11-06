#!/usr/bin/env python3
"""Perfil inclusivo por linha de C dentro de uma funcao do modulo main: bps no primeiro endereco de cada linha.
uso: lineprof.py VARIANTE funcao arquivo.h/.c  (executa perf012; imprime ms por linha)"""
import re,sys,os,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1];v,fn,fil=sys.argv[1:4]
banked=fil.startswith('banks/')
mod=Path(fil).stem
B=Path(os.environ.get('BPDIR','/tmp/claude-0/-home-user-MSXBasicGames/4527bde8-f4ff-521f-8bcc-a6ecfa430c6f/scratchpad/bp'))/v/'src/out'/((mod+'.lst') if banked else 'main.lst')
nm={'V9990':'KAW9990','PREVIEW':'KAW68PV','V9968':'KAW9968'}[v]
m=(R/'build'/v/(nm+'.map')).read_text();s=(R/'build'/v/'main.sym').read_text()
off=int(re.search(r'([0-9A-F]{8})\s+_video_wait\s+main',m)[1],16)-int(re.search(r'_video_wait\s+([0-9A-F]{8})',s)[1],16)
if banked:
 ms=(R/'build'/v/(mod+'.sym')).read_text();fa=int(re.search(r'_%s\s+([0-9A-F]{8})'%fn,ms)[1],16);off=0xA000;bank=int(re.search(r'art_s(\d+)',mod)[1])
 syms=sorted(int(a,16) for n,a in re.findall(r'^\s*\d+ (_\w+)\s+([0-9A-F]{8})\s',ms,re.M))
else:
 fa=int(re.search(r'_%s\s+([0-9A-F]{8})'%fn,s)[1],16);bank=0
 syms=sorted(int(a,16) for n,a in re.findall(r'^\s*0 (_\w+)\s+([0-9A-F]{8})\s',s,re.M))
# fim: proximo simbolo de funcao
fe=min([x for x in syms if x>fa]+[fa+0x800])
cur=None;first={}
for l in B.read_text().split('\n'):
    mm=re.search(r';\S*?%s:(\d+):'%re.escape(fil),l)
    if mm: cur=int(mm[1]);continue
    mm=re.match(r'\s+([0-9A-F]{8})\s',l)
    if mm and cur is not None:
        a=int(mm[1],16)
        if fa<=a<fe and cur not in first: first[cur]=a
if not first:sys.exit('sem linhas')
t=(R/'tests/perf012.tcl').read_text()
hook='''set ::lp_last ""; set ::lp_t 0; array set ::lp_acc {}
proc lp_hit {line} {if {!$::perf_running} return;set now [machine_info time];if {$::lp_last ne ""} {if {[info exists ::lp_acc($::lp_last)]} {set ::lp_acc($::lp_last) [expr {$::lp_acc($::lp_last)+$now-$::lp_t}]} else {set ::lp_acc($::lp_last) [expr {$now-$::lp_t}]}};set ::lp_last $line;set ::lp_t $now}
proc lp_end {} {if {$::lp_last ne ""} {set now [machine_info time];if {[info exists ::lp_acc($::lp_last)]} {set ::lp_acc($::lp_last) [expr {$::lp_acc($::lp_last)+$now-$::lp_t}]} else {set ::lp_acc($::lp_last) [expr {$now-$::lp_t}]}};set ::lp_last ""}
'''
bps=' debug set_bp %d {} {lp_end}\n'%(fe+off)  # nao usado
cond='$::perf_running'+(' && [debug read memory 0xC006]==%d'%bank if bank else '')
lines=''.join(' debug set_bp %d {%s} {lp_hit %d}\n'%(a+off,cond,l) for l,a in first.items())
# no retorno da funcao: bp no endereco logo apos o call nao conhecido -> usamos entrada de funcao para fechar a linha anterior
entry=' debug set_bp %d {%s} {lp_end}\n'%(fa+off,cond)
t=hook+t
t=t.replace(' perf_start;delay 2;perf_end idle',lines+entry+' perf_start;delay 2;perf_end idle',1)
t=t.replace(' puts stderr "RESULT failures=$::failures";exit',' foreach k [array names ::lp_acc] {puts stderr "LP $k $::lp_acc($k)"}\n puts stderr "RESULT failures=$::failures";exit',1)
t=t.replace('proc perf_end {phase} {','proc perf_end {phase} {\n if {$phase ne "four"} {array unset ::lp_acc;set ::lp_last ""}',1)
(R/'tests/lineprof_run.tcl').write_text(t)
env=dict(os.environ,KAW_TEST_SCRIPT='lineprof_run.tcl')
out=subprocess.run([sys.executable,str(R/'tests/run_tests.py'),v],env=env,capture_output=True,text=True)
txt=out.stdout+out.stderr
rows={}
for mm in re.finditer(r'^LP (\d+) (\S+)',txt,re.M):rows[int(mm[1])]=float(mm[2])*1000
src=(R/'src'/fil).read_text().split('\n')
tot=sum(rows.values())
print('total ms',round(tot,1))
for l,ms in sorted(rows.items(),key=lambda x:-x[1])[:int(os.environ.get('TOP','14'))]:print('%7.1f ms %5.1f%%  L%d %s'%(ms,100*ms/tot,l,src[l-1][:100]))
