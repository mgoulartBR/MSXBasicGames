"""PVS por bloco para o E1M1: quais subquads de parede um bloco enxerga (LOS 2D em C) e o modelo Geo3D (vertices/faces/UV) dele."""
import sys,os,math,ctypes,collections
import numpy as np
HERE=os.path.dirname(os.path.abspath(__file__)); ROOT=os.path.join(HERE,'..'); sys.path.insert(0,HERE)
import e1m1,e1m1_map
from wad import Wad
BLOCK=128
MAX_SPR=10; MAX_FACES=255-MAX_SPR; MAX_VERTS=255-4*MAX_SPR
DOOR_SPECIALS=(1,26,27,28,31,32,33,34,117,118)
_lib=None
def lib():
    global _lib
    if _lib is None:
        so=os.path.join(ROOT,'private','vis.so')
        if not os.path.exists(so): os.system('gcc -O2 -shared -fPIC -o %s %s'%(so,os.path.join(HERE,'vis.c')))
        _lib=ctypes.CDLL(so)
    return _lib

def open_doors(m):
    """fase 1: as portas ficam abertas para sempre (teto = o menor teto vizinho)."""
    for l in m.linedefs:
        if l['special'] in DOOR_SPECIALS and l['left']>=0:
            a=m.sidedefs[l['right']]['sector']; b=m.sidedefs[l['left']]['sector']
            for d,o in ((a,b),(b,a)):
                sd=m.sectors[d]
                if sd['ceil']<=sd['floor']+0: sd['ceil']=max(sd['floor']+72,min(m.sectors[o]['ceil'],sd['floor']+72)); sd['is_door']=True

def blockers(m):
    vx=m.vertexes; out=[]
    for l in m.linedefs:
        (ax,ay),(bx,by)=vx[l['v1']],vx[l['v2']]
        if l['left']<0: out.append((ax,ay,bx,by)); continue
        fs=m.sectors[m.sidedefs[l['right']]['sector']]; bs=m.sectors[m.sidedefs[l['left']]['sector']]
        if min(fs['ceil'],bs['ceil'])<=max(fs['floor'],bs['floor']): out.append((ax,ay,bx,by))
    return np.ascontiguousarray(np.array(out,np.float64))

def los(segs,q):
    q=np.ascontiguousarray(np.array(q,np.float64)); out=np.zeros(len(q),np.uint8)
    lib().los_batch(len(segs),segs.ctypes.data_as(ctypes.c_void_p),len(q),q.ctypes.data_as(ctypes.c_void_p),out.ctypes.data_as(ctypes.c_void_p))
    return out.astype(bool)

class Level: pass
def load(wadpath):
    L=Level(); L.w=Wad(wadpath); L.m=L.w.load_map('E1M1'); open_doors(L.m)
    L.pcs=e1m1.wall_pieces(L.w,L.m); L.sc,L.info=e1m1.choose_scales(L.w,L.pcs); L.rgb,L.alpha,L.tiles=e1m1.build_atlas(L.w,L.sc,L.info)
    L.tile_avg={}
    for k,t in L.tiles.items():
        reg=L.rgb[t['y']:t['y']+t['h'],t['x']:t['x']+t['w']].reshape(-1,3).astype(float); L.tile_avg[k]=tuple(int(v) for v in reg.mean(0))
    L.R=e1m1_map.build(L.m); L.segs=blockers(L.m)
    L.stiles=make_tiles(L); L.lids=make_lids(L)
    L.subs=[]                                                           # subquad: dict(p=piece, rng=(...), A,u (dir), L)
    for pi,p in enumerate(L.pcs):
        (ax,ay),(bx,by)=p['A'],p['B']; Ln=math.hypot(bx-ax,by-ay)
        for s in e1m1.split_piece(p): L.subs.append(dict(p=p,s=s,A=(ax,ay),u=((bx-ax)/Ln,(by-ay)/Ln)))
    return L

