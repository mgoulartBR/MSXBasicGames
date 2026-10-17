import re,sys,collections
syms=[]
for l in open('road.map'):
    m=re.match(r'\s+([0-9A-F]{8})\s+(\S+)\s+(\S+)',l)
    if m: syms.append((int(m.group(1),16),m.group(2)))
syms.sort()
# so simbolos ROM
rom=[(a,n) for a,n in syms if 0x4000<=a<0x8000]
cnt=collections.Counter()
pcs=[int(x) for x in open(sys.argv[1]).read().split()]
for pc in pcs:
    if pc<0x4000: cnt['BIOS(<0x4000)']+=1; continue
    if pc>=0x8000: cnt['RAM/other']+=1; continue
    name='?'
    for a,n in rom:
        if a<=pc: name=n
        else: break
    cnt[name]+=1
tot=sum(cnt.values())
for n,c in cnt.most_common(14): print('%5.1f%%  %s'%(100*c/tot,n))
