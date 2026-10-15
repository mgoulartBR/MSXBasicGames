import re,sys,bisect,collections
v=sys.argv[1]
syms=[]
for l in open(f'build/{v}.map'):
    m=re.match(r'\s+([0-9A-F]{8})\s+(_\w+)',l)
    if m and int(m.group(1),16)<0x10000: syms.append((int(m.group(1),16),m.group(2)))
syms.sort(); addrs=[s[0] for s in syms]
c=collections.Counter();n=0
for l in open(f'/tmp/rt_prof_{v}.txt'):
    if not l.strip(): continue
    a=int(l,16); i=bisect.bisect_right(addrs,a)-1; n+=1; c[syms[i][1]]+=1
idle=c['_Ui_Frame']
print(v,sys.argv[2],'busy %.0f%%'%(100*(n-idle)/n),[(k,round(100*x/n)) for k,x in c.most_common(6)])
# longest run of consecutive non-idle samples (each ~13.7 ms) = worst frame stall
run=best=0
for l in open(f'/tmp/rt_prof_{v}.txt'):
    if not l.strip(): continue
    a=int(l,16); i=bisect.bisect_right(addrs,a)-1
    if syms[i][1]=='_Ui_Frame': run=0
    else:
        run+=1; best=max(best,run)
print('   worst stall ~%.0f ms'%(best*13.7))
