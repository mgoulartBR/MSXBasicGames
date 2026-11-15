import glob,sys
pre=sys.argv[1];bad=n=0
for f in sorted(glob.glob(pre+'_*.vram')):
    v=open(f,'rb').read();rec=open(f[:-5]+'.rec').read().split('\n')
    regs=[l for l in rec if l.startswith('regs')][0].split()[1:]
    r5=int(regs[5]);r11=int(regs[11]);att=((r11&3)<<15)|((r5&0xFC)<<7)
    a=list(v[att:att+260]);end=a.index(216) if 216 in a[:4*24:4] else None
    # entries up to the terminator (y=216) must be (y,x,pat%4==0,0)
    cnt=0;ok=True
    for i in range(24):
        if a[i*4]==216:break
        if a[i*4+3]!=0 or a[i*4+2]%4!=0:ok=False
        cnt+=1
    n+=1;bad+=0 if ok else 1
print(pre,'dumps',n,'corrupt attribute tables',bad)
