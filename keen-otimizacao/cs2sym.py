import sys,re,bisect,glob,os
from collections import defaultdict
out=sys.argv[1];L=sys.argv[2]
glob_abs={}
for a,n,mod in re.findall(r'^\s+([0-9A-F]{8})\s+(_\w+)\s+(\S+)\s*$',open(f'{out}/keen_msx.map').read(),re.M):glob_abs[(mod,n)]=int(a,16)
syms=[]
for f in glob.glob(f'{out}/*.sym'):
    mod=os.path.basename(f)[:-4];ent=[]
    for m in re.finditer(r'^\s+(\d+) (_\w+)\s+([0-9A-F]{8}) [A-Z]*R',open(f).read(),re.M):ent.append((int(m[1]),m[2],int(m[3],16)))
    base=None
    for ar,n,rel in ent:
        if (mod,n) in glob_abs and not re.search(r'_\d{4,}_\d+$',n):base=glob_abs[(mod,n)]-rel;break
    if base is None:continue
    for ar,n,rel in ent:
        if re.search(r'_\d{4,}_\d+$',n) or n.startswith('_g_'):continue
        syms.append((base+rel,n))
syms.sort();addrs=[a for a,_ in syms]
names={'91':'AND','92':'OR','98':'TIMP','d0':'HMMM','c0':'HMMV','90':'IMP'}
agg=defaultdict(lambda:[0,0.0]);tot=0.0
for l in open(f'cst_{L}.txt.cs'):
    t=l.split()
    if t[0]!='C2':continue
    cmd,ret,seg,n,dt=t[1],int(t[2]),int(t[3]),int(t[4]),float(t[5])
    seg=0 if ret<0x8000 else seg
    i=bisect.bisect_right(addrs,(seg<<16)|ret)-1
    fn=syms[i][1] if i>=0 and (syms[i][0]>>16)==seg else '?%02X:%04X'%(seg,ret)
    a=agg[(names.get(cmd,cmd),fn)];a[0]+=n;a[1]+=dt;tot+=dt
print(f'L{L} total interval time {tot:.1f}s')
for (c,fn),(n,dt) in sorted(agg.items(),key=lambda x:-x[1][1])[:int(sys.argv[3]) if len(sys.argv)>3 else 8]:
    print(f'  {100*dt/tot:4.0f}%  {c:5s} {fn:28s} n={n}')
