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
RED=idx_of((0x94,0x4A,0x21));GOLD=idx_of((0xDE,0x94,0x4A))
def convert(path,size,ss=4,cuts=(0.10,0.32,0.62,0.9)):
    im=Image.open(path).convert('RGBA');al=np.array(im)[...,3]>128
    ys,xs=np.nonzero(al);box=(xs.min(),ys.min(),xs.max()+1,ys.max()+1);sw,sh=box[2]-box[0],box[3]-box[1]
    sc=min((size-2)/sw,(size-1)/sh);tw,th=round(sw*sc),round(sh*sc)
    big=im.crop(box).resize((tw*ss,th*ss),Image.LANCZOS);a_=big.split()[3]
    big=big.convert('RGB').filter(ImageFilter.MedianFilter(5)).filter(ImageFilter.GaussianBlur(1.2));b=np.array(big);op=np.array(a_)>128
    L=lab(b.reshape(-1,3)).reshape(b.shape);Ll=L[...,0]
    red=(L[...,1]>18)&(Ll<55)
    q=np.quantile(Ll[op&~red],cuts);k=np.digitize(Ll,q)
    ramp=np.array(RAMP);idx=ramp[k];idx[red&op]=RED
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
