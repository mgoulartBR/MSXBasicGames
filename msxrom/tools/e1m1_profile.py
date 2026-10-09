"""Perfil de CPU do E1M1 no Z80 emulado (T-states por quadro, sem esperas de VDP/Geo3D) e por funcao. Usa private/."""
import re,sys,os,collections,bisect,subprocess
sys.path.insert(0,os.path.dirname(__file__))
from z80_harness import Machine
R=os.path.join(os.path.dirname(__file__),'..'); P=os.path.join(R,'out','prof'); os.makedirs(P,exist_ok=True)
src=re.sub(r'^static ','',open(os.path.join(R,'src','e1m1.c')).read(),flags=re.M); open(os.path.join(P,'e1m1.c'),'w').write(src)
def sh(c): subprocess.run(c,shell=True,check=True,cwd=R,capture_output=True)
sh('sdcc -mz80 --opt-code-size -Iprivate -c -o out/prof/e1m1.rel out/prof/e1m1.c')
sh('sdcc -mz80 --no-std-crt0 --code-loc 0x8000 --data-loc 0xC000 -o out/prof/p.ihx out/crt0.rel out/math.rel out/prof/e1m1.rel')
sh('python3 tools/mkrom.py out/prof/p.ihx private/e1m1_banks.bin out/prof/p.rom')
txt=open(os.path.join(P,'p.map')).read()
sym=sorted((int(m.group(1),16),m.group(2)) for m in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',txt,re.M) if 0x8000<=int(m.group(1),16)<0xC000); addrs=[a for a,_ in sym]
def fn(pc):
    i=bisect.bisect_right(addrs,pc)-1; return sym[i][1] if i>=0 else '?'
M=Machine(lambda row:0xFF,rom=os.path.join(P,'p.rom')); STEP=157; hist=collections.Counter(); t=0; marks=[]
# posicao opcional: x y ang_doom
mem=M.m.memory
while len(M.frames)<2: M.m.ticks_to_stop=STEP; M.m.run()
sy={m.group(2):int(m.group(1),16) for m in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',txt,re.M)}
mem[sy['msg_t']]=0; mem[sy['msg']]=0; mem[sy['msg']+1]=0
if len(sys.argv)>3:
    sy={m.group(2):int(m.group(1),16) for m in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',txt,re.M)}
    for n,v in (('px',int(sys.argv[1])),('py',int(sys.argv[2]))): mem[sy[n]:sy[n]+2]=(v&0xFFFF).to_bytes(2,'little')
    mem[sy['yaw']]=int(((90-float(sys.argv[3]))%360)/360*256)&255
base=len(M.frames); marks=[]; t=0
while len(M.frames)<base+6:
    M.m.ticks_to_stop=STEP; M.m.run(); t+=STEP; hist[fn(M.m.pc)]+=1
    if len(marks)<len(M.frames)-base: marks.append(t)
per=[b-a for a,b in zip(marks,marks[1:])]; avg=sum(per)/len(per)
print('T-states por quadro (so CPU): %.0f -> Z80 3.58 MHz: %.0f ms (%.1f fps teto)'%(avg,avg/3579.545,3579545/avg))
tot=sum(hist.values())
for f,c in hist.most_common(14): print('%5.1f%%  %s'%(100*c/tot,f))
print('LRMM CPU/quadro',M.log.get('lrmm_cpu',0)//len(M.frames),'lmmv',M.log['lmmv']//len(M.frames))
