# uso: pccall.py <src/out> <arq.pc> <funcao>  — quem chama <funcao> quando a amostra de PC está dentro dela
import sys,re,bisect
from collections import Counter
out,pcf,fn=sys.argv[1],sys.argv[2],sys.argv[3]
m=open(f'{out}/keen_msx.map').read()
import glob,os
glob_abs={}
for a,n,mod in re.findall(r'^\s+([0-9A-F]{8})\s+(_\w+)\s+(\S+)\s*$',m,re.M):glob_abs[(mod,n)]=int(a,16)
syms=[]
for f in glob.glob(f'{out}/*.sym'):
    mod=os.path.basename(f)[:-4];ent=[(m_[2],int(m_[3],16)) for m_ in re.finditer(r'^\s+(\d+)\s+(_\w+)\s+([0-9A-F]{8}) [A-Z]*R',open(f).read(),re.M)]
    base=None
    for n,rel in ent:
        if (mod,n) in glob_abs and not re.search(r'_\d{4,}_\d+$',n):base=glob_abs[(mod,n)]-rel;break
    if base is None:continue
    for n,rel in ent:
        if not re.search(r'_\d{4,}_\d+$',n) and not n.startswith('_g_'):syms.append((base+rel,n))
syms.sort();A=[a for a,_ in syms]
def name(seg,pc):
    seg=0 if pc<0x8000 else seg
    i=bisect.bisect_right(A,(seg<<16)|pc)-1
    return syms[i][1] if i>=0 and (syms[i][0]>>16)==seg else '?'
tgt=[a for a,n in syms if n=='_'+fn][0]
nxt=min([a for a,n in syms if a>tgt]+[tgt+0x80])
c=Counter();tot=0
for l in open(pcf):
    p=l.strip().split(',')
    if len(p)<3:continue
    pc,seg,ret=int(p[0]),int(p[1]),int(p[2])
    k=(0 if pc<0x8000 else seg)<<16|pc
    if tgt<=k<nxt:
        tot+=1;c[name(seg,ret)]+=1   # chamador: mesmo banco paginado (aproximação)
for n,v in c.most_common(14):print('%5.1f%%  %s'%(100*v/max(tot,1),n))
print('amostras em',fn,tot)
