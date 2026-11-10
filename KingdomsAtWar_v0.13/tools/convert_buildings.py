#!/usr/bin/env python3
"""Converte imagens de prédios (fundo transparente) para tiles do jogo com a paleta FIXA de 15 cores (paleta do terreno).
Pipeline: recorta, reduz a 4x o tamanho final (LANCZOS), agrupa as cores em 15 regioes (k-means em Lab), liga cada regiao a uma cor
da paleta (atribuicao gulosa por distancia, sem repetir enquanto houver cores livres), e reduz por moda de bloco com preferencia por
cores de destaque. Menos ruido que o mapeamento pixel a pixel.
uso: convert_buildings.py saida_dir  imagem.webp:tamanho ..."""
import sys,re
from pathlib import Path
import numpy as np
from PIL import Image
r=Path(__file__).resolve().parents[1]
t=(r/'src/art07.h').read_text()
hp=list(map(int,re.search(r'palette9\[\]=\{([^}]+)',t)[1].split(',')))
HUM=np.array([[min(255,round(v*255/31)) for v in hp[i*3:i*3+3]] for i in range(16)],float)
def lab(c):
    c=np.asarray(c,float)/255;c=np.where(c>0.04045,((c+0.055)/1.055)**2.4,c/12.92)
    X=c@np.array([[0.4124,0.2126,0.0193],[0.3576,0.7152,0.1192],[0.1805,0.0722,0.9505]])/np.array([0.95047,1,1.08883])
    f=np.where(X>0.008856,np.cbrt(X),7.787*X+16/116);return np.stack([116*f[...,1]-16,500*(f[...,0]-f[...,1]),200*(f[...,1]-f[...,2])],-1)
PL=lab(HUM[1:])
def kmeans(S,K,rng):
    cent=[S[0]]
    for _ in range(K-1):
        d=np.min(((S[:,None,:]-np.array(cent)[None])**2).sum(2),axis=1);cent.append(S[rng.choice(len(S),p=d/d.sum())])
    cent=np.array(cent)
    for _ in range(20):
        l=((S[:,None,:]-cent[None])**2).sum(2).argmin(1)
        for k in range(K):
            if (l==k).any():cent[k]=S[l==k].mean(0)
    return cent
def convert(path,size,margin=1,ss=4,smooth=0):
    im=Image.open(path).convert('RGBA');a=np.array(im);al=a[...,3]>128
    ys,xs=np.nonzero(al);box=(xs.min(),ys.min(),xs.max()+1,ys.max()+1);sw,sh=box[2]-box[0],box[3]-box[1]
    sc=min((size-2*margin)/sw,(size-margin)/sh);tw,th=round(sw*sc),round(sh*sc)
    big=im.crop(box).resize((tw*ss,th*ss),Image.LANCZOS)
    if smooth:
        from PIL import ImageFilter
        a_=big.split()[3];big=big.convert('RGB').filter(ImageFilter.MedianFilter(2*smooth+1)).filter(ImageFilter.SMOOTH_MORE);big.putalpha(a_)
    b=np.array(big);op=b[...,3]>128
    L=lab(b[...,:3].reshape(-1,3)).reshape(b.shape[:2]+(3,))
    px=L[op];rng=np.random.default_rng(1);S=px[rng.choice(len(px),size=min(30000,len(px)),replace=False)]
    cent=kmeans(S,15,rng)
    lab_c=((L.reshape(-1,3)[:,None,:]-cent[None])**2).sum(2).argmin(1).reshape(L.shape[:2])
    sizes=np.bincount(lab_c[op],minlength=15);order=np.argsort(-sizes);used=set();amap=[0]*15
    for c in order:
        d=((PL-cent[c])**2).sum(1)
        for k in np.argsort(d):
            if k not in used:amap[c]=k+1;used.add(k);break
        else:amap[c]=int(d.argmin())+1
    idx=np.array(amap)[lab_c];idx[~op]=-1
    out=np.full((size,size),-1,int);ox=(size-tw)//2;oy=size-th
    for ty in range(th):
        for tx in range(tw):
            blk=idx[ty*ss:(ty+1)*ss,tx*ss:(tx+1)*ss].ravel();vis=blk[blk>=0]
            if len(vis)<0.45*blk.size:continue
            cnt=np.bincount(vis,minlength=16);out[oy+ty,ox+tx]=cnt.argmax()
    if smooth:
        for _ in range(smooth):
            o2=out.copy()
            for y in range(1,size-1):
                for x in range(1,size-1):
                    c=out[y,x]
                    if c<0:continue
                    nb=[out[y+dy,x+dx] for dy in(-1,0,1) for dx in(-1,0,1) if (dy or dx)]
                    if nb.count(c)<=1:
                        v=[n for n in nb if n>=0]
                        if v:o2[y,x]=np.bincount(v).argmax()
            out=o2
    return out
if __name__=='__main__':
    out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
    for spec in sys.argv[2:]:
        p,s=spec.split(':');idx=convert(p,int(s),smooth=int(__import__('os').environ.get('SMOOTH','0')));np.save(out/(Path(p).stem+'_idx.npy'),idx)
