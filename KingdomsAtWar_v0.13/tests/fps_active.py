#!/usr/bin/env python3
"""Cadencia de quadros do cenario perf012 'four' sem as esperas ociosas (intervalos > 100 ms: a unidade parada
esperando o roteiro do teste). uso: fps_active.py [VARIANTE]"""
import os,re,statistics,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[1];v=sys.argv[1] if len(sys.argv)>1 else 'V9990P1'
env=dict(os.environ,KAW_TEST_SCRIPT='perf012.tcl')
o=subprocess.run([sys.executable,str(R/'tests/run_tests.py'),v],env=env,capture_output=True,text=True);txt=o.stdout+o.stderr
c=[float(x) for x in re.search(r'^CADENCE four (.*)$',txt,re.M)[1].split()]
d=[(b-a)*1000 for a,b in zip(c,c[1:])];act=[x for x in d if x<=100]
print('frames=%d ativos=%d media=%.1f ms (%.1f fps) mediana=%.1f ms  <=36ms: %d%%  ociosos=%d'%(len(c),len(act),sum(act)/len(act),1000*len(act)/sum(act),statistics.median(act),100*sum(x<=36 for x in act)/len(act),len(d)-len(act)))
print('maiores:',[round(x) for x in sorted(act)[-8:]],' p90=%.0f'%sorted(act)[int(len(act)*.9)])
