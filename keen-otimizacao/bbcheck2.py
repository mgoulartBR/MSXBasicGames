import sys,re,json,random
d=json.load(open('../vb/assets/boobus/combat_assets.json'))['sprites']
def old_px(frame):
    w,h=d[frame]['crop'][2],d[frame]['crop'][3];raw=open(f'../vb95/assets/boobus/boss_{frame-108}.bin','rb').read()
    M=[[0]*w for _ in range(h)];C=[[0]*w for _ in range(h)]
    for y in range(h):
        row=raw[y*w:(y+1)*w]
        for x in range(w):
            b=row[x//2];M[y][x]=(b>>4) if x%2==0 else b&15
            b=row[w//2+x//2];C[y][x]=(b>>4) if x%2==0 else b&15
    return w,h,M,C
def build_new(frame,page,ty,vram):
    w,h=d[frame]['crop'][2],d[frame]['crop'][3];raw=open(f'../vb/assets/boobus/boss_{frame-108}.bin','rb').read();wb=w//2
    for r in range(h):
        row=page*256+((ty+r%52)&255);base=64 if r>=52 else 0
        vram[row*128+base:row*128+base+wb]=raw[r*wb:(r+1)*wb]
def get(v,x,y):
    b=v[y*128+x//2];return b>>4 if x%2==0 else b&15
def put(v,x,y,val):
    i=y*128+x//2
    v[i]=(v[i]&0x0F)|(val<<4) if x%2==0 else (v[i]&0xF0)|val
def parse(line):return [list(map(int,m.split()[:15])) for m in re.findall(r'\{([^}]*)\}',line)]
random.seed(3);n=bad=pix=0
L=open(sys.argv[1]).read().split('\n');i=0
while i<len(L):
    if L[i].startswith('CALL'):
        t=L[i].split();frame=int(t[6]);campy=int(t[8]);ty=(campy+192)&255;cmds=parse(L[i+2]);i+=3
        if not cmds or (cmds[0][2]|cmds[0][3]<<8)>=512:continue
        page=((cmds[0][6]|cmds[0][7]<<8))>>8
        w,h,M,C=old_px(frame)
        V=bytearray(random.randrange(256) for _ in range(65536));ref=bytearray(V)
        build_new(frame,page,ty,V)
        # new: apply TIMP commands
        for c in cmds:
            sx=c[0]|c[1]<<8;sy=c[2]|c[3]<<8;dx=c[4]|c[5]<<8;dy=c[6]|c[7]<<8;nx=c[8]|c[9]<<8;ny=c[10]|c[11]<<8;assert c[14]==0x98,c[14]
            half=1 if sx>=128 else 0
            for r in range(ny):
                for k in range(nx):
                    s=get(V,sx+k,sy+r)
                    if s:put(V,dx+k,dy+r,s)
                    # reference with OLD formula
                    col=sx-128*half+k;rtex=((sy+r-page*256)-ty)%256;assert rtex<52,(rtex)
                    row=rtex+52*half
                    m=M[row][col];cc=C[row][col];dv=get(ref,dx+k,dy+r)
                    put(ref,dx+k,dy+r,((dv&m)|cc));pix+=1
        # compare only destination pixels (the texture area gets identical writes? no: ref not rebuilt) -> compare dest rects
        okc=True
        for c in cmds:
            dx=c[4]|c[5]<<8;dy=c[6]|c[7]<<8;nx=c[8]|c[9]<<8;ny=c[10]|c[11]<<8
            for r in range(ny):
                for k in range(nx):
                    a=get(V,dx+k,dy+r);b=get(ref,dx+k,dy+r)
                    if (9 if a==9 else a)!=b and not (a==9 and b==0):okc=False
        n+=1
        if not okc:bad+=1;print('MISMATCH frame',frame)
    else:i+=1
print('calls',n,'pixels',pix,'mismatches',bad)
