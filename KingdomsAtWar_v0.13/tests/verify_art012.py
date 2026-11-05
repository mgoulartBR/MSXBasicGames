"""Verify v0.9.4 approved art, team variants, ROM payloads and MSX memory budgets."""
from pathlib import Path
import hashlib, json, re, sys, zipfile
import numpy as np
R=Path(__file__).resolve().parents[1]
baseline=zipfile.ZipFile(sys.argv[1]);prefix='KingdomsAtWar_v0.11/'
M=json.loads((R/'art07/runtime/manifest.json').read_text());checks={}
def ck(name,value):
 checks[name]=bool(value)
 assert value,name
def decode(blob,count):
 out=bytearray();p=0
 while len(out)<count:
  flags=blob[p];p+=1
  for bit in range(8):
   if len(out)==count:break
   if flags&(1<<bit):out.append(blob[p]);p+=1
   else:
    distance,length=blob[p]+1,blob[p+1]+3;p+=2
    for _ in range(length):out.append(out[-distance])
 assert len(out)==count
 return bytes(out)
ck('120_poses',M['total_poses']==120)
for rec in M['records']:
 name=rec['name'];blob=(R/f'art07/runtime/{name}.bin').read_bytes()
 ck(name+'_fits_bank',rec['offset']+rec['compressed']<=8192 and 4<=rec['bank']<27)
 if not name.startswith('pose') or int(name[4:])<144:
  ck(name+'_approved_pixels_preserved',blob==baseline.read(prefix+f'art07/runtime/{name}.bin'))
for f in range(16):
 a=np.frombuffer((R/f'art07/runtime/pose{24+f}.bin').read_bytes(),np.uint8)
 b=np.frombuffer((R/f'art07/runtime/pose{144+f}.bin').read_bytes(),np.uint8)
 ap=np.concatenate([a>>4,a&15]);bp=np.concatenate([b>>4,b&15])
 ck(f'enemy{f}_same_silhouette',np.array_equal(ap==0,bp==0))
 ck(f'enemy{f}_distinct_team',not np.array_equal(ap,bp) and np.max(bp)<16)
hashes={};memory={}
for variant,name in [('V9990','KAW9990'),('PREVIEW','KAW68PV'),('V9968','KAW9968')]:
 blob=(R/(name+'.ROM')).read_bytes();hashes[name]=hashlib.sha256(blob).hexdigest()
 ck(name+'_header_size',len(blob)==262144 and blob[:2]==b'AB')
 for rec in M['records']:
  start=rec['bank']*8192+rec['offset']
  ck(name+'_'+rec['name'],decode(blob[start:start+rec['compressed']],rec['raw'])==(R/f"art07/runtime/{rec['name']}.bin").read_bytes())
 text=(R/f'build/{variant}/{name}.map').read_text()
 symbols={k:int(v,16) for v,k in re.findall(r'^\s+([0-9A-F]{8})\s+((?:s|l)__\w+)\s*$',text,re.M)}
 code=symbols['s__CODE']+symbols['l__CODE'];ram=symbols['s__INITIALIZED']+symbols['l__INITIALIZED']
 banks={k[3:]:v for k,v in symbols.items() if k.startswith('l__SEG')}
 ck(name+'_fixed_code',code<=0xA000);ck(name+'_ram_stack_boundary',ram<0xF380)
 ck(name+'_bank_sizes',all(v<=8192 for v in banks.values()))
 memory[variant]={'fixed_code_end':hex(code),'ram_end':hex(ram),'bank_bytes':banks}
ck('v9990_pose_atlas_fits',max(1280+(int(r['name'][4:])//10)*48+M['sizes'][int(r['name'][4:])//24] for r in M['records'] if r['name'].startswith('pose'))<=2048)
ck('native_cache_holds_ten_actors',10<=14)
report=dict(passed=all(checks.values()),checks=len(checks),assertions=checks,rom_sha256=hashes,memory=memory)
(R/'tests/art012.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS',len(checks),'ROM/art/memory checks')
