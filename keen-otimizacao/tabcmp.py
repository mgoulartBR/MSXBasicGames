import glob,sys
def tables(f):
    v=open(f,'rb').read();rec=open(f[:-5]+'.rec').read().split('\n')
    regs=[l for l in rec if l.startswith('regs')][0].split()[1:]
    r5=int(regs[5]);r6=int(regs[6]);r11=int(regs[11])
    att=((r11&3)<<15)|((r5&0xFC)<<7)
    a=v[att:att+64]
    # key: pattern indices of the first 16 entries + R6
    key=(r6,tuple(a[i*4+2] for i in range(16)))
    col=v[att-0x200:att-0x200+384]
    pat=v[att-0x600:att-0x600+768]
    gp=v[(r6&0x3F)<<11:((r6&0x3F)<<11)+2048]
    return key,col,pat,gp,att
ref={}
for pre in sys.argv[2].split(','):
    for f in glob.glob(pre+'_*.vram'):
        if len(open(f,'rb').read())<0x14000:continue
        k,c,p,g,att=tables(f);ref.setdefault(k,[]).append((f,c,p,g))
print('reference keys',len(ref))
for f in sorted(glob.glob(sys.argv[1]+'_*.vram')):
    k,c,p,g,att=tables(f)
    if k in ref:
        r=ref[k][0]
        dc=[i for i in range(384) if c[i]!=r[1][i]]
        dg=[i for i in range(2048) if g[i]!=r[3][i]]
        dp=[i for i in range(768) if p[i]!=r[2][i]]
        print(f,'R6=%d'%k[0],'color diff',len(dc),dc[:6],'pattern-table(base) diff',len(dp),'group diff',len(dg),'(ref',r[0],')')
    else:print(f,'no reference for key R6=%d'%k[0],k[1][:6])
