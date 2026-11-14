# uso: pcline2.py <src/out> <modulo> <seg> <arq.pc> [N]  (base do módulo = 0x8000 nos bancos paginados)
import re,bisect,sys
from collections import Counter
out,mod,seg,pcf=sys.argv[1],sys.argv[2],int(sys.argv[3]),sys.argv[4];N=int(sys.argv[5]) if len(sys.argv)>5 else 25
addr=[];info=[];cur=''
for l in open(f'{out}/{mod}.lst').read().split('\n'):
    s=re.search(r';(\S+\.[ch]):(\d+):\s*(.*)',l)
    if s:cur=f'{s[1].split("/")[-1]}:{s[2]} {s[3][:64]}'
    mm=re.match(r'\s+([0-9A-F]{8})\s+[0-9A-F]{2}',l)
    if mm:addr.append(int(mm[1],16));info.append(cur)
p=sorted(zip(addr,info),key=lambda t:t[0]);A=[x[0] for x in p];I=[x[1] for x in p]
c=Counter();tot=0;allv=0
for line in open(pcf):
    pc,sg=map(int,line.split(','));allv+=1
    if sg!=seg or pc<0x8000:continue
    tot+=1;c[I[max(0,bisect.bisect_right(A,pc-0x8000)-1)]]+=1
for k,v in c.most_common(N):print('%5.1f%% %s'%(100*v/allv,k))
print('seg samples',tot,'of',allv)
