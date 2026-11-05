from pathlib import Path
import json,hashlib,shutil,re
R=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
hashes={n:sha(R/(n+'.ROM'))for n in ['KAW9990','KAW68PV','KAW9968']};logs=[]
for v,n in [('V9990','KAW9990'),('PREVIEW','KAW68PV')]:
 names=[f'{c}-{v}.log'for c in ['kernel012','perf012','fog011','combat011','worker011','camera011','overlay011','perf091',('defense011'if v=='V9990'else'kingdom011')]]+[f'box011-{v}-port1.log']
 for name in names:
  t=(R/'tests'/name).read_text();assert t.startswith('ROM_SHA256 '+hashes[n]+'\n'),name;assert 'FAIL'not in t and 'RESULT failures=0'in t,name;logs.append(dict(file=name,rom_sha256=hashes[n],checks=t.count('PASS ')))
t=(R/'tests/native_missing-V9968.log').read_text();assert t.startswith('ROM_SHA256 '+hashes['KAW9968']+'\n')and'RESULT failures=0'in t and 'FAIL'not in t;logs.append(dict(file='native_missing-V9968.log',rom_sha256=hashes['KAW9968'],checks=t.count('PASS ')))
for report in ['features011','damage011','fog_runs012','native012-report','fog011','art012']:
 assert json.loads((R/f'tests/{report}.json').read_text())['passed'],report
for name,h in json.loads((R/'tests/features011.json').read_text())['source_sha256'].items():assert sha(R/name)==h,name
assert json.loads((R/'tests/fog011.json').read_text())['rom_sha256']==hashes
for v in ['V9990','PREVIEW']:
 t=(R/f'tests/perf012-{v}.log').read_text();assert 'projectile_tick'not in re.search(r'CALLS idle (.*)',t)[1]and'fog_update'not in re.search(r'CALLS idle (.*)',t)[1]and'repair_tick'not in re.search(r'CALLS idle (.*)',t)[1]
for n in ['KAW9990','KAW68PV']:shutil.copyfile(R/(n+'.ROM'),R/'webmsx'/(n+' [ASCII8].rom'))
(R/'SHA256SUMS.txt').write_text(''.join(h+'  '+n+'.ROM\n'for n,h in hashes.items()))
frozen={str(p.relative_to(R)):sha(p)for root in ['src','tools','tests']for p in(R/root).rglob('*')if p.is_file()and p.suffix in {'.c','.h','.js','.py','.sh','.tcl'}and'out'not in p.parts}
(R/'tests/source012.json').write_text(json.dumps(frozen,indent=2)+'\n')
v=dict(version='0.12',passed=True,rom_sha256=hashes,scenario_logs=logs,limitations=['Positive V9968 hardware validation pending; PREVIEW uses a V9958','Measurements use openMSX Z80 at standard clock; not a physical-hardware benchmark','WebMSX itself not rerun; actual emulated mouse protocol tested through SDL','No claim of 30/60 FPS; see scenario-specific cadence in performance012.json'])
(R/'tests/validation012.json').write_text(json.dumps(v,indent=2)+'\n');print('PASS current-ROM validation',sum(x['checks']for x in logs),'scenario checks')
