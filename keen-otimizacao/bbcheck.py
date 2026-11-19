import sys,re,json,random
d=json.load(open('../vb/assets/boobus/combat_assets.json'))['sprites']
def parse(line):
    return [tuple(map(int,m.split())) for m in re.findall(r'\{([^}]*)\}',line)]
def build(frame,page,ty,vram):
    w,h=d[frame]['crop'][2],d[frame]['crop'][3]
    raw=open(f'../vb/assets/boobus/boss_{frame-108}.bin','rb').read()
    for r in range(h):
        row=(page*256+((ty+r%52)&255));base=(64 if r>=52 else 0)
        vram[row*128+base:row*128+base+w]=raw[r*w:(r+1)*w]
def run(cmds,vram):
    for c in cmds:
        sx=c[0]|c[1]<<8;sy=c[2]|c[3]<<8;dx=c[4]|c[5]<<8;dy=c[6]|c[7]<<8;nx=c[8]|c[9]<<8;ny=c[10]|c[11]<<8;arg=c[13];cmd=c[14]
        assert arg==0,arg
        op=cmd>>4
        def get(x,y):
            b=vram[y*128+x//2];return b>>4 if x%2==0 else b&15
        def put(x,y,v):
            i=y*128+x//2
            vram[i]=(vram[i]&0x0F)|(v<<4) if x%2==0 else (vram[i]&0xF0)|v
        if op==0xD:
            assert sx%2==0 and dx%2==0 and nx%2==0,(sx,dx,nx)
            for y in range(ny):
                for k in range(nx//2):vram[(dy+y)*128+dx//2+k]=vram[(sy+y)*128+sx//2+k]
        elif op==9:
            lo=cmd&15
            for y in range(ny):
                for x in range(nx):
                    s=get(sx+x,sy+y);t=get(dx+x,dy+y)
                    assert lo in (1,2),lo
                    put(dx+x,dy+y,(t&s) if lo==1 else (t|s))
        else:raise Exception(cmd)
n=bad=fast=0;random.seed(1)
L=open(sys.argv[1]).read().split('\n');i=0
while i<len(L):
    if L[i].startswith('CALL'):
        t=L[i].split();frame=int(t[6]);campy=int(t[8]);ty=(campy+192)&255
        old=parse(L[i+1]);new=parse(L[i+2]);i+=3
        if not old:continue
        if old[0][2]|old[0][3]<<8>=512:continue   # streamed path
        page=(old[0][6]|old[0][7]<<8)>>8
        A=bytearray(random.randrange(256) for _ in range(65536));B=bytearray(A)
        build(frame,page,ty,A);build(frame,page,ty,B)
        run(old,A);run(new,B)
        n+=1
        if any(c[14]>>4==0xD for c in new):fast+=1
        if A!=B:bad+=1;print('MISMATCH frame',frame,'page',page,'old',len(old),'new',len(new))
    else:i+=1
print('calls',n,'with HMMM',fast,'mismatches',bad)
