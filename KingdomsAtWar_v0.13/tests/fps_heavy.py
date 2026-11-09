#!/usr/bin/env python3
"""Cadencia nos cenarios pesados de heavy012.tcl (8 unidades grandes andando; luta em grupo).
uso: fps_heavy.py [VARIANTE]   (intervalos > 150 ms sao esperas ociosas e saem da media)"""
import os,re,statistics,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];v=sys.argv[1] if len(sys.argv)>1 else 'V9990P1'
o=subprocess.run([sys.executable,str(R/'tests/run_tests.py'),v],env=dict(os.environ,KAW_TEST_SCRIPT='heavy012.tcl'),capture_output=True,text=True);txt=o.stdout+o.stderr
for ph in ('mixed','eight','fight'):
    c=[float(x) for x in re.search(r'^CADENCE %s (.*)$'%ph,txt,re.M)[1].split()]
    d=[(b-a)*1000 for a,b in zip(c,c[1:])];act=[x for x in d if x<=150];s=sorted(act)
    print('%-5s ativos=%d media=%.1f ms (%.1f fps) mediana=%.0f p90=%.0f max=%.0f  >50ms: %d  ociosos=%d'%(ph,len(act),sum(act)/len(act),1000*len(act)/sum(act),statistics.median(act),s[int(len(s)*.9)],s[-1],sum(x>50 for x in act),len(d)-len(act)))
