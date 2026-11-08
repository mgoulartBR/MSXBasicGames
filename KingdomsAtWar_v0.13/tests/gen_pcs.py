#!/usr/bin/env python3
"""gera tests/pcs4.tcl: histograma de PC dentro de uma funcao global de banco. uso: gen_pcs.py VARIANTE funcao"""
import re,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];v,fn=sys.argv[1:3]
nm={'V9990':'KAW9990','V9990P1':'KAW9990P1','PREVIEW':'KAW68PV','V9968':'KAW9968'}[v]
m=(R/'build'/v/(nm+'.map')).read_text()
a=int(re.search(r'([0-9A-F]{8})\s+_%s\s'%fn,m)[1],16);pc=a&0xFFFF;bank=a>>16
t=(R/'tests/perf012.tcl').read_text()
t=t.replace('proc perf012 {} {','''set ::pcs [dict create];set ::gate 0
proc gate_enter {} {set ::gate 1;set sp [reg SP];set ra [expr {[debug read memory $sp]+256*[debug read memory [expr {$sp+1}]]}];set ::gate_bp [debug set_bp $ra "\\[reg SP\\]==[expr {$sp+2}]" {set ::gate 0;catch {debug remove_bp $::gate_bp}}]}
proc samp {} {if {$::gate} {dict incr ::pcs [reg PC]};after time 0.0002 samp}
proc perf012 {} {''',1)
cond='{[debug read memory 0xC006]==%d}'%bank if bank else '{}'
t=t.replace(' perf_start;delay 2;perf_end idle',' debug set_bp %d %s {gate_enter}\n samp\n perf_start;delay 2;perf_end idle'%(pc,cond),1)
t=t.replace(' puts stderr "RESULT failures=$::failures";exit',' dict for {k v} $::pcs {puts stderr "PCS $k $v"}\n puts stderr "RESULT failures=$::failures";exit',1)
(R/'tests/pcs4.tcl').write_text(t)
