# uso: pcsym2.py <projeto/src/out> <arquivo.pc> [N]  — resolve também funções static via os .sym de cada módulo
import sys,re,bisect,glob,os
from collections import Counter
out=sys.argv[1]
glob_abs={}   # (module,name)->abs
for a,n,mod in re.findall(r'^\s+([0-9A-F]{8})\s+(_\w+)\s+(\S+)\s*$',open(f'{out}/keen_msx.map').read(),re.M):glob_abs[(mod,n)]=int(a,16)
syms=[]
for f in glob.glob(f'{out}/*.sym'):
    mod=os.path.basename(f)[:-4];ent=[]
    for m in re.finditer(r'^\s+(\d+) (_\w+)\s+([0-9A-F]{8}) [A-Z]*R',open(f).read(),re.M):ent.append((int(m[1]),m[2],int(m[3],16)))
    base=None
    for ar,n,rel in ent:
        if (mod,n) in glob_abs and not re.search(r'_\d{4,}_\d+$',n):
            base=glob_abs[(mod,n)]-rel;break
    if base is None:continue
    for ar,n,rel in ent:
        if re.search(r'_\d{4,}_\d+$',n) or n.startswith('_g_') :continue
        syms.append((base+rel,n))
syms.sort();addrs=[a for a,_ in syms]
c=Counter();tot=0
for line in open(sys.argv[2]):
    line=line.strip()
    if not line:continue
    pc,seg=map(int,line.split(','));tot+=1
    seg=0 if pc<0x8000 else seg
    key=(seg<<16)|pc
    i=bisect.bisect_right(addrs,key)-1
    c[syms[i][1] if i>=0 and (syms[i][0]>>16)==seg else '?%02X:%04X'%(seg,pc)]+=1
for n,k in c.most_common(int(sys.argv[3]) if len(sys.argv)>3 else 30):print('%5.1f%%  %s'%(100*k/tot,n))
print('samples',tot)
