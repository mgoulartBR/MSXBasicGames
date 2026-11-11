#!/usr/bin/env python3
"""Conversao de predios por rampa de luminosidade: preserva luz/sombra com poucos tons da paleta do terreno.
uso: convert_ramp.py saida_dir imagem:tamanho ..."""
import sys,os
from pathlib import Path
import numpy as np
from PIL import Image,ImageFilter
sys.path.insert(0,str(Path(__file__).parent))
from convert_buildings import HUM,lab
def idx_of(rgb):return int(((HUM[1:]-np.array(rgb))**2).sum(1).argmin())+1
RAMP=[idx_of(c) for c in [(0x21,0x21,0x21),(0x4A,0x21,0x21),(0x6B,0x4A,0x21),(0xB5,0x94,0x4A),(0xDE,0xDE,0x94)]]
GREY=[idx_of(c) for c in [(0x4A,0x4A,0x4A),(0x94,0x94,0x94),(0xDE,0xDE,0xDE),(0xFF,0xFF,0xDE)]]
RED=idx_of((0x94,0x4A,0x21));GOLD=idx_of((0xDE,0x94,0x4A))
def keybg(im):
    a=np.array(im)
    if (a[...,3]<250).any():return im
    from collections import deque
    rgb=a[...,:3].astype(int);bgc=(rgb.min(2)>215)&((rgb.max(2)-rgb.min(2))<14)
    h,w=bgc.shape;seen=np.zeros_like(bgc);q=deque()
    for x in range(w):
        for y in (0,h-1):
            if bgc[y,x]:q.append((y,x));seen[y,x]=1
    for y in range(h):
        for x in (0,w-1):
            if bgc[y,x] and not seen[y,x]:q.append((y,x));seen[y,x]=1
    while q:
        y,x=q.popleft()
        for dy,dx in((1,0),(-1,0),(0,1),(0,-1)):
            ny,nx=y+dy,x+dx
            if 0<=ny<h and 0<=nx<w and bgc[ny,nx] and not seen[ny,nx]:seen[ny,nx]=1;q.append((ny,nx))
    a[seen,3]=0;return Image.fromarray(a)
def convert(path,size,ss=4,cuts=(0.10,0.32,0.62,0.9)):
    im=keybg(Image.open(path).convert('RGBA'));al=np.array(im)[...,3]>128
    ys,xs=np.nonzero(al);box=(xs.min(),ys.min(),xs.max()+1,ys.max()+1);sw,sh=box[2]-box[0],box[3]-box[1]
    sc=min((size-2)/sw,(size-1)/sh);tw,th=round(sw*sc),round(sh*sc)
    big=im.crop(box).resize((tw*ss,th*ss),Image.LANCZOS);a_=big.split()[3]
    big=big.convert('RGB').filter(ImageFilter.MedianFilter(5)).filter(ImageFilter.GaussianBlur(1.2));b=np.array(big);op=np.array(a_)>128
    L=lab(b.reshape(-1,3)).reshape(b.shape);Ll=L[...,0]
    red=(L[...,1]>18)&(Ll<55)
    ch=np.hypot(L[...,1],L[...,2]);metal=(ch<float(os.environ.get('METALCH','11')))&(Ll>30)&op&~red if os.environ.get('METAL','1')=='1' else np.zeros_like(op)
    q=np.quantile(Ll[op&~red&~metal],cuts);k=np.digitize(Ll,q)
    ramp=np.array(RAMP);idx=ramp[k];idx[red&op]=RED
    gk=np.digitize(Ll,[45,68,86]);idx[metal]=np.array(GREY)[gk][metal]
    ac=(L[...,1]>8)&(L[...,2]>35)&(Ll>55)&(Ll<80);idx[ac&op]=np.where(k[ac&op]>=3,GOLD,idx[ac&op])
    idx[~op]=-1;out=np.full((size,size),-1,int);ox=(size-tw)//2;oy=size-th
    for ty in range(th):
        for tx in range(tw):
            blk=idx[ty*ss:(ty+1)*ss,tx*ss:(tx+1)*ss].ravel();v=blk[blk>=0]
            if len(v)<0.5*blk.size:continue
            out[oy+ty,ox+tx]=np.bincount(v,minlength=16).argmax()
    return out
if __name__=='__main__':
    o=Path(sys.argv[1]);o.mkdir(parents=True,exist_ok=True)
    for sp in sys.argv[2:]:
        p,s=sp.rsplit(':',1);np.save(o/(Path(p).stem+'_idx.npy'),convert(p,int(s)))
