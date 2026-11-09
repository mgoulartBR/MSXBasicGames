#!/usr/bin/env python3
"""Corpo a corpo parado (melee012.tcl): quadros desenhados e tempo ocupado. uso: melee_report.py [VARIANTE]"""
import os,re,statistics,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];v=sys.argv[1] if len(sys.argv)>1 else 'V9990P1'
o=subprocess.run([sys.executable,str(R/'tests/prof13.py'),v],env=dict(os.environ,BASE='melee012.tcl',TOP='9999'),capture_output=True,text=True).stdout
m=re.search(r'^METRIC melee seconds=(\S+) frames=(\d+)',o,re.M);sec=float(m[1]);fr=int(m[2])
blk=o.split('PROFILE melee')[1].split('PROFILE')[0]
rows=[(int(a),n) for a,n in re.findall(r'^\s+[\d.]+% +(\d+) (\S+)',blk,re.M)]
tot=sum(a for a,n in rows);idle=sum(a for a,n in rows if n.endswith(('video_wait','V9_GetPort')))
print('melee quadros=%d em %.1f s (%.1f/s)  CPU ocupada=%.0f%%  ocupado/quadro=%.1f ms'%(fr,sec,fr/sec,100*(tot-idle)/tot,(tot-idle)*0.5/fr))
