"""Package only current source, ROM and validation evidence for v0.11."""
from pathlib import Path
import zipfile,json,hashlib
R=Path(__file__).resolve().parents[1];D=R.parent/'KingdomsAtWar_v0.11_defesa_fog.zip';files=set()
def take(n):
 p=R/n;assert p.exists(),p
 if p.is_dir():files.update(x for x in p.rglob('*') if x.is_file() and '__pycache__' not in x.parts)
 else:files.add(p)
for n in ['README.md','CHANGELOG.md','WEBMSX.md','THIRD_PARTY_NOTICES.md','LICENSE_MSXgl.txt','LICENSE_V9968.txt','SHA256SUMS.txt','test_report.md','build.sh','play-v9990.sh','play-v9990.tcl','play-preview-v9958.sh','play-preview-v9958.tcl','KAW9990.ROM','KAW68PV.ROM','KAW9968.ROM','reference','art07','art/reference_v68_atlas.bin','art/reference_v68_font.bin','build','webmsx','tools/prepare09.py','tools/package011.py','tools/validate011.py']:take(n)
for p in (R/'src').rglob('*'):
 if p.is_file() and p.suffix in {'.c','.h','.js'} and 'out' not in p.parts:take(p.relative_to(R))
for n in ['helpers.tcl','kingdom_helpers.tcl','mouse_helpers.tcl','run_tests.py','run_mouse.py','boot07.tcl','native_missing.tcl','perf091.tcl','check_memory092.py','perf011-before-mask-V9990.log','perf011-baseline010-V9990.log','suite011.sh','features011.py','features011.json','damage011.py','damage011.json','kingdom011.tcl','defense011.tcl','fog011.tcl','helpers011.tcl','combat011.tcl','worker011.tcl','camera011.tcl','overlay011.tcl','box011.tcl','verify_fog011.py','verify_art011.py','art011.json','fog011.json','validation011.json','source011.json','host/mouse_events.c','host/SDL_events_reference.h']:take('tests/'+n)
v=json.loads((R/'tests/validation011.json').read_text());assert v['passed']
for n,h in json.loads((R/'tests/source011.json').read_text()).items():assert hashlib.sha256((R/n).read_bytes()).hexdigest()==h;assert R/n in files,n
for n,h in v['rom_sha256'].items():assert hashlib.sha256((R/(n+'.ROM')).read_bytes()).hexdigest()==h
for x in v['scenario_logs']:take('tests/'+x['file'])
for var in ['V9990','PREVIEW']:
 for n in ['repair011-damaged','repair011-working','repair011-complete','tower011-firing','combat09-duel','combat09-siege','combat09-cleared','box091-held','box091-released','box091-group-combat','box091-corner','perf091','worker07','new_worker_in_mine','camera07-return',f'camera07-{var}-corner',f'camera07-{var}-middle',f'fog011-{var}-unknown',f'fog011-{var}-visible',f'fog011-{var}-remembered',f'overlay011-{var}-first',f'overlay011-{var}-moved',f'overlay011-{var}-cancelled']:take(f'screenshots/{var}/{n}.png')
take('screenshots/PREVIEW/research011-knight.png');take('screenshots/PREVIEW/kingdom011-finished.png');take('screenshots/V9990/defense011-finished.png')
for pat in ['camera07-*-*.bin','camera07-*-meta.txt','box091-*.bin','box091-*.txt','perf091-*.bin','perf091-*.txt','overlay011-*.bin','overlay011-*.txt','fog011-*.bin','fog011-*.txt']:
 for p in (R/'tests').glob(pat):take(p.relative_to(R))
take('screenshots/V9968_detection_no_device.png')
for n in ['KAW68PV','KAW9990']:assert (R/(n+'.ROM')).read_bytes()==(R/'webmsx'/(n+' [ASCII8].rom')).read_bytes()
with zipfile.ZipFile(D,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in sorted(files):z.write(p,Path(R.name)/p.relative_to(R))
with zipfile.ZipFile(D) as z:assert z.testzip() is None
assert D.stat().st_size<50*1024*1024
print(D,D.stat().st_size,len(files),hashlib.sha256(D.read_bytes()).hexdigest())
