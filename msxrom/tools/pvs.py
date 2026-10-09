"""Conjunto visivel (PVS) por BLOCO 2x2 de celulas, para o Geo3D.

A ROM envia UMA vez, ao entrar num bloco, todas as paredes (e o piso de acido) que podem ser vistos dali (<= MAX_FACES faces e
<= MAX_VERTS vertices; o resto do limite de 255 fica para os sprites). Por quadro so vao matriz, luz, faces dos sprites e o RUN.
Visibilidade em 2D: uma face entra no conjunto se, de algum ponto de vista no bloco (3x3 por celula aberta), algum ponto dela
(3 amostras) esta do lado de fora e o segmento nao atravessa celula solida. Piso de acido: visivel se algum ponto de vista enxerga
o centro ou um canto da celula."""
import math,struct
import level
T=level.T; H=level.H; W=level.W; HH=level.HH
MAX_SPR=10                                   # sprites simultaneos por quadro (inimigos + kits); a ROM limita a 10
MAX_FACES=255-MAX_SPR; MAX_VERTS=255-4*MAX_SPR
solid=level.solid
BW=(W+1)//2; BH=(HH+1)//2
def tile_of(ch,x,z): return 2 if ch=='L' else [0,0,1,2][(x*7+z*3)%4]
ACID_TILE=3

def wall_quads():
    quads=[]
    for z in range(HH):
        for x in range(W):
            ch=level.MAP[z][x]
            if ch not in '#L': continue
            x0,x1,z0,z1=x*T,x*T+T,z*T,z*T+T; tile=tile_of(ch,x,z)
            sides=[(not solid(x,z-1),((x0,z0),(x1,z0)),(0,-1)),(not solid(x,z+1),((x1,z1),(x0,z1)),(0,1)),
                   (not solid(x-1,z),((x0,z1),(x0,z0)),(-1,0)),(not solid(x+1,z),((x1,z0),(x1,z1)),(1,0))]
            for ok,(l,r),nrm in sides:
                if ok: quads.append(dict(l=l,r=r,n=nrm,tile=tile,c=((l[0]+r[0])/2,(l[1]+r[1])/2)))
    return quads
def acid_cells(): return [(x,z) for z in range(HH) for x in range(W) if level.acid(x,z)]

def clear(x0,z0,x1,z1):
    steps=int(max(abs(x1-x0),abs(z1-z0))/6)+1
    for i in range(1,steps):
        x=x0+(x1-x0)*i/steps; z=z0+(z1-z0)*i/steps
        if solid(int(x)//T,int(z)//T): return False
    return True

def viewpoints(block):
    bx,bz=block; pts=set()
    for cz in (2*bz,2*bz+1):
        for cx in (2*bx,2*bx+1):
            if solid(cx,cz): continue
            for dx in (-30,0,30):
                for dz in (-30,0,30): pts.add((cx*T+32+dx,cz*T+32+dz))
    return sorted(pts)

def wall_visible(vps,q):
    nx,nz=q['n']
    for t in (0.1,0.5,0.9):
        tx=q['l'][0]+(q['r'][0]-q['l'][0])*t+nx; tz=q['l'][1]+(q['r'][1]-q['l'][1])*t+nz
        for vx,vz in vps:
            if (vx-tx)*nx+(vz-tz)*nz<=0: continue
            if clear(vx,vz,tx,tz): return True
    return False
def floor_visible(vps,cell):
    x,z=cell
    for tx,tz in ((x*T+32,z*T+32),(x*T+6,z*T+6),(x*T+58,z*T+6),(x*T+6,z*T+58),(x*T+58,z*T+58)):
        for vx,vz in vps:
            if clear(vx,vz,tx,tz): return True
    return False

def model(items):
    """items: ('w',quad,S) ou ('f',(x,z),S). Devolve vertices e faces (indices, normal Q14, uv)."""
    verts=[];vi={};faces=[]
    def V(p):
        if p not in vi: vi[p]=len(verts); verts.append(p)
        return vi[p]
    for kind,obj,S in items:
        if kind=='w':
            q=obj; (lx,lz),(rx,rz)=q['l'],q['r']
            P=lambda s:(round(lx+(rx-lx)*s/S),round(lz+(rz-lz)*s/S))
            for i in range(S):
                (ax,az),(bx,bz)=P(i),P(i+1)
                BR=(bx,0,bz);BL=(ax,0,az);TL=(ax,H,az);TR=(bx,H,bz)
                a=[BL[k]-BR[k] for k in range(3)];b=[TL[k]-BR[k] for k in range(3)]
                n=(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]); l=math.sqrt(sum(c*c for c in n))
                nq=tuple(max(-32768,min(32767,int(round(c/l*16384)))) for c in n)
                Wd=64//S;u0=q['tile']*64+Wd*i;u1=q['tile']*64+Wd*(i+1)-1
                faces.append(dict(i=[V(BR),V(BL),V(TL),V(TR)],n=nq,uv=[(u1,63),(u0,63),(u0,0),(u1,0)]))
        else:                                                     # piso: normal para cima, visto de cima no sentido horario
            x,z=obj; Wd=64//S
            for j in range(S):
                for i in range(S):
                    xa,xb=x*T+i*T//S,x*T+(i+1)*T//S; za,zb=z*T+j*T//S,z*T+(j+1)*T//S
                    u0=ACID_TILE*64+Wd*i;u1=u0+Wd-1;vn=63-Wd*j;vf=vn-Wd+1
                    faces.append(dict(i=[V((xa,0,za)),V((xa,0,zb)),V((xb,0,zb)),V((xb,0,za))],n=(0,16384,0),uv=[(u0,vn),(u0,vf),(u1,vf),(u1,vn)]))
    return verts,faces

