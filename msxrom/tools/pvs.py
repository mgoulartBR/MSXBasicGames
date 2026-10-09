"""Conjunto visivel (PVS) por celula do mapa, para o Geo3D.

Em vez de reenviar a geometria a cada quadro, a ROM envia UMA vez, ao entrar numa celula, todas as paredes que podem ser
vistas dali (<=246 faces e <=219 vertices; o resto do limite de 255 fica para os inimigos). Por quadro so vao a matriz e o RUN.
Isso e a regra 1 do estudo do Wolf3D: so o que muda a cada quadro entra no caminho do quadro.

Visibilidade em 2D (paredes de altura total, camera na altura fixa): uma parede (face exposta) entra no conjunto da celula se,
de algum ponto de vista dentro da celula (grade 3x3), algum ponto da parede (3 amostras) esta do lado de fora dela
(produto escalar com a normal > 0) e o segmento entre os dois nao atravessa nenhuma celula solida."""
import math,struct
import level
T=level.T; H=level.H; MAP=level.MAP; N=len(MAP)
MAX_FACES=246; MAX_VERTS=219
solid=level.solid
def tile_of(ch,x,z): return 3 if ch=='D' else 2 if ch=='L' else [0,0,1,2][(x*7+z*3)%4]

def wall_quads():
    quads=[]
    for z in range(N):
        for x in range(N):
            ch=MAP[z][x]
            if ch not in '#LD': continue
            x0,x1,z0,z1=x*T,x*T+T,z*T,z*T+T; tile=tile_of(ch,x,z)
            sides=[(not solid(x,z-1),((x0,z0),(x1,z0)),(0,-1)),(not solid(x,z+1),((x1,z1),(x0,z1)),(0,1)),
                   (not solid(x-1,z),((x0,z1),(x0,z0)),(-1,0)),(not solid(x+1,z),((x1,z0),(x1,z1)),(1,0))]
            for ok,(l,r),nrm in sides:
                if ok: quads.append(dict(l=l,r=r,n=nrm,tile=tile,c=((l[0]+r[0])/2,(l[1]+r[1])/2)))
    return quads

def clear(x0,z0,x1,z1):
    steps=int(max(abs(x1-x0),abs(z1-z0))/6)+1
    for i in range(1,steps):
        x=x0+(x1-x0)*i/steps; z=z0+(z1-z0)*i/steps
        if solid(int(x)//T,int(z)//T): return False
    return True

def visible(cell,q):
    cx,cz=cell; nx,nz=q['n']
    vps=[(cx*T+32+dx,cz*T+32+dz) for dx in (-30,0,30) for dz in (-30,0,30)]
    for t in (0.1,0.5,0.9):
        tx=q['l'][0]+(q['r'][0]-q['l'][0])*t+nx; tz=q['l'][1]+(q['r'][1]-q['l'][1])*t+nz       # 1 unidade para fora
        for vx,vz in vps:
            if (vx-tx)*nx+(vz-tz)*nz<=0: continue
            if clear(vx,vz,tx,tz): return True
    return False

def model(qs):
    """qs: lista de (quad, tiras). Devolve vertices e faces (indices, normal Q14, uv)."""
    verts=[];vi={};faces=[]
    def V(p):
        if p not in vi: vi[p]=len(verts); verts.append(p)
        return vi[p]
    for q,S in qs:
        (lx,lz),(rx,rz)=q['l'],q['r']
        P=lambda s:(round(lx+(rx-lx)*s/S),round(lz+(rz-lz)*s/S))
        for i in range(S):
            (ax,az),(bx,bz)=P(i),P(i+1)
            BR=(bx,0,bz);BL=(ax,0,az);TL=(ax,H,az);TR=(bx,H,bz)
            a=[BL[k]-BR[k] for k in range(3)];b=[TL[k]-BR[k] for k in range(3)]
            n=(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]); l=math.sqrt(sum(c*c for c in n))
            nq=tuple(max(-32768,min(32767,int(round(c/l*16384)))) for c in n)
            W=64//S;u0=q['tile']*64+W*i;u1=q['tile']*64+W*(i+1)-1
            faces.append(dict(i=[V(BR),V(BL),V(TL),V(TR)],n=nq,uv=[(u1,63),(u0,63),(u0,0),(u1,0)]))
    return verts,faces

def strips_for(d): return 8 if d<=90 else 4 if d<=220 else 2 if d<=420 else 1

def build_cell(cell):
    cx,cz=cell; ccx,ccz=cx*T+32,cz*T+32
    qs=[]
    for q in QUADS:
        if visible(cell,q):
            d=math.hypot(q['c'][0]-ccx,q['c'][1]-ccz); qs.append([q,strips_for(d),d])
    qs.sort(key=lambda e:e[2])
    while True:
        verts,faces=model([(q,s) for q,s,_ in qs])
        if len(faces)<=MAX_FACES and len(verts)<=MAX_VERTS: break
        far=max(range(len(qs)),key=lambda i:qs[i][2])            # reduz o LOD (ou descarta) a parede mais distante
        if qs[far][1]>1: qs[far][1]//=2
        else: qs.pop(far)
    return verts,faces,[(q,s) for q,s,_ in qs]

QUADS=wall_quads()
def pack(verts,faces):
    vb=b''.join(struct.pack('<hhh',*v) for v in verts)
    fb=b''.join(struct.pack('<BBBBhhhB',*f['i'],*f['n'],0x80) for f in faces)
    tb=b''.join(bytes([c for uv in f['uv'] for c in uv]) for f in faces)
    return vb+fb+tb

def build_all():
    out={}
    for z in range(N):
        for x in range(N):
            if solid(x,z): continue
            v,f,qs=build_cell((x,z))
            out[(x,z)]=dict(verts=v,faces=f,blob=pack(v,f),nquads=len(qs))
    return out

if __name__=='__main__':
    cells=build_all()
    nf=[len(c['faces']) for c in cells.values()]; nv=[len(c['verts']) for c in cells.values()]
    print('celulas abertas',len(cells),'| faces min/med/max',min(nf),sum(nf)//len(nf),max(nf),'| vertices max',max(nv),'| bytes totais',sum(len(c['blob']) for c in cells.values()))
