"""Valida src/math.s (mulq14 em assembly) contra Python em 600 pares, rodando no Z80 emulado."""
import os,random,re,subprocess,sys
from z80 import Z80Machine
R=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..'); O=os.path.join(R,'out','t_mul'); os.makedirs(O,exist_ok=True)
random.seed(1)
pairs=[(-16384,151),(16384,-151),(-16384,-467),(0,5),(16384,16384),(-16384,1023),(1,1),(-1,1),(16383,2047),(-2047,16384)]
pairs+=[(random.choice([-16384,16384,random.randint(-16384,16384)]),random.randint(-2047,2047)) for _ in range(590)]
n=len(pairs)
open(os.path.join(O,'t.c'),'w').write("typedef int s16;typedef unsigned int u16;\ns16 mulq14(s16 a,s16 b) __sdcccall(1);\n"
 f"const s16 A[{n}]={{{','.join(str(a) for a,b in pairs)}}};const s16 B[{n}]={{{','.join(str(b) for a,b in pairs)}}};s16 RES[{n}];\n"
 f"void main(void){{u16 i;for(i=0;i<{n};i++)RES[i]=mulq14(A[i],B[i]);for(;;);}}\n")
sh=lambda c:subprocess.run(c,shell=True,check=True,capture_output=True,cwd=O)
sh(f"sdasz80 -plosgff -o math.rel {R}/src/math.s && sdasz80 -plosgff -o crt.rel {R}/tools/crt_min.s && sdcc -mz80 -c t.c && sdcc -mz80 --no-std-crt0 --code-loc 0x4010 --data-loc 0xC000 -o t.ihx crt.rel math.rel t.rel && makebin -s 32768 t.ihx t.bin")
m=Z80Machine(); d=open(os.path.join(O,'t.bin'),'rb').read(); m.memory[0x4000:0x8000]=d[0x4000:0x8000]; m.pc=0x4000; m.sp=0xF380
for _ in range(400): m.ticks_to_stop=2000000; m.run()
sym={mm.group(2):int(mm.group(1),16) for mm in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',open(os.path.join(O,'t.map')).read(),re.M)}
bad=0
for i,(a,b) in enumerate(pairs):
    v=int.from_bytes(bytes(m.memory[sym['RES']+2*i:sym['RES']+2*i+2]),'little'); v=v-65536 if v>32767 else v
    exp=int(a*b/16384)
    if v!=exp:
        bad+=1
        if bad<5:print('ERRO',a,b,v,exp)
print('pares',n,'divergências',bad); sys.exit(1 if bad else 0)
