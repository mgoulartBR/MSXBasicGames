#!/usr/bin/env python3
"""Remove o fundo xadrez 'transparente' gravado nos pixels de uma imagem de canteiro (cinzas neutros de 2 tons) e
preenche so os buracos pequenos e fechados (falhas de argamassa na pedra); o vao aberto da armacao fica transparente.
uso: clean_canteiro.py entrada saida.png"""
import sys
from collections import deque
import numpy as np
from PIL import Image
src,dst=sys.argv[1:3]
im=Image.open(src).convert('RGBA');a=np.array(im).astype(int);h,w=a.shape[:2]
r,g,b=a[...,0],a[...,1],a[...,2]
neu=(abs(r-g)<=8)&(abs(g-b)<=8)
# 2 tons do xadrez: os dois valores neutros mais frequentes na borda da imagem
edge=np.concatenate([a[0,:,0],a[-1,:,0],a[:,0,0],a[:,-1,0]]);vals=np.bincount(edge.astype(int),minlength=256)
tones=[int(v) for v in np.argsort(-vals)[:6] if vals[v]>20]
chk=np.zeros_like(neu)
for t in tones:chk|=neu&(abs(r-t)<=6)
rem=chk.copy();grey=neu&(r>=40)
lo=min(tones);hi=max(tones);grey&=(r<=hi+20)
def nbr(m):
    o=m.copy();o[1:]|=m[:-1];o[:-1]|=m[1:];o[:,1:]|=m[:,:-1];o[:,:-1]|=m[:,1:];return o
for _ in range(3):rem|=nbr(rem)&grey
dark=neu&(r<=80)&~rem
def erode(m):
    o=m.copy();o[1:]&=m[:-1];o[:-1]&=m[1:];o[:,1:]&=m[:,:-1];o[:,:-1]&=m[:,1:];o[0]=0;o[-1]=0;o[:,0]=0;o[:,-1]=0;return o
e=dark
for _ in range(4):e=erode(e)
o=e
for _ in range(4):o=nbr(o)&dark
rem|=dark&~o
out=np.array(im);out[rem,3]=0
def comps(mask):
    lab=np.zeros(mask.shape,int);n=0;sizes={}
    for y0 in range(h):
        for x0 in range(w):
            if mask[y0,x0] and not lab[y0,x0]:
                n+=1;lab[y0,x0]=n;dq=deque([(y0,x0)]);c=0
                while dq:
                    y,x=dq.popleft();c+=1
                    for dy,dx in((1,0),(-1,0),(0,1),(0,-1)):
                        ny,nx=y+dy,x+dx
                        if 0<=ny<h and 0<=nx<w and mask[ny,nx] and not lab[ny,nx]:lab[ny,nx]=n;dq.append((ny,nx))
                sizes[n]=c
    return lab,sizes
lab,sizes=comps(out[...,3]>0);big=max(sizes,key=sizes.get);out[lab!=big,3]=0
# buracos fechados pequenos
opaque=out[...,3]>0;lab,sizes=comps(~opaque)
border=set(lab[0,:])|set(lab[-1,:])|set(lab[:,0])|set(lab[:,-1])
small=np.zeros_like(opaque)
for k,s in sizes.items():
    if k not in border and s<4000:small|=lab==k
dq=deque((y,x) for y,x in zip(*np.nonzero(opaque)) if (small[min(h-1,y+1),x] or small[max(0,y-1),x] or small[y,min(w-1,x+1)] or small[y,max(0,x-1)]))
known=opaque.copy();n=0
while dq:
    y,x=dq.popleft()
    for dy,dx in((1,0),(-1,0),(0,1),(0,-1)):
        ny,nx=y+dy,x+dx
        if 0<=ny<h and 0<=nx<w and small[ny,nx] and not known[ny,nx]:
            out[ny,nx,:3]=out[y,x,:3];out[ny,nx,3]=255;known[ny,nx]=1;dq.append((ny,nx));n+=1
print('tons do xadrez',tones,'buracos preenchidos',n)
Image.fromarray(out).save(dst)
