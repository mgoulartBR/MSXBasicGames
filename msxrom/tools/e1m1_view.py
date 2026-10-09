"""Vista de verificacao do E1M1 pelo modelo de referencia do Geo3D (SIMULATED), usando o PVS do bloco do jogador.
uso: e1m1_view.py doom1.wad saida.png [x y angulo_doom ...]  (varios pontos: x y ang x y ang ... -> folha de contato)"""
import sys,os,math
import numpy as np
from PIL import Image,ImageDraw
HERE=os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0,HERE)
sys.path.insert(0,os.environ.get('GEO3D_SIM','/home/user/alexmoncks/v9968_cartridge/geo3d/sim'))
from gen_scenes import render_faces
import e1m1,e1m1_pvs as P

def quantize(rgb,k=192):
    px=rgb.reshape(-1,3).astype(np.float32); u,inv,cnt=np.unique(px,axis=0,return_inverse=True,return_counts=True)
    boxes=[np.arange(len(u))]
    while len(boxes)<k:
        i=max(range(len(boxes)),key=lambda j:(len(boxes[j])>1)*(np.ptp(u[boxes[j]],0).max()*cnt[boxes[j]].sum()))
        b=boxes.pop(i)
        if len(b)<2: boxes.append(b); break
        ch=int(np.argmax(np.ptp(u[b],0))); o=b[np.argsort(u[b][:,ch])]; cw=np.cumsum(cnt[o]); h=min(max(int(np.searchsorted(cw,cw[-1]/2))+1,1),len(o)-1)
        boxes+=[o[:h],o[h:]]
    pal=np.zeros((256,3),np.uint8); lab=np.zeros(len(u),np.int32)
    for n,b in enumerate(boxes): pal[n+1]=np.round((u[b]*cnt[b,None]).sum(0)/cnt[b].sum()); lab[b]=n+1
    return lab[inv].reshape(rgb.shape[:2]).astype(np.uint8),pal

def lrmm(vram,c):
    sx=c[0]|(c[1]&15)<<8; sy=c[2]|(c[3]&31)<<8; dx=c[4]|(c[5]&1)<<8; dy=(c[6]|c[7]<<8)&0x3FF; nx=c[8]|(c[9]&7)<<8; col=c[12]; lop=c[18]&15
    du=c[14]|c[15]<<8; dv=c[16]|c[17]<<8; du-=65536*(du>>15); dv-=65536*(dv>>15)
    for i in range(nx):
        x=((sx<<8)+i*du)>>8; y=((sy<<8)+i*dv)>>8; d=dx+i
        if d>255: break
        src=int(vram[(y&1023),(x&255)]) if (0<=x<=255 and 0<=y<=1023) else col
        if lop&8 and src==0: continue
        vram[dy,d]=src

def floor_at(L,x,y):
    R=L.R; cx,cy=int((x-R.x0)//8),int((y-R.y0)//8); s=R.sec[cy,cx]
    return L.m.sectors[s]['floor'] if s>=0 else 0

def render(L,vram,pal,px,py,ang_doom,cache={}):
    blk=(int(px//P.BLOCK),int(py//P.BLOCK))
    if blk not in cache: cache[blk]=P.build_block(L,*blk)
    b=cache[blk]
    yaw=math.radians(ang_doom-90); s,c=math.sin(yaw),math.cos(yaw); q=lambda v:int(round(v*16384))
    M=[q(c),0,q(-s),0,16384,0,q(s),0,q(c)]; eye=floor_at(L,px,py)+41
    TX=-(c*px-s*py); TZ=-(s*px+c*py); TY=-eye
    cfg=M+[round(TX),round(TY),round(TZ)]+[170,128,89,2,256,178]
    verts=b['verts']; faces=[tuple(f)+(0,-16384,0,0x80) for f in b['faces']]
    tex=dict(on=True,texx=0,texy=512,tstride=64,uv=b['uvs'])
    cmds,skip,draw,cull=render_faces(cfg,verts,faces,8,0,[-5734,11207,-10486],tex)
    vram[0:89]=250; vram[89:178]=251
    for cm in cmds:
        if len(cm)==19: lrmm(vram,cm)
    return b,skip,draw

if __name__=='__main__':
    L=P.load(sys.argv[1]); out=sys.argv[2]
    pts=[float(v) for v in sys.argv[3:]] or [1056,-3616,90]
    idx,pal=quantize(L.rgb); idx[~L.alpha]=0
    pal[250]=(60,60,70); pal[251]=(90,80,70)
    ims=[]
    for i in range(0,len(pts),3):
        vram=np.zeros((1024,256),np.uint8); vram[512:768]=idx
        b,skip,draw=render(L,vram,pal,pts[i],pts[i+1],pts[i+2])
        print(pts[i:i+3],'faces',len(b['faces']),'verts',len(b['verts']),'mantido',round(b['kept'],3),'skip',skip,'draw',draw)
        ims.append(Image.fromarray(pal[vram[:178]],'RGB'))
    cols=min(3,len(ims)); rows=(len(ims)+cols-1)//cols; S=Image.new('RGB',(256*cols,178*rows))
    for i,im in enumerate(ims): S.paste(im,((i%cols)*256,(i//cols)*178))
    S.resize((S.width*3//(1 if cols==1 else 2),S.height*3//(1 if cols==1 else 2)),Image.NEAREST).save(out)
