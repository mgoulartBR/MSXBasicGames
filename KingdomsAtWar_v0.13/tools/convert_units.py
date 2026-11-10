#!/usr/bin/env python3
"""Converte todas as folhas de unidades da facção inimiga com UMA paleta comum de 15 cores (grupo 1 dos sprites).
Cada folha: grade (linhas x 4 colunas), fundo transparente; colunas = quadros (caminhada) ou direcoes (poses de trabalho).
uso: convert_units.py saida_dir  folha1.webp:linhas:tamanho:altura ...   (ordem: sword peasant archer spear knight ram)
saidas: saida_dir/<nome>_idx.npy (indice -1=transparente), palette.npy (15x3), preview.png"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
K=15
def load(p):
    im=Image.open(p).convert('RGBA');return im,np.array(im)
def kmeans(px,w,rng):
    S=px[rng.choice(len(px),size=min(120000,len(px)),p=w/w.sum())]
    cent=[S[0]]
    for _ in range(K-1):
        d=np.min(((S[:,None,:]-np.array(cent)[None])**2).sum(2),axis=1);cent.append(S[rng.choice(len(S),p=d/d.sum())])
    cent=np.array(cent)
    for _ in range(30):
        l=((S[:,None,:]-cent[None])**2).sum(2).argmin(1)
        for k in range(K):
            if (l==k).any():cent[k]=S[l==k].mean(0)
    return cent[np.argsort(cent.sum(1))]
def label(a,cent):
    rgb=a[...,:3].astype(float);H,W=a.shape[:2];lab=np.full((H,W),-1,np.int16)
    for y0 in range(0,H,100):
        blk=rgb[y0:y0+100].reshape(-1,3);lab[y0:y0+100]=((blk[:,None,:]-cent[None])**2).sum(2).argmin(1).reshape(-1,W)
    lab[a[...,3]<=128]=-1;return lab
def sheet_to_idx(a,lab,rows,SZ,HM):
    H,W=a.shape[:2];cw=W//4;ch=H//rows;op=a[...,3]>128;boxes=[]
    for r in range(rows):
        for c in range(4):
            ys,xs=np.nonzero(op[r*ch:(r+1)*ch,c*cw:(c+1)*cw]);boxes.append((c*cw+xs.min(),r*ch+ys.min(),c*cw+xs.max()+1,r*ch+ys.max()+1))
    sc=HM/max(max(b[3]-b[1] for b in boxes),max(b[2]-b[0] for b in boxes))
    res=np.full((rows*SZ,4*SZ),-1,np.int16)
    for n,b in enumerate(boxes):
        sw=b[2]-b[0];sh=b[3]-b[1];tw=max(1,round(sw*sc));th=max(1,round(sh*sc))
        ox=(n%4)*SZ+(SZ-tw)//2;oy=(n//4)*SZ+SZ-1-th
        for ty in range(th):
            for tx in range(tw):
                y0=b[1]+int(ty*sh/th);y1=b[1]+int((ty+1)*sh/th);x0=b[0]+int(tx*sw/tw);x1=b[0]+int((tx+1)*sw/tw)
                bl=lab[y0:y1,x0:x1].ravel();vis=bl[bl>=0]
                if len(vis)<0.45*bl.size:continue
                cnt=np.bincount(vis,minlength=K)
                if cnt[0]>=0.30*len(vis):res[oy+ty,ox+tx]=0
                else:cnt[0]=0;res[oy+ty,ox+tx]=cnt.argmax()
    # remove pixels isoladas (sem vizinho opaco em 8 direcoes): ruido da geracao
    pad=np.pad(res>=0,1);nb=sum(pad[1+dy:1+dy+res.shape[0],1+dx:1+dx+res.shape[1]].astype(int) for dy in (-1,0,1) for dx in (-1,0,1) if dy or dx)
    res[(res>=0)&(nb==0)]=-1
    return res
if __name__=='__main__':
    out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
    specs=[a.split(':') for a in sys.argv[2:]];names=['sword','peasant','archer','spear','knight','ram']
    data=[load(s[0]) for s in specs];rng=np.random.default_rng(1)
    px=np.concatenate([d[1][...,:3][d[1][...,3]>128].astype(float) for d in data])
    sat=(px.max(1)-px.min(1))/(px.max(1)+1e-6);cent=kmeans(px,1+5*sat,rng)
    np.save(out/'palette.npy',np.clip(cent,0,255).astype(np.uint8))
    bg=(0x4a,0x70,0x50);pal=np.clip(cent,0,255).astype(np.uint8);panels=[]
    for name,(im,a),s in zip(names,data,specs):
        rows,SZ,HM=int(s[1]),int(s[2]),float(s[3])
        res=sheet_to_idx(a,label(a,cent),rows,SZ,HM);np.save(out/f'{name}_idx.npy',res)
        rgba=np.zeros(res.shape+(4,),np.uint8)
        for k in range(K):rgba[res==k]=(*pal[k],255)
        g=Image.new('RGBA',res.shape[::-1],bg+(255,));g.alpha_composite(Image.fromarray(rgba));panels.append(g.convert('RGB').resize((res.shape[1]*3,res.shape[0]*3),Image.NEAREST))
    W=max(p.width for p in panels);Hh=sum(p.height+6 for p in panels)
    m=Image.new('RGB',(W,Hh),(30,30,30));y=0
    for p in panels:m.paste(p,(0,y));y+=p.height+6
    m.save(out/'preview.png');print('cores',[tuple(int(v) for v in c) for c in pal])
