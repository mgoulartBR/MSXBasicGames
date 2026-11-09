#!/usr/bin/env python3
"""Tempo ocupado (sem as esperas video_wait/V9_GetPort) por quadro nos cenarios de heavy012.tcl.
Metrica sem a quantizacao por vertical blank; amostragem a cada 0,5 ms emulados. uso: busy_heavy.py [VARIANTE]"""
import os,re,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];v=sys.argv[1] if len(sys.argv)>1 else 'V9990P1'
o=subprocess.run([sys.executable,str(R/'tests/prof13.py'),v],env=dict(os.environ,BASE='heavy012.tcl',TOP='9999'),capture_output=True,text=True).stdout
fr={m[1]:int(m[2]) for m in re.finditer(r'^METRIC (\w+) seconds=\S+ frames=(\d+)',o,re.M)}
for ph in ('eight','fight'):
    blk=o.split('PROFILE %s'%ph)[1].split('PROFILE')[0]
    rows=[(int(a),n) for a,n in re.findall(r'^\s+[\d.]+% +(\d+) (\S+)',blk,re.M)]
    tot=sum(a for a,n in rows);idle=sum(a for a,n in rows if n.endswith(('video_wait','V9_GetPort')))
    print('%-5s ocupado=%.1f ms/quadro (ocioso %.0f%%)  quadros=%d'%(ph,(tot-idle)*0.5/fr[ph],100*idle/tot,fr[ph]))
