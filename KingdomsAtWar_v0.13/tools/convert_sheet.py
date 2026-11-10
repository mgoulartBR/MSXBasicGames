#!/usr/bin/env python3
"""Converte uma folha de sprites gerada por IA (grade 4x4: baixo, direita, cima, esquerda x 4 quadros de caminhada,
fundo transparente) em poses 32x32 de ate 15 cores + transparente, preservando contorno e cores raras (ouro, turquesa).
uso: convert_sheet.py folha.webp saida_prefixo [tamanho=32] [altura_max=30]
saidas: <prefixo>_32.png (folha 128x128 RGBA), <prefixo>_idx.npy (indices, -1 = transparente), <prefixo>_pal.npy,
        <prefixo>_preview.png (x6 sobre o verde do jogo, ao lado do original)"""
import sys
import numpy as np
from PIL import Image
src,pre=sys.argv[1],sys.argv[2];SZ=int(sys.argv[3]) if len(sys.argv)>3 else 32;HM=float(sys.argv[4]) if len(sys.argv)>4 else SZ-2
K=15
im=Image.open(src).convert('RGBA');A=np.array(im);H,W=A.shape[:2];cw=W//4;ch=H//4
op=A[...,3]>128;rgb=A[...,:3].astype(float);px=rgb[op]
sat=(px.max(1)-px.min(1))/(px.max(1)+1e-6);w=1+5*sat
rng=np.random.default_rng(1)
S=px[rng.choice(len(px),size=min(60000,len(px)),p=w/w.sum())]
cent=[S[0]]
for _ in range(K-1):
    d=np.min(((S[:,None,:]-np.array(cent)[None])**2).sum(2),axis=1);cent.append(S[rng.choice(len(S),p=d/d.sum())])
cent=np.array(cent)
for _ in range(25):
    l=((S[:,None,:]-cent[None])**2).sum(2).argmin(1)
    for k in range(K):
        if (l==k).any():cent[k]=S[l==k].mean(0)
cent=cent[np.argsort(cent.sum(1))]
lab=np.full((H,W),-1,np.int16)
for y0 in range(0,H,100):
    blk=rgb[y0:y0+100].reshape(-1,3);lab[y0:y0+100]=((blk[:,None,:]-cent[None])**2).sum(2).argmin(1).reshape(-1,W)
lab[~op]=-1
boxes=[]
for r in range(4):
    for c in range(4):
        ys,xs=np.nonzero(op[r*ch:(r+1)*ch,c*cw:(c+1)*cw]);boxes.append((c*cw+xs.min(),r*ch+ys.min(),c*cw+xs.max()+1,r*ch+ys.max()+1))
sc=HM/max(max(b[3]-b[1] for b in boxes),max(b[2]-b[0] for b in boxes))
res=np.full((4*SZ,4*SZ),-1,np.int16)
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
pal=np.clip(cent,0,255).astype(np.uint8)
out=np.zeros((4*SZ,4*SZ,4),np.uint8)
for k in range(K):out[res==k]=(*pal[k],255)
Image.fromarray(out).save(pre+'_32.png');np.save(pre+'_idx.npy',res);np.save(pre+'_pal.npy',pal)
S2=6;bg=(0x4a,0x70,0x50);n=4*SZ
g=Image.new('RGBA',(n,n),bg+(255,));g.alpha_composite(Image.fromarray(out))
v=Image.new('RGB',(n*S2*2+20,n*S2),(30,30,30));v.paste(g.convert('RGB').resize((n*S2,n*S2),Image.NEAREST),(0,0))
o=Image.new('RGBA',im.size,bg+(255,));o.alpha_composite(im);v.paste(o.convert('RGB').resize((n*S2,n*S2),Image.LANCZOS),(n*S2+20,0));v.save(pre+'_preview.png')
print('cores:',[tuple(int(x) for x in c) for c in pal])
