"""Mede buracos: compara a mascara de paredes do PVS do bloco (com limites) com a de TODAS as paredes sem limites (modelo de referencia do Geo3D)."""
import sys,math,random,os
sys.path.insert(0,os.path.dirname(__file__))
import numpy as np
import e1m1_pvs as P, e1m1_view as V
from gen_scenes import render_faces
def mask_of(L,b,px,py,ang):
    yaw=math.radians(90-ang); s,c=math.sin(yaw),math.cos(yaw); q=lambda v:int(round(v*16384))
    M=[q(c),0,q(-s),0,16384,0,q(s),0,q(c)]; eye=V.floor_at(L,px,py)+41
    cfg=M+[round(-(c*px-s*py)),-eye,round(-(s*px+c*py))]+[160,128,89,2,256,178]
    faces=[tuple(f)+(0,-16384,0,0x80) for f in b['faces']]
    cmds,_,_,_=render_faces(cfg,b['verts'],faces,8,0,[-5734,11207,-10486],dict(on=False,texx=0,texy=0,tstride=0,uv=[(0,)*8]*len(faces)))
    m=np.zeros((178,256),bool)
    for cm in cmds:
        y=(cm[2]|cm[3]<<8)&0x3FF; x0=cm[0]|(cm[1]&1)<<8; n=cm[4]|(cm[5]&7)<<8
        if 0<=y<178: m[y,x0:x0+n+1]=True
    return m
def truth(L,px,py):
    # todos os subquads com tiras por distancia ao ponto (sem PVS, sem limites)
    ids=range(len(L.subs)); return ids
if __name__=='__main__':
    L=P.load(sys.argv[1]); R=L.R; ys,xs=np.nonzero(R.open); random.seed(int(sys.argv[2]) if len(sys.argv)>2 else 5); N=int(sys.argv[3]) if len(sys.argv)>3 else 40
    P.MAX_FACES_SAVE=(P.MAX_FACES,P.MAX_VERTS); res=[]; cache={}
    for _ in range(N):
        i=random.randrange(len(xs)); px=R.x0+int(xs[i])*8+4; py=R.y0+int(ys[i])*8+4; ang=random.random()*360
        blk=(int(px//128),int(py//128))
        h=int(((ang-90)%360)/90+0.5)%4 if False else int(round(((90-ang)%360)/90))%4
        if (blk,h) not in cache: cache[(blk,h)]=P.build_block(L,*blk,head=h)
        b=cache[(blk,h)]
        if not b: continue
        # verdade: modelo sem limites com TODOS os subquads, tiras k=1.0 a partir do ponto
        P.MAX_FACES,P.MAX_VERTS=10000,40000
        t=P.model(L,list(range(len(L.subs))),blk[0],blk[1],1.0)
        P.MAX_FACES,P.MAX_VERTS=P.MAX_FACES_SAVE
        bt=dict(verts=t[0],faces=t[1],uvs=t[2])
        m=mask_of(L,b,px,py,ang); mt=mask_of(L,bt,px,py,ang)
        miss=(mt&~m)[:178].sum(); res.append((miss/max(1,mt.sum()),px,py,round(ang),int(miss)))
    fr=[r[0] for r in res]; print('amostras',len(res),'perda media %.1f%%'%(100*np.mean(fr)),'p90 %.1f%%'%(100*np.percentile(fr,90)))
    res.sort(reverse=True); print(res[:6])
