import struct
from PIL import Image
def load(prefix):
    g=lambda e:open(prefix+'.'+e,'rb').read()
    v=bytearray(g('vram')); v[0::2],v[1::2]=g('vram')[1::2],g('vram')[0::2]   # core stores host-endian words
    return bytes(v),g('cram'),g('vsram'),g('reg')
def palette(cram):
    pal=[]
    for i in range(64):
        c=struct.unpack('>H',cram[i*2:i*2+2])[0] if False else (cram[i*2]|(cram[i*2+1]<<8))  # core stores host-order? try both below
        pal.append(c)
    return pal
def cram_colors(cram,order):
    out=[]
    for i in range(64):
        c=(cram[i*2]<<8|cram[i*2+1]) if order=='be' else (cram[i*2]|cram[i*2+1]<<8)
        out.append(((c&7)*255//7,((c>>3)&7)*255//7,((c>>6)&7)*255//7))
    return out
def tile_px(vram,idx,hf,vf):
    o=idx*32; rows=[]
    for y in range(8):
        yy=7-y if vf else y
        row=[]
        for x in range(4):
            b=vram[o+yy*4+x]; row+= [b>>4,b&15]
        rows.append(row[::-1] if hf else row)
    return rows
def plane(vram,cols,base,w,h,order):
    im=Image.new('RGBA',(w*8,h*8),(0,0,0,0)); px=im.load()
    for ty in range(h):
        for tx in range(w):
            e=(vram[base+(ty*w+tx)*2]<<8)|vram[base+(ty*w+tx)*2+1]
            idx=e&0x7ff; hf=(e>>11)&1; vf=(e>>12)&1; pl=(e>>13)&3; pr=e>>15
            t=tile_px(vram,idx,hf,vf)
            for y in range(8):
                for x in range(8):
                    c=t[y][x]
                    if c: px[tx*8+x,ty*8+y]=cols[pl*16+c]+(255,)
    return im
def sprites(vram,cols,sp):
    out=[]; i=0
    for _ in range(80):
        o=sp+i*8
        y=((vram[o]<<8|vram[o+1])&0x3ff)-128
        sz=vram[o+2]; link=vram[o+3]&0x7f
        e=(vram[o+4]<<8)|vram[o+5]; x=((vram[o+6]<<8|vram[o+7])&0x1ff)-128
        out.append((y,x,sz,e)); i=link
        if i==0: break
    return out

def compose(vram,cram,vsram,reg,with_sprites=True,layers=('B','A','S')):
    cols=cram_colors(cram,'le')
    pa=(reg[2]&0x38)<<10; pb=(reg[4]&7)<<13; sp=(reg[5]&0x7f)<<9; hs=(reg[13]&0x3f)<<10
    A=plane(vram,cols,pa,64,64,'le'); B=plane(vram,cols,pb,64,64,'le')
    hA=(vram[hs]<<8|vram[hs+1])&0x3ff; hB=(vram[hs+2]<<8|vram[hs+3])&0x3ff
    vA=(vsram[0]|vsram[1]<<8)&0x3ff; vB=(vsram[2]|vsram[3]<<8)&0x3ff
    out=Image.new('RGBA',(320,224),(0,0,0,255))
    def blit(P,h,v):
        pxs=P.load(); o=out.load()
        for y in range(224):
            for x in range(320):
                c=pxs[(x-h)%512,(y+v)%512]
                if c[3]: o[x,y]=c
    if 'B' in layers: blit(B,hB,vB)
    if 'A' in layers: blit(A,hA,vA)
    if 'S' in layers:
        o=out.load()
        for (y,x,sz,e) in sprites(vram,cols,sp):
            w=((sz>>2)&3)+1; h=(sz&3)+1; idx=e&0x7ff; hf=(e>>11)&1; vf=(e>>12)&1; pl=(e>>13)&3
            for cx in range(w):
                for cy in range(h):
                    tcx=(w-1-cx) if hf else cx; tcy=(h-1-cy) if vf else cy
                    t=tile_px(vram,idx+tcx*h+tcy,hf,vf)
                    for yy in range(8):
                        for xx in range(8):
                            c=t[yy][xx]; X=x+cx*8+xx; Y=y+cy*8+yy
                            if c and 0<=X<320 and 0<=Y<224: o[X,Y]=cols[pl*16+c]+(255,)
    return out
