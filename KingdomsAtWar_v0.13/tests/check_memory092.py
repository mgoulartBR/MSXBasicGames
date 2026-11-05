"""Reject ROM layouts that overlap ASCII8 windows or the reserved stack."""
from pathlib import Path
import re,sys
R=Path(__file__).resolve().parents[1]
for v in sys.argv[1:] or ['V9990','PREVIEW','V9968']:
 n={'V9990':'KAW9990','PREVIEW':'KAW68PV','V9968':'KAW9968'}[v]
 s=(R/f'build/{v}/{n}.map').read_text();d={k:int(a,16) for a,k in re.findall(r'^\s+([0-9A-F]{8})\s+((?:s|l)__\w+)\s*$',s,re.M)}
 end=d['s__CODE']+d['l__CODE'];ram=d['s__INITIALIZED']+d['l__INITIALIZED'];banks=[a for k,a in d.items() if k.startswith('l__SEG')]
 assert end<=0xA000,(v,'fixed ROM overflow',hex(end))
 assert ram<0xF380,(v,'RAM/stack overlap',hex(ram))
 assert banks and max(banks)<=8192,(v,'bank overflow')
 b=(R/(n+'.ROM')).read_bytes();assert len(b)==262144 and b[:2]==b'AB'
 print(f'PASS {v} fixed_end={end:04X} RAM_end={ram:04X} largest_bank={max(banks)}')
