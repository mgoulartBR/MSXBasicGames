#!/usr/bin/env python3
"""Variantes de legibilidade da facção inimiga, aplicadas aos indices/paleta convertidos (sem custo em jogo):
  contorno: 1 px externo na cor mais escura da paleta (sombra das proprias cores, nao preto puro)
  contraste: curva de luminosidade na paleta (escuros mais escuros, claros mais claros) e pouco mais de croma
uso: style_variants.py dir_entrada dir_saida [contorno] [contraste]"""
import sys,shutil,colorsys
from pathlib import Path
import numpy as np
src,dst=Path(sys.argv[1]),Path(sys.argv[2]);opts=set(sys.argv[3:]);dst.mkdir(parents=True,exist_ok=True)
SZ={'sword':32,'peasant':32,'archer':32,'spear':48,'knight':48,'ram':48}
def dil(m):
    p=np.pad(m,1);o=np.zeros_like(m)
    for dy in (-1,0,1):
        for dx in (-1,0,1):o|=p[1+dy:1+dy+m.shape[0],1+dx:1+dx+m.shape[1]]
    return o
pal=np.load(src/'palette.npy').astype(float)
dark=int(pal.sum(1).argmin())
for n,sz in SZ.items():
    idx=np.load(src/f'{n}_idx.npy').copy()
    if 'contorno' in opts:
        H,W=idx.shape
        for cy in range(H//sz):
            for cx in range(W//4//sz*0+4):
                cell=idx[cy*sz:(cy+1)*sz,cx*sz:(cx+1)*sz];m=cell>=0;ring=dil(m)&~m;cell[ring]=dark
    np.save(dst/f'{n}_idx.npy',idx)
if 'contraste' in opts:
    out=[]
    for c in pal:
        h,s,v=colorsys.rgb_to_hsv(*(c/255));v=min(1,max(0,(v-0.40)*1.5+0.44));s=min(1,s*1.12);out.append([x*255 for x in colorsys.hsv_to_rgb(h,s,v)])
    pal=np.array(out)
np.save(dst/'palette.npy',np.clip(pal,0,255).astype(np.uint8))
