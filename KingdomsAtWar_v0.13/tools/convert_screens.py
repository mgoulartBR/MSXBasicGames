#!/usr/bin/env python3
"""Converte as telas (4:3) para 256x192 com a paleta de 16 cores do jogo (indice 0 = preto, 1..15 = paleta do terreno).
uso: convert_screens.py entrada.png... ; grava reference/telas/<nome>_idx.npy (192x256, uint8) e <nome>_prev.png (previa 3x)"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
r=Path(__file__).resolve().parents[1]
PAL=[(0,0,0)]+[tuple(int(h[i:i+2],16) for i in (0,2,4)) for h in '212121 214A21 4A6B4A 21214A 4A6B94 4A2121 944A21 6B4A21 DE944A B5944A DEDE94 4A4A4A 949494 DEDEDE FFFFDE'.split()]
def convert(path):
    im=Image.open(path).convert('RGB').resize((256,192),Image.LANCZOS)
    a0=np.array(im);a0[a0.max(2)<46]=(0x21,0x21,0x21);im=Image.fromarray(a0)  # fundo escuro chapado: sem ruido na faixa de texto
    p=Image.new('P',(1,1));flat=[c for rgb in PAL for c in rgb]+[0]*(768-48);p.putpalette(flat)
    q=im.quantize(palette=p,dither=Image.FLOYDSTEINBERG)
    a=np.array(q,dtype=np.uint8)
    name=Path(path).stem;out=r/'reference/telas'
    np.save(out/f'{name}_idx.npy',a)
    prev=np.array(PAL,dtype=np.uint8)[a];Image.fromarray(prev).resize((768,576),Image.NEAREST).save(out/f'{name}_prev.png')
    tiles={a[y:y+8,x:x+8].tobytes() for y in range(0,192,8) for x in range(0,256,8)}
    print(name,'tiles unicos 8x8:',len(tiles),'de 768')
for f in sys.argv[1:]:convert(f)