def strips_for(d): return 8 if d<=130 else 4 if d<=260 else 2 if d<=480 else 1

def _dist(q,cx,cz): return math.hypot(q['c'][0]-cx,q['c'][1]-cz)
def merge_walls(walls,cx,cz,d2,d4):
    """Funde paredes alinhadas e vizinhas (mesma normal, mesmo tile) distantes: 2 celulas se d>d2, 4 se d>d4. A textura estica."""
    groups={}
    for q in walls: groups.setdefault((q['n'],q['l'][0] if q['n'][0] else q['l'][1]),[]).append(q)
    out=[]
    for (n,_),qs in groups.items():
        ax=1 if n[0] else 0
        lo=lambda q:min(q['l'][ax],q['r'][ax]); hi=lambda q:max(q['l'][ax],q['r'][ax])
        qs.sort(key=lo); i=0
        while i<len(qs):
            m=1
            for size,thr in ((4,d4),(2,d2)):
                run=qs[i:i+size]
                if len(run)==size and all(_dist(r,cx,cz)>thr for r in run) and all(r['tile']==run[0]['tile'] for r in run) \
                   and all(hi(run[k])==lo(run[k+1]) for k in range(size-1)):
                    m=size; break
            run=qs[i:i+m]
            if m==1: out.append(dict(run[0],S=None))
            else:
                inc=run[0]['l'][ax]<run[0]['r'][ax]
                l,r=(run[0]['l'],run[-1]['r']) if inc else (run[-1]['l'],run[0]['r'])
                out.append(dict(l=l,r=r,n=n,tile=run[0]['tile'],c=((l[0]+r[0])/2,(l[1]+r[1])/2),S=1))
            i+=m
    return out

MERGE_TRIES=[(1e9,1e9),(480,900),(300,600),(200,400),(130,260)]       # (limiar p/ fundir 2, limiar p/ fundir 4); 1e9 = sem fusao

def assemble(walls,floors,thr,cx,cz):
    items=[]
    for q in merge_walls(walls,cx,cz,*thr):
        d=_dist(q,cx,cz); items.append(['w',q,q['S'] or strips_for(d),d])
    for c in floors:
        d=math.hypot(c[0]*T+32-cx,c[1]*T+32-cz); items.append(['f',c,2 if d<=130 else 1,d])
    return items
def fits(items):
    verts,faces=model([(k,o,s) for k,o,s,_ in items])
    return len(faces)<=MAX_FACES and len(verts)<=MAX_VERTS,verts,faces

def build_block(block):
    bx,bz=block; cx,cz=bx*2*T+T,bz*2*T+T                         # centro do bloco
    vps=viewpoints(block)
    walls=[q for q in QUADS if wall_visible(vps,q)]; floors=[c for c in ACIDS if floor_visible(vps,c)]
    for thr in MERGE_TRIES:                                       # fusao progressiva de paredes distantes ate caber
        items=assemble(walls,floors,thr,cx,cz); ok,verts,faces=fits(items)
        if ok: return verts,faces
    dropped=0
    while True:                                                   # ultimo recurso: sacrificar itens
        ok,verts,faces=fits(items)
        if ok: break
        far=lambda c:max(c,key=lambda i:items[i][3])
        c=[i for i,e in enumerate(items) if e[0]=='f' and e[2]>1]
        if c: items[far(c)][2]=1; continue
        c=[i for i,e in enumerate(items) if e[0]=='w' and e[2]>1]
        if c: items[far(c)][2]//=2; continue
        c=[i for i,e in enumerate(items) if e[0]=='f']
        if c: items.pop(far(c)); dropped+=1; STATS['acid']+=1; continue
        items.pop(far(list(range(len(items))))); dropped+=1; STATS['wall']+=1
    STATS['dropped']+=dropped
    return verts,faces

QUADS=wall_quads(); ACIDS=acid_cells(); STATS={'dropped':0,'acid':0,'wall':0}
def pack(verts,faces):
    vb=b''.join(struct.pack('<hhh',*v) for v in verts)
    fb=b''.join(struct.pack('<BBBBhhhB',*f['i'],*f['n'],0x80) for f in faces)
    tb=b''.join(bytes([c for uv in f['uv'] for c in uv]) for f in faces)
    return bytes([len(verts),len(faces)])+vb+fb+tb                # cabeçalho: nv, nf

def build_all():
    out={}
    for bz in range(BH):
        for bx in range(BW):
            if all(solid(cx,cz) for cz in (2*bz,2*bz+1) for cx in (2*bx,2*bx+1)): continue
            v,f=build_block((bx,bz)); out[(bx,bz)]=dict(verts=v,faces=f,blob=pack(v,f))
    return out

if __name__=='__main__':
    import time; t=time.time(); blocks=build_all()
    nf=[len(c['faces']) for c in blocks.values()]; nv=[len(c['verts']) for c in blocks.values()]
    print('blocos',len(blocks),'| faces min/med/max',min(nf),sum(nf)//len(nf),max(nf),'| vertices max',max(nv),'| bytes totais',sum(len(c['blob']) for c in blocks.values()),'| descartados: acido',STATS['acid'],'paredes',STATS['wall'],'| %.0fs'%(time.time()-t))
