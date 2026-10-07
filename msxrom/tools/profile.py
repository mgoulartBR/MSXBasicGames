"""Perfil de CPU da ROM no Z80 emulado: T-states por quadro e distribuicao por funcao (amostragem de PC)."""
import re,sys,os,collections,bisect
sys.path.insert(0,os.path.dirname(__file__))
from z80_harness import Machine
R=os.path.join(os.path.dirname(__file__),'..')
import subprocess,re as _re
# build de perfil: remove 'static' das funções de nível superior para que apareçam no mapa de símbolos
os.makedirs(os.path.join(R,'out','prof'),exist_ok=True)
src=open(os.path.join(R,'src','main.c')).read()
src=_re.sub(r'^static ','',src,flags=_re.M)
open(os.path.join(R,'out','prof','main.c'),'w').write(src)
def sh(c): subprocess.run(c,shell=True,check=True,cwd=R,capture_output=True)
sh('sdcc -mz80 --opt-code-size -Isrc -c -o out/prof/main.rel out/prof/main.c')
sh('sdcc -mz80 --no-std-crt0 --code-loc 0x8000 --data-loc 0xC000 -o out/prof/p.ihx out/crt0.rel out/math.rel out/prof/main.rel')
sh('python3 tools/mkrom.py out/prof/p.ihx out/data_banks.bin out/prof/p.rom')
sym=sorted((int(m.group(1),16),m.group(2)) for m in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',open(os.path.join(R,'out','prof','p.map')).read(),re.M) if int(m.group(1),16)>=0x8000 and int(m.group(1),16)<0xC000)
addrs=[a for a,_ in sym]
def fn(pc):
    i=bisect.bisect_right(addrs,pc)-1; return sym[i][1] if i>=0 else '?'
M=Machine(lambda row:0xFF,rom=os.path.join(R,'out','prof','p.rom')); STEP=157
hist=collections.Counter(); t=0; marks=[]
while len(M.frames)<7:
    M.m.ticks_to_stop=STEP; M.m.run(); t+=STEP
    if len(M.frames)>=3: hist[fn(M.m.pc)]+=1
    if len(marks)<len(M.frames): marks.append(t)
per=[b-a for a,b in zip(marks,marks[1:])][2:]    # ignora boot
avg=sum(per)/len(per)
print('T-states por quadro (CPU, sem esperas de VDP/Geo3D): %.0f  -> %.1f ms em Z80 3.58 MHz = %.1f fps (teto da CPU)'%(avg,avg/3579.545,3579545/avg))
tot=sum(hist.values())
for f,c in hist.most_common(12): print('%5.1f%%  %s'%(100*c/tot,f))
print('bytes enviados ao Geo3D por quadro:',M.log['geo_bytes']//len(M.frames))
