"""Rasters do E1M1: celulas de 8 unidades transitaveis (inundacao a partir do jogador, bloqueios = linhas de um lado e degraus/janelas
intransponiveis) e setor de cada celula (lado do linedef mais proximo)."""
import sys,math
import numpy as np
CELL=8
class MapRaster: pass

def build(m):
    vx=m.vertexes; xs=[v[0] for v in vx]; ys=[v[1] for v in vx]
    R=MapRaster(); R.x0=(min(xs)//CELL-2)*CELL; R.y0=(min(ys)//CELL-2)*CELL
    R.nx=(max(xs)-R.x0)//CELL+3; R.ny=(max(ys)-R.y0)//CELL+3
    blocked=np.zeros((R.ny,R.nx),bool)
    segs=[]
    for l in m.linedefs:
        (ax,ay),(bx,by)=vx[l['v1']],vx[l['v2']]
        solid=l['left']<0 or bool(l['flags']&1)
        if not solid:
            fs=m.sectors[m.sidedefs[l['right']]['sector']]; bs=m.sectors[m.sidedefs[l['left']]['sector']]
            if l['special']==0 and min(fs['ceil'],bs['ceil'])-max(fs['floor'],bs['floor'])<56: solid=True   # sem altura livre (so isso bloqueia na grade; degraus sao tratados pelo motor: sobe ate 24, desce qualquer altura); linhas com acao ficam passaveis
        if solid:
            n=int(math.hypot(bx-ax,by-ay)/2)+2
            for i in range(n+1):
                x=ax+(bx-ax)*i/n; y=ay+(by-ay)*i/n; blocked[int((y-R.y0)//CELL),int((x-R.x0)//CELL)]=True
        segs.append((ax,ay,bx,by))
    # inundacao
    start=[t for t in m.things if t['type']==1][0]
    sx,sy=int((start['x']-R.x0)//CELL),int((start['y']-R.y0)//CELL)
    open_=np.zeros_like(blocked); stack=[(sy,sx)]; open_[sy,sx]=True
    while stack:
        y,x=stack.pop()
        for dy,dx in ((1,0),(-1,0),(0,1),(0,-1)):
            yy,xx=y+dy,x+dx
            if 0<=yy<R.ny and 0<=xx<R.nx and not open_[yy,xx] and not blocked[yy,xx]: open_[yy,xx]=True; stack.append((yy,xx))
    R.open=open_
    # setor de cada celula aberta: linedef mais proximo, lado do ponto
    ys_,xs_=np.nonzero(open_); px=R.x0+xs_*CELL+CELL/2; py=R.y0+ys_*CELL+CELL/2
    S=np.array(segs,np.float64); best=np.full(len(px),1e18); sec=np.zeros(len(px),np.int32)
    side_sec=np.array([[m.sidedefs[l['right']]['sector'],m.sidedefs[l['left']]['sector'] if l['left']>=0 else -1] for l in m.linedefs])
    for i in range(0,len(px),4000):
        X=px[i:i+4000,None]; Y=py[i:i+4000,None]
        ax,ay,bx,by=S[:,0][None],S[:,1][None],S[:,2][None],S[:,3][None]
        dx,dy=bx-ax,by-ay; L2=dx*dx+dy*dy; t=np.clip(((X-ax)*dx+(Y-ay)*dy)/np.where(L2==0,1,L2),0,1)
        d=(X-(ax+t*dx))**2+(Y-(ay+t*dy))**2
        j=d.argmin(1); dm=d[np.arange(len(j)),j]
        side=(dx[0][j]*(Y[:,0]-ay[0][j])-dy[0][j]*(X[:,0]-ax[0][j]))      # >0: esquerda de v1->v2 ; direita = lado 'right'? (Doom: direita = lado frontal)
        right=side<0
        ss=np.where(right,side_sec[j,0],side_sec[j,1]); ss=np.where(ss<0,side_sec[j,0],ss)
        sec[i:i+4000]=ss; best[i:i+4000]=dm
    R.sec=-np.ones((R.ny,R.nx),np.int32); R.sec[ys_,xs_]=sec
    return R

if __name__=='__main__':
    sys.path.insert(0,__file__.rsplit('/',1)[0]); from wad import Wad
    from PIL import Image
    w=Wad(sys.argv[1]); m=w.load_map('E1M1'); R=build(m)
    print('grade',R.nx,R.ny,'abertas',int(R.open.sum()),'setores usados',len(set(R.sec[R.open])),'de',len(m.sectors))
    im=np.zeros((R.ny,R.nx,3),np.uint8); rng=np.random.RandomState(1); col=rng.randint(60,255,(len(m.sectors),3))
    im[R.open]=col[R.sec[R.open]]
    for t in m.things:
        cx,cy=int((t['x']-R.x0)//CELL),int((t['y']-R.y0)//CELL)
        if 0<=cy<R.ny and 0<=cx<R.nx: im[cy,cx]=(255,255,255) if R.open[cy,cx] else (255,0,0)
    Image.fromarray(im[::-1]).resize((R.nx*2,R.ny*2),Image.NEAREST).save(sys.argv[2])
    bad=[t for t in m.things if t['type'] in (3004,9,3001,2011,2012,2014,2015,2018,2019,2001,2007,2008,2048,2049,2035,2028,2046,48,34,2023,2024,17,2022,2025,2026,2045) and not R.open[int((t['y']-R.y0)//CELL),int((t['x']-R.x0)//CELL)]]
    print('things relevantes fora da area aberta:',len(bad),[(t['type'],t['x'],t['y']) for t in bad][:10])
