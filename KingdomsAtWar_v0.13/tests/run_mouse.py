#!/usr/bin/env python3
"""Inject SDL host events; the MSX reads its actual emulated mouse via PSG."""
from pathlib import Path
import os,sys,subprocess,re,hashlib,threading
r=Path(__file__).resolve().parents[1]
variant=sys.argv[1] if len(sys.argv)>1 else 'PREVIEW'
shared=r/'tests/host'/('mouse_events-'+variant+'-'+str(os.getpid())+'.so')
subprocess.run(['gcc','-shared','-fPIC','-O2','-pthread',str(r/'tests/host/mouse_events.c'),'-ldl','-o',str(shared)],check=True)
variant=sys.argv[1] if len(sys.argv)>1 else 'PREVIEW';script=sys.argv[2] if len(sys.argv)>2 else 'mouse_probe.tcl';port=sys.argv[3] if len(sys.argv)>3 else '1'
rom={'V9990':'KAW9990','V9990P1':'KAW9990P1','PREVIEW':'KAW68PV','V9968':'KAW9968'}[variant]
m=(r/'build'/variant/(rom+'.map')).read_text();sym={a[1]:int(a[0],16) for a in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+main\s*$',m,re.M)}
(r/'tests'/('symbols-'+variant+'.tcl')).write_text('\n'.join('set S(%s) %d'%(k,v)for k,v in sym.items())+'\n')
fifo=r/'tests'/('mouse-'+variant+'-'+port+'-'+str(os.getpid())+'.fifo')
if fifo.exists():fifo.unlink()
os.mkfifo(fifo)
env=dict(os.environ,SDL_VIDEODRIVER='offscreen',SDL_AUDIODRIVER='dummy',KAW_VARIANT=variant,KAW_MOUSE_PORT=port,KAW_MOUSE_FIFO=str(fifo),LD_PRELOAD=str(shared))
args=['openmsx','-machine',os.environ.get('KAW_MACHINE','C-BIOS_MSX2' if variant in ('V9990','V9990P1') else 'C-BIOS_MSX2+')]+(['-ext','gfx9000'] if variant in ('V9990','V9990P1') else [])+['-cart','../'+rom+'.ROM','-romtype','ASCII8','-script',script]
try:
 prefix='ROM_SHA256 '+hashlib.sha256((r/(rom+'.ROM')).read_bytes()).hexdigest()+'\n'
 with subprocess.Popen(args,cwd=r/'tests',env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,bufsize=1) as child:
  watchdog=threading.Timer(340 if script=="box011.tcl" else 180,child.kill);watchdog.start();lines=[]
  for line in child.stdout:lines.append(line);print(line,end='',flush=True)
  status=child.wait();watchdog.cancel()
 out=prefix+''.join(lines)
 (r/'tests'/(Path(script).stem+'-'+variant+'-port'+port+'.log')).write_text(out)
 assert status==0 and 'RESULT failures=0' in out and 'FAIL' not in out

finally:
 fifo.unlink(missing_ok=True)
 shared.unlink(missing_ok=True)
