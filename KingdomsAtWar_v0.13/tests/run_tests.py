#!/usr/bin/env python3
import re,os,subprocess,sys,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
for variant in (sys.argv[1:] or ['V9990','PREVIEW']):
 name={'V9990':'KAW9990','V9990P1':'KAW9990P1','PREVIEW':'KAW68PV','V9968':'KAW9968'}[variant]
 s=(root/'build'/variant/(name+'.map')).read_text()
 sym={m[1]:int(m[0],16) for m in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+main\s*$',s,re.M)}
 (root/'tests'/('symbols-'+variant+'.tcl')).write_text('\n'.join('set S(%s) %d'%(k,v)for k,v in sym.items())+'\n')
 env=dict(os.environ,SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',KAW_VARIANT=variant)
 args=['openmsx','-machine','C-BIOS_MSX2' if variant in ('V9990','V9990P1') else 'C-BIOS_MSX2+']
 if variant in ('V9990','V9990P1'):args+=['-ext','gfx9000']
 script=os.environ.get('KAW_TEST_SCRIPT','boot07.tcl')
 args+=['-cart','../'+name+'.ROM','-romtype','ASCII8','-script',script]
 rom_hash=hashlib.sha256((root/(name+'.ROM')).read_bytes()).hexdigest()
 import threading
 parts=[]
 with subprocess.Popen(args,cwd=root/'tests',env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,bufsize=1) as child:
  timer=threading.Timer(1200 if script in {"military010.tcl","kingdom011.tcl"} else 540,child.kill);timer.start()
  for line in child.stdout:parts.append(line);print(line,end='',flush=True)
  returncode=child.wait();timer.cancel()
 out='ROM_SHA256 '+rom_hash+'\n'+''.join(parts);suffix='-classic-no-mouse' if os.environ.get('KAW_CLASSIC') else '';(root/'tests'/(('economy-' if script=='economy.tcl' else 'acceptance-' if script=='acceptance.tcl' else Path(script).stem+'-')+variant+suffix+'.log')).write_text(out)
 print(variant,out,flush=True)
 if returncode or 'FAIL' in out or 'RESULT failures=0' not in out:sys.exit(1)
