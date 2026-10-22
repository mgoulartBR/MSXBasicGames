import json, numpy as np, librosa
rows=json.load(open('loop.json')); N=len(rows)
nm=lambda m: librosa.midi_to_note(m).replace('♯','#')
def clean(r):
    r=sorted(r,key=lambda x:-x[1])
    if not r: return []
    mx=r[0][1]; out=[]
    for m,e in r:
        if e<0.30*mx: continue
        if any(0<abs(m-m2)<=3 and e<0.6*e2 for m2,e2 in out): continue
        out.append((m,e))
    return out
A=[1]*N;B=[1]*N;Cc=[1]*N
for i,r in enumerate(rows):
    r=clean(r); ns=sorted(r)
    if not ns: continue
    hi=[x for x in ns if x[0]>=60]; lo=[x for x in ns if x[0]<=57]; mid=[x for x in ns if 57<x[0]<60]
    lead=None; bass=None; harm=None
    if hi:
        hs=sorted(hi,key=lambda x:-x[1]); lead=hs[0]
        if len(hs)>1 and hs[1][1]>=0.6*hs[0][1]: harm=hs[1]
    if lo:
        bass=min(lo,key=lambda x:x[0])
        rest=[x for x in lo if x is not bass]
        if rest and harm is None: harm=max(rest,key=lambda x:x[1])
    if mid:
        if lead is None: lead=mid[0]
        elif harm is None: harm=mid[0]
    if lead: A[i]=lead[0]
    if bass: B[i]=bass[0]
    if harm: Cc[i]=harm[0]
def fillgaps(ch):
    for i in range(1,N-1):
        if ch[i]==1 and ch[i-1]>=12 and ch[i-1]==ch[i+1]: ch[i]=ch[i-1]
    return ch
for ch in (A,B,Cc): fillgaps(ch)
json.dump({'A':A,'B':B,'C':Cc},open('theme_notes.json','w'))
for bar in range(12):
    print('--- bar',bar+1)
    for name,ch in (('A',A),('B',B),('C',Cc)):
        print(name,' '.join(('--' if ch[i]==1 else nm(ch[i])).ljust(3) for i in range(bar*16,min(N,bar*16+16))))
