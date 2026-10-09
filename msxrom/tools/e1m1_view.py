"""Vista de verificacao do E1M1 pelo modelo de referencia do Geo3D (SIMULATED): uso e1m1_view.py doom1.wad saida.png [x y angulo_graus_doom]."""
import sys,os,math
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0,HERE)
sys.path.insert(0,os.environ.get('GEO3D_SIM','/home/user/alexmoncks/v9968_cartridge/geo3d/sim'))
from gen_scenes import render_faces
import e1m1
from wad import Wad

def quantize(rgb,k=192):
    """paleta unica por corte mediano simples (verificacao); devolve indices 1..k (0 = transparente) e paleta."""
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

def split_strips(d0,d1,maxlen):
    n=max(1,math.ceil((d1-d0)/maxlen)); return [(d0+(d1-d0)*i/n,d0+(d1-d0)*(i+1)/n) for i in range(n)]

def faces_for(pcs,tiles,px,pz,yaw,budget=245,fov=1.0):
    """quads visiveis no frustum, mais proximos primeiro; tiras por distancia. Devolve verts,faces,uvs."""
    cand=[]
    s,c=math.sin(yaw),math.cos(yaw)
    for p in pcs:
        (ax,ay),(bx,by)=p['A'],p['B']; L=math.hypot(bx-ax,by-ay); ux,uy=(bx-ax)/L,(by-ay)/L
        tl=tiles[p['tex']]
        for (d0,d1,hb,ht,u0,u1,r0,r1) in e1m1.split_piece(p):
            mx=ax+ux*(d0+d1)/2-px; mz=ay+uy*(d0+d1)/2-pz
            dist=math.hypot(mx,mz)
            if dist>2600: continue
            # so o lado que olha para o jogador: observador a direita de A->B
            if (px-ax)*(by-ay)-(pz-ay)*(bx-ax)<=0 and False: pass
            cand.append((dist,p,tl,(d0,d1,hb,ht,u0,u1,r0,r1),(ax,ay,ux,uy)))
    cand.sort(key=lambda e:e[0])
    verts=[];vi={};faces=[];uvs=[]
    def V(pt):
        if pt not in vi: vi[pt]=len(verts); verts.append(pt)
        return vi[pt]
    for dist,p,tl,(d0,d1,hb,ht,u0,u1,r0,r1),(ax,ay,ux,uy) in cand:
        # lado: observador precisa estar a direita de A->B
        if (ux*(pz-ay)-uy*(px-ax))>=0: continue
        zc=(px-ax)*0  # placeholder
        # frustum grosseiro: algum extremo na frente
        zs=[((ax+ux*d-px)*s+(ay+uy*d-pz)*c) for d in (d0,d1)]
        if max(zs)<4: continue
        maxlen=16 if dist<128 else 32 if dist<256 else 64 if dist<640 else 128
        for (a,b) in split_strips(d0,d1,maxlen):
            if len(faces)>=budget or len(verts)>251: break
            ua=u0+(u1-u0)*(a-d0)/(d1-d0); ub=u0+(u1-u0)*(b-d0)/(d1-d0)
            xa,za=round(ax+ux*a),round(ay+uy*a); xb,zb=round(ax+ux*b),round(ay+uy*b)
            BR=(xb,hb,zb);BL=(xa,hb,za);TL=(xa,ht,za);TR=(xb,ht,zb)
            tu=lambda u:min(255,max(0,int(tl['x']+u*tl['sx']))) ; tv=lambda r:min(255,max(0,int(tl['y']+r*tl['sy'])))
            u_a=tu(ua) ; u_b=min(255,max(0,int(tl['x']+ub*tl['sx']+0.5)-1)); v0=tv(r0); v1=min(255,max(0,int(tl['y']+r1*tl['sy']+0.5)-1))
            faces.append((V(BR),V(BL),V(TL),V(TR),0,-16384,0,0x80)); uvs.append((u_b,v1,u_a,v1,u_a,v0,u_b,v0))
    return verts,faces,uvs

if __name__=='__main__':
    w=Wad(sys.argv[1]); m=w.load_map('E1M1'); out=sys.argv[2]
    px,pz,ang=(float(sys.argv[3]),float(sys.argv[4]),float(sys.argv[5])) if len(sys.argv)>5 else (1056,-3616,90)
    pcs=e1m1.wall_pieces(w,m); sc,info=e1m1.choose_scales(w,pcs); rgb,alpha,tiles=e1m1.build_atlas(w,sc,info)
    idx,pal=quantize(rgb); idx[~alpha]=0
    vram=np.zeros((1024,256),np.uint8); vram[512:768]=idx
    yaw=math.radians(ang-90)                       # yaw 0 = norte (+y do Doom)
    s,c=math.sin(yaw),math.cos(yaw); q=lambda v:int(round(v*16384))
    M=[q(c),0,q(-s),0,16384,0,q(s),0,q(c)]
    EYE=41+0; TX=-(c*px-s*pz); TZ=-(s*px+c*pz); TY=-(0+EYE)
    verts,faces,uvs=faces_for(pcs,tiles,px,pz,yaw)
    print('faces',len(faces),'verts',len(verts)); assert len(verts)<=255
    cfg=M+[round(TX),round(TY),round(TZ)]+[170,128,89,2,256,178]
    cfg=[cfg[i] for i in range(9)]+[cfg[9],cfg[10],cfg[11]]+[0,0,0]+[170,128,89,2,256,178]
    cfg=cfg[:18]; 
    # cfg layout (9 matriz, 3 translacao, F CX CY ZNEAR W H em 24..) como em z80_harness.run
    cfg=M+[round(TX),round(TY),round(TZ)]+[170,128,89,2,256,178]
    # indices usados pelo modelo: cfg[0:9] matriz, cfg[16],cfg[17] = W,H (ver gen_scenes); reaproveita a ordem do harness
    light=[-5734,11207,-10486]
    tex=dict(on=True,texx=0,texy=512,tstride=64,uv=uvs)
    cmds,skip,draw,cull=render_faces(cfg,verts,faces,8,0,light,tex)
    scr=np.zeros((1024,256),np.uint8); 
    # fundo: teto e piso
    scr[0:89]=250; scr[89:178]=251; pal[250]=(60,60,70); pal[251]=(90,80,70)
    vram[0:178]=scr[0:178]
    for cm in cmds:
        if len(cm)==19: lrmm(vram,cm)
    img=Image.fromarray(pal[vram[:178]],'RGB').resize((768,534),Image.NEAREST); img.save(out); print('draw',draw,'skip',skip,'cull',cull)
