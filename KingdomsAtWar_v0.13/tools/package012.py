from pathlib import Path
import zipfile,json,hashlib
R=Path(__file__).resolve().parents[1];D=R.parent/'KingdomsAtWar_v0.12_performance.zip';files=set()
def take(n):
 p=R/n;assert p.exists(),p
 if p.is_dir():files.update(x for x in p.rglob('*')if x.is_file()and'__pycache__'not in x.parts)
 else:files.add(p)
for n in ['README.md','CHANGELOG.md','WEBMSX.md','THIRD_PARTY_NOTICES.md','LICENSE_MSXgl.txt','LICENSE_V9968.txt','SHA256SUMS.txt','test_report.md','build.sh','play-v9990.sh','play-v9990.tcl','play-preview-v9958.sh','play-preview-v9958.tcl','KAW9990.ROM','KAW68PV.ROM','KAW9968.ROM','reference','art07','art/reference_v68_atlas.bin','art/reference_v68_font.bin','build','webmsx','tools']:take(n)
for root in ['src','tests']:
 for p in(R/root).rglob('*'):
  if p.is_file()and p.suffix in {'.c','.h','.js','.py','.sh','.tcl'}and'out'not in p.parts:take(p.relative_to(R))
v=json.loads((R/'tests/validation012.json').read_text());assert v['passed']
for name,h in json.loads((R/'tests/source012.json').read_text()).items():assert hashlib.sha256((R/name).read_bytes()).hexdigest()==h;assert R/name in files
for n,h in v['rom_sha256'].items():assert hashlib.sha256((R/(n+'.ROM')).read_bytes()).hexdigest()==h
for x in v['scenario_logs']:take('tests/'+x['file'])
for n in ['validation012','source012','performance012','features011','damage011','fog_runs012','native012-report','fog011','art012']:take('tests/'+n+'.json')
for v in ['V9990','PREVIEW']:
 take('tests/baseline012-'+v+'.log')
 for p in(R/f'screenshots/{v}').glob('*.png'):take(p.relative_to(R))
for pat in ['camera07-*-*.bin','camera07-*-meta.txt','perf091-*.bin','perf091-*.txt','overlay011-*.bin','overlay011-*.txt','fog011-*.bin','fog011-*.txt']:
 for p in(R/'tests').glob(pat):take(p.relative_to(R))
with zipfile.ZipFile(D,'w',zipfile.ZIP_DEFLATED,compresslevel=6)as z:
 for p in sorted(files):z.write(p,Path(R.name)/p.relative_to(R))
with zipfile.ZipFile(D)as z:assert z.testzip()is None
assert D.stat().st_size<50*1024*1024
print(D,D.stat().st_size,hashlib.sha256(D.read_bytes()).hexdigest())
