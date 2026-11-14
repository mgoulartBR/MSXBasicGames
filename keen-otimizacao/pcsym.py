# uso: pcsym.py <mapa> <arquivo.pc> [N]
import sys,re,bisect
from collections import Counter
m=open(sys.argv[1]).read()
syms=sorted((int(a,16),n) for a,n in re.findall(r'^\s+([0-9A-F]{8})\s+_(\w+)\s+',m,re.M))
addrs=[a for a,_ in syms]
c=Counter();tot=0
for line in open(sys.argv[2]):
    line=line.strip()
    if not line:continue
    pc,seg=map(int,line.split(','));tot+=1
    seg=0 if pc<0x8000 else seg
    key=(seg<<16)|pc
    i=bisect.bisect_right(addrs,key)-1
    name=syms[i][1] if i>=0 and (syms[i][0]>>16)==seg else '?%02X:%04X'%(seg,pc)
    c[name]+=1
for n,k in c.most_common(int(sys.argv[3]) if len(sys.argv)>3 else 25):print('%5.1f%%  %s'%(100*k/tot,n))
print('samples',tot)
