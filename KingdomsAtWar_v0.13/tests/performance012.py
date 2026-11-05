from pathlib import Path
import re,json,statistics
R=Path(__file__).resolve().parents[1];report={}
for v in ['V9990','PREVIEW']:
 report[v]={}
 for phase in ['idle','one','four']:
  rows=[]
  for base in [R.parent/'KingdomsAtWar_v0.11',R]:
   p=(R/f'tests/baseline012-{v}.log') if base!=R else (R/f'tests/perf012-{v}.log');t=p.read_text();m=re.search(r'METRIC '+phase+r' seconds=(\S+) frames=(\d+) ticks=(\d+)',t);assert m
   secs=float(m[1]);f=int(m[2]);times=list(map(float,re.search(r'^CADENCE '+phase+r' (.*)',t,re.M)[1].split()));d=sorted((b-a)*1000 for a,b in zip(times,times[1:]));rows.append(dict(seconds=secs,frames=f,fps=f/secs,ticks=int(m[3]),median_ms=statistics.median(d) if d else None,p95_ms=d[min(len(d)-1,int(.95*len(d)))]if d else None,max_ms=max(d)if d else None,rom_sha256=re.search(r'ROM_SHA256 (\w+)',t)[1]))
  report[v][phase]={'before':rows[0],'after':rows[1],'fps_gain_percent':(rows[1]['fps']/rows[0]['fps']-1)*100 if rows[0]['fps']else None}
(R/'tests/performance012.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