LID_H=320; LID_MAXCEIL=130
def make_lids(L):
    """'tampas' pretas: acima de cada parede cujo setor tem teto baixo, uma face lisa preta ate LID_H acima do teto. Faz o papel do teto: oculta o que ha atras e acima da parede
    (salao alto alem de uma abertura, paredes de outros setores). So no lado frontal (observador a direita de A->B)."""
    m=L.m; vx=m.vertexes; out=[]
    for li,l in enumerate(m.linedefs):
        for side in ('right','left'):
            si=l[side]
            if si<0: continue
            fs=m.sectors[m.sidedefs[si]['sector']]
            if fs['ceil']>LID_MAXCEIL or fs['ctex']=='F_SKY1': continue
            osi=l['left' if side=='right' else 'right']
            if osi>=0 and m.sectors[m.sidedefs[osi]['sector']]['ceil']<=fs['ceil']: continue          # do outro lado nao ha nada mais alto que o teto daqui
            A,B=(vx[l['v1']],vx[l['v2']]) if side=='right' else (vx[l['v2']],vx[l['v1']])
            Ln=math.hypot(B[0]-A[0],B[1]-A[1])
            if Ln<2: continue
            out.append(dict(A=A,u=((B[0]-A[0])/Ln,(B[1]-A[1])/Ln),L=Ln,yb=fs['ceil'],yt=fs['ceil']+LID_H))
    return out

def visible_lids(L,vps):
    qs=[];meta=[]
    for i,d in enumerate(L.lids):
        ax,ay=d['A']; ux,uy=d['u']; nx,ny=uy,-ux
        for t in (0.15,0.5,0.85):
            px,py=ax+ux*d['L']*t+nx*1.5,ay+uy*d['L']*t+ny*1.5
            for vx,vy in vps:
                if (vx-px)*nx+(vy-py)*ny>0: qs.append((vx,vy,px,py)); meta.append(i)
    if not qs: return []
    ok=los(L.segs,qs); return sorted(set(np.array(meta)[ok].tolist()))

def lid_in_cone(L,i,bx,by,hd_deg,half_deg):
    d=L.lids[i]; cx,cy=bx*BLOCK+BLOCK/2,by*BLOCK+BLOCK/2; hd=math.radians(hd_deg); half=math.radians(half_deg)
    ax,ay=d['A']; ux,uy=d['u']
    for t in (0,0.25,0.5,0.75,1):
        x,y=ax+ux*d['L']*t-cx,ay+uy*d['L']*t-cy; r=math.hypot(x,y)
        if r<110: return True
        if abs((math.atan2(y,x)-hd+math.pi)%(2*math.pi)-math.pi)<half+math.asin(min(1,90/r)): return True
    return False

