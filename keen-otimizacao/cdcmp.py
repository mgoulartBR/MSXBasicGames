import sys
def filt(v):
    out=[];i=0
    while i<len(v):
        if i+1<len(v) and v[i] in('2','0') and v[i+1]=='143':i+=2;continue
        out.append(v[i]);i+=1
    return out
d={}
for l in open(sys.argv[1]):
    k,_,rest=l.partition(' ')
    n,ph=k.split(',')
    d.setdefault(int(n),{})[ph]=filt(rest.split())
bad=0;tot=0;cmds=0
for n in sorted(d):
    v=d[n]
    if 'C' in v and 'A' in v:
        tot+=1;cmds+=len(v['C'])
        if v['C']!=v['A']:
            bad+=1
            if bad<=3:
                a,b=v['C'],v['A']
                j=next((i for i in range(min(len(a),len(b))) if a[i]!=b[i]),min(len(a),len(b)))
                print('call',n,'len',len(a),len(b),'first diff at',j,a[max(0,j-6):j+8],b[max(0,j-6):j+8])
print('calls',tot,'mismatch',bad,'bytes compared',cmds)