TILE=32; TMAX=10
def make_tiles(L):
    """retangulos de piso/teto por setor: grade de 32 unidades (celula do setor = setor no centro), fundidos em retangulos de ate 6x6 celulas."""
    R=L.R; m=L.m; taller=set()
    for l in m.linedefs:
        if l['left']<0: continue
        a=m.sidedefs[l['right']]['sector']; b=m.sidedefs[l['left']]['sector']
        if m.sectors[a]['ceil']<m.sectors[b]['ceil']: taller.add(a)
        if m.sectors[b]['ceil']<m.sectors[a]['ceil']: taller.add(b)
    cell={}
    gx0=int(R.x0//TILE); gy0=int(R.y0//TILE)
    for gx in range(gx0,gx0+R.nx*8//TILE+2):
        for gy in range(gy0,gy0+R.ny*8//TILE+2):
            x,y=gx*TILE+TILE//2,gy*TILE+TILE//2; cx,cy=int((x-R.x0)//8),int((y-R.y0)//8)
            if 0<=cx<R.nx and 0<=cy<R.ny and R.open[cy,cx]: cell[(gx,gy)]=int(R.sec[cy,cx])
    used=set(); tiles=[]
    for (gx,gy) in sorted(cell):
        if (gx,gy) in used: continue
        sec=cell[(gx,gy)]; w=1
        while w<TMAX and cell.get((gx+w,gy))==sec and (gx+w,gy) not in used: w+=1
        h=1
        while h<TMAX and all(cell.get((gx+i,gy+h))==sec and (gx+i,gy+h) not in used for i in range(w)): h+=1
        for i in range(w):
            for j in range(h): used.add((gx+i,gy+j))
        sd=m.sectors[sec]
        tiles.append(dict(sec=sec,x0=gx*TILE,y0=gy*TILE,x1=(gx+w)*TILE,y1=(gy+h)*TILE,floor=sd['floor'],ceil=sd['ceil'],ceil_ok=sec in taller,sky=sd['ctex']=='F_SKY1'))
    return tiles

def visible_tiles(L,vps,bx,by):
    """tiles com algum ponto (centro ou 4 pontos internos) visivel de algum ponto de vista; exclui os que tocam o proprio bloco (a base cobre o piso sob o jogador)."""
    x0,y0=bx*BLOCK-40,by*BLOCK-40; x1,y1=(bx+1)*BLOCK+40,(by+1)*BLOCK+40; out=[]; qs=[];meta=[]
    for ti,t in enumerate(L.stiles):
        if t['x0']<x1 and t['x1']>x0 and t['y0']<y1 and t['y1']>y0: continue
        if t['x0']-x1>1800 or x0-t['x1']>1800 or t['y0']-y1>1800 or y0-t['y1']>1800: continue
        w=t['x1']-t['x0'];h=t['y1']-t['y0']
        for fx,fy in ((.5,.5),(.2,.2),(.8,.2),(.2,.8),(.8,.8)):
            px,py=t['x0']+w*fx,t['y0']+h*fy
            for vx,vy in vps: qs.append((vx,vy,px,py)); meta.append(ti)
    if not qs: return []
    ok=los(L.segs,qs); return sorted(set(np.array(meta)[ok].tolist()))

def block_viewpoints(L,bx,by):
    R=L.R; pts=[]
    cx0=int((bx*BLOCK-R.x0)//e1m1_map.CELL); cy0=int((by*BLOCK-R.y0)//e1m1_map.CELL); n=BLOCK//e1m1_map.CELL
    for cy in range(cy0,cy0+n):
        for cx in range(cx0,cx0+n):
            if 0<=cy<R.ny and 0<=cx<R.nx and R.open[cy,cx] and (cx-cx0)%4==2 and (cy-cy0)%4==2: pts.append((R.x0+cx*e1m1_map.CELL+4,R.y0+cy*e1m1_map.CELL+4))
    if not pts:                                                          # bloco so com bordas abertas: usa qualquer celula aberta
        for cy in range(cy0,cy0+n):
            for cx in range(cx0,cx0+n):
                if 0<=cy<R.ny and 0<=cx<R.nx and R.open[cy,cx]: pts.append((R.x0+cx*e1m1_map.CELL+4,R.y0+cy*e1m1_map.CELL+4)); break
            if pts: break
    return pts

def visible_subs(L,vps):
    """indices dos subquads com algum ponto visivel de algum ponto de vista do lado certo."""
    qs=[];meta=[]
    for si,s in enumerate(L.subs):
        (d0,d1,*_)=s['s']; ax,ay=s['A']; ux,uy=s['u']; nx,ny=uy,-ux
        for t in (0.15,0.5,0.85):
            d=d0+(d1-d0)*t; px,py=ax+ux*d+nx*1.5,ay+uy*d+ny*1.5
            for vx,vy in vps:
                if (vx-px)*nx+(vy-py)*ny>0: qs.append((vx,vy,px,py)); meta.append(si)
    if not qs: return set()
    ok=los(L.segs,qs); return set(np.array(meta)[ok].tolist())

def strips(a,b,dmin_fn,k):
    """corta [a,b] (distancia ao longo da parede) em tiras cujo tamanho ~ k*distancia minima ao bloco."""
    out=[]
    def rec(a,b):
        n=b-a; t=min(max(k*dmin_fn(a,b),12),230)
        if n>t*1.01:
            m=(a+b)/2; rec(a,m); rec(m,b)
        else: out.append((a,b))
    rec(a,b); return out

DFLAT=260                                  # pecas cujo ponto mais proximo do bloco esta alem disso viram UMA face de cor lisa (media da textura)
def model(L,sub_ids,bx,by,k,minlen=24,dflat=None,tiles=(),lids=()):
    """tiras por distancia + preenchimento guloso por importancia (area projetada ~ comprimento*altura/(dist+48)^2) ate os limites do Geo3D.
    Devolve verts, faces, uvs, flat (None = texturizada; (r,g,b) = face lisa), fracao de importancia mantida."""
    dflat=DFLAT if dflat is None else dflat
    cx,cy=bx*BLOCK+BLOCK/2,by*BLOCK+BLOCK/2; half=BLOCK/2*1.2
    def dseg(ax,ay,ux,uy,a,b):
        best=1e9
        for i in range(5):
            d=a+(b-a)*i/4; x,y=ax+ux*d,ay+uy*d; best=min(best,max(0,max(abs(x-cx),abs(y-cy))-half))
        return best
    items=[]; groups={}
    for si in sub_ids: groups.setdefault(id(L.subs[si]['p']),[]).append(si)
    for pid,sis in groups.items():
        s0=L.subs[sis[0]]; p=s0['p']; ax,ay=s0['A']; ux,uy=s0['u']; Ln=math.hypot(p['B'][0]-ax,p['B'][1]-ay)
        if dseg(ax,ay,ux,uy,0,Ln)>dflat:                              # peca distante: UMA face por faixa vertical, com UMA repeticao da textura esticada no comprimento todo
            bands={}
            for si in sis: sub=L.subs[si]['s']; bands[(sub[2],sub[3],round(sub[6],3),round(sub[7],3))]=sub
            for (hb,ht,r0,r1),sub in bands.items():
                nch=max(1,math.ceil(Ln/256))                                   # pedacos de ate 256 unidades: limita a distorcao do mapeamento afim
                for ci in range(nch):
                    a_,b_=Ln*ci/nch,Ln*(ci+1)/nch; area=(b_-a_)*(ht-hb)
                    items.append((area/(dseg(ax,ay,ux,uy,a_,b_)+48)**2,'F',(p,ax+ux*a_,ay+uy*a_,ux,uy,b_-a_,hb,ht,r0,r1),0,0))
            continue
        for si in sis:
            s=L.subs[si]; (d0,d1,hb,ht,u0,u1,r0,r1)=s['s']
            def dmin(a,b,ax=ax,ay=ay,ux=ux,uy=uy): return dseg(ax,ay,ux,uy,a,b)
            def rec(a,b):
                n=b-a; t=min(max(k*dmin(a,b),minlen),230)
                if n>t*1.01: m=(a+b)/2; rec(a,m); rec(m,b)
                else: items.append((n*(ht-hb)/(dmin(a,b)+48)**2,'T',si,a,b))
            rec(d0,d1)
    for li_ in lids:                                                   # tampas pretas (fazem o papel do teto)
        d=L.lids[li_]; ax,ay=d['A']; ux,uy=d['u']
        def dminl(a,b,ax=ax,ay=ay,ux=ux,uy=uy):
            best=1e9
            for i in range(5):
                dd=a+(b-a)*i/4; x,y=ax+ux*dd,ay+uy*dd; best=min(best,max(0,max(abs(x-cx),abs(y-cy))-half))
            return best
        def recl(a,b,li_=li_,d=d):
            t=min(max(2.5*dminl(a,b),80),600)
            if b-a>t*1.01: m_=(a+b)/2; recl(a,m_); recl(m_,b)
            else: items.append((1.2*(b-a)*72/(dminl(a,b)+48)**2,'L',(li_,a,b),0,0))
        recl(0,d['L'])
    for ti in tiles:                                                   # pisos e tetos (faces lisas); importancia = area projetada, com peso 0.7
        t=L.stiles[ti]; ddx=max(t['x0']-(cx+half),(cx-half)-t['x1'],0); ddy=max(t['y0']-(cy+half),(cy-half)-t['y1'],0); dm=math.hypot(ddx,ddy)
        area=(t['x1']-t['x0'])*(t['y1']-t['y0']); imp=0.25*area*41/(dm+48)**3
        items.append((imp,'P',(ti,0),0,0))
        if False and t['ceil_ok'] and not t['sky']: items.append((3.0*area*max(20,t['ceil']-41)/(dm+48)**3,'P',(ti,1),0,0))
    items.sort(key=lambda e:-e[0])
    verts=[];vi={};faces=[];uvs=[];flat=[];tot=sum(e[0] for e in items) or 1;kept=0
    def V(pt):
        if pt not in vi: vi[pt]=len(verts); verts.append(pt)
        return vi[pt]
    for it in items:
        imp,kind=it[0],it[1]
        if kind=='L':
            li_,a,b=it[2]; d=L.lids[li_]; ax,ay=d['A']; ux,uy=d['u']
            xa,ya=round(ax+ux*a),round(ay+uy*a); xb,yb=round(ax+ux*b),round(ay+uy*b)
            if (xa,ya)==(xb,yb): continue
            q4=[(xb,d['yb'],yb),(xa,d['yb'],ya),(xa,d['yt'],ya),(xb,d['yt'],yb)]
            if len(faces)+1>MAX_FACES or len(verts)+len(set(q4)-set(vi))>MAX_VERTS: continue
            faces.append([V(q) for q in q4]); uvs.append((0,)*8); flat.append(('C',-1)); continue
        if kind=='P':
            ti,isc=it[2]; t=L.stiles[ti]; hy=t['ceil'] if isc else t['floor']
            xa,xb,ya,yb=t['x0'],t['x1'],t['y0'],t['y1']
            q4=[(xa,hy,ya),(xb,hy,ya),(xb,hy,yb),(xa,hy,yb)] if not isc else [(xa,hy,ya),(xa,hy,yb),(xb,hy,yb),(xb,hy,ya)]
            if len(faces)+1>MAX_FACES or len(verts)+len(set(q4)-set(vi))>MAX_VERTS: continue
            faces.append([V(q) for q in q4]); uvs.append((0,)*8); flat.append(('C' if isc else 'F',t['sec'])); kept+=imp*0; continue
        if kind=='F':
            p,ax,ay,ux,uy,Ln,hb,ht,r0,r1=it[2]; xa,ya=round(ax),round(ay); xb,yb=round(ax+ux*Ln),round(ay+uy*Ln); tl=L.tiles[p['tex']]
            if (xa,ya)==(xb,yb): continue
            q4=[(xb,hb,yb),(xa,hb,ya),(xa,ht,ya),(xb,ht,yb)]
            if len(faces)+1>MAX_FACES or len(verts)+len(set(q4)-set(vi))>MAX_VERTS: continue
            c=lambda v:min(255,max(0,v)); uA=c(tl['x']); uB=c(tl['x']+tl['w']-1)
            v0=c(int(tl['y']+r0*tl['sy'])); v1=c(max(int(tl['y']+r1*tl['sy']+0.5)-1,v0))
            faces.append([V(q) for q in q4]); uvs.append((uB,v1,uA,v1,uA,v0,uB,v0)); flat.append(None); kept+=imp; continue
        si,a,b=it[2],it[3],it[4]
        s=L.subs[si]; p=s['p']; (d0,d1,hb,ht,u0,u1,r0,r1)=s['s']; ax,ay=s['A']; ux,uy=s['u']; tl=L.tiles[p['tex']]
        xa,ya=round(ax+ux*a),round(ay+uy*a); xb,yb=round(ax+ux*b),round(ay+uy*b)
        if (xa,ya)==(xb,yb): continue
        q4=[(xb,hb,yb),(xa,hb,ya),(xa,ht,ya),(xb,ht,yb)]
        if len(faces)+1>MAX_FACES or len(verts)+len(set(q4)-set(vi))>MAX_VERTS: continue
        ua=u0+(u1-u0)*(a-d0)/(d1-d0); ub=u0+(u1-u0)*(b-d0)/(d1-d0)
        c=lambda v:min(255,max(0,v))
        uA=c(int(tl['x']+ua*tl['sx'])); uB=c(max(int(tl['x']+ub*tl['sx']+0.5)-1,uA))
        v0=c(int(tl['y']+r0*tl['sy'])); v1=c(max(int(tl['y']+r1*tl['sy']+0.5)-1,v0))
        faces.append([V(q) for q in q4]); uvs.append((uB,v1,uA,v1,uA,v0,uB,v0)); flat.append(None); kept+=imp
    return verts,faces,uvs,flat,kept/tot

K_LIST=(1.0,1.5,2.2)                # tiras mais longas distorcem a textura (mapeamento afim)
NHEAD=4; CONE=92.0                       # 4 setores de direcao (90 graus cada); cone de +-92 graus: cobre o campo de visao (+-40) com histerese de 5 graus
def inview(L,si,bx,by,hd_deg,half_deg):
    """o subquad tem algum ponto dentro do cone (azimute do bloco, alargado pelo tamanho do bloco)?"""
    s=L.subs[si];(d0,d1,*_)=s['s'];ax,ay=s['A'];ux,uy=s['u']; cx,cy=bx*BLOCK+BLOCK/2,by*BLOCK+BLOCK/2
    hd=math.radians(hd_deg); half=math.radians(half_deg)
    for t in (0,0.25,0.5,0.75,1):
        d=d0+(d1-d0)*t; x,y=ax+ux*d-cx,ay+uy*d-cy; r=math.hypot(x,y)
        if r<110: return True
        da=abs((math.atan2(y,x)-hd+math.pi)%(2*math.pi)-math.pi)
        if da<half+math.asin(min(1,90/r)): return True
    return False
def tile_in_cone(L,ti,bx,by,hd_deg,half_deg):
    t=L.stiles[ti]; cx,cy=bx*BLOCK+BLOCK/2,by*BLOCK+BLOCK/2; hd=math.radians(hd_deg); half=math.radians(half_deg)
    x,y=(t['x0']+t['x1'])/2-cx,(t['y0']+t['y1'])/2-cy; r=math.hypot(x,y); rad=math.hypot(t['x1']-t['x0'],t['y1']-t['y0'])/2+90
    if r<=rad: return True
    da=abs((math.atan2(y,x)-hd+math.pi)%(2*math.pi)-math.pi)
    return da<half+math.asin(min(1,rad/r))
def head_deg(h): return 90-h*90                 # yaw 0 = norte (Doom 90), yaw cresce no sentido horario

def build_block(L,bx,by,ids=None,head=None,tiles=(),lids=()):
    vps=block_viewpoints(L,bx,by)
    if not vps: return None
    if ids is None: ids=sorted(visible_subs(L,vps))
    if head is not None: ids=[i for i in ids if inview(L,i,bx,by,head_deg(head),CONE)]
    best=None
    for k in K_LIST:                                                    # tiras mais longas (k maior) = menos vertices; escolhe o menor k que mantem quase tudo
        v,f,u,fl,frac=model(L,ids,bx,by,k,tiles=tiles,lids=lids)
        if best is None or frac>best[4]+1e-9: best=(v,f,u,fl,frac,k)
        if frac>=0.999: break
    v,f,u,fl,frac,k=best
    return dict(verts=v,faces=f,uvs=u,flat=fl,kept=frac,nsub=len(ids),vps=len(vps),k=k)

def open_blocks(L):
    R=L.R; s=set()
    ys,xs=np.nonzero(R.open)
    for y,x in zip(ys,xs): s.add((int((R.x0+x*e1m1_map.CELL)//BLOCK),int((R.y0+y*e1m1_map.CELL)//BLOCK)))
    return sorted(s)

if __name__=='__main__':
    import time; t=time.time(); L=load(sys.argv[1]); print('carregado %.1fs'%(time.time()-t),'subquads',len(L.subs),'blockers',len(L.segs))
    bl=open_blocks(L); print('blocos abertos',len(bl)); res={}
    for i,b in enumerate(bl):
        res[b]=build_block(L,*b)
        if i%50==0: print(i,b,'%.0fs'%(time.time()-t),flush=True)
    ok=[r for r in res.values() if r and not r.get('overflow')]; ov=[b for b,r in res.items() if r and r.get('overflow')]
    nf=[len(r['faces']) for r in ok]; nv=[len(r['verts']) for r in ok]
    print('ok',len(ok),'overflow',len(ov),'faces med/max',int(np.median(nf)),max(nf),'verts max',max(nv),'k usados',collections.Counter(r['k'] for r in ok))
    print('bytes estimados',sum(2+6*len(r['verts'])+19*len(r['faces']) for r in ok))
    import pickle; pickle.dump(res,open(os.path.join(ROOT,'private','pvs.pkl'),'wb'))
