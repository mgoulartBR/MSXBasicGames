#!/usr/bin/env python3
"""Extrai os itens EGA (LIB1/LIB2) para PNG. Uso: extract_lib.py LIB1 saida_dir
Requer numpy e Pillow. Paleta aproximada da tela de titulo (o jogo troca paletas por tela)."""
import os, sys
import numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(__file__))
import srlib

PAL = np.array([(0,0,0),(0,0,0),(85,85,85),(170,170,170),(255,255,255),(170,170,170),(0,0,0),(85,255,255),
                (0,170,170),(0,170,0),(170,0,0),(255,85,85),(170,0,170),(85,255,85),(170,85,0),(255,255,85)], np.uint8)

def render(it):
    bw = (it.w + 7) // 8
    out = it.decode()
    n = 4 if len(out) == 4 * it.h * bw else 1   # flag 2: um plano so (mascara)
    a = np.frombuffer(out, np.uint8).reshape(n, it.h, bw)
    bits = np.unpackbits(a, axis=2)[:, :, :it.w]
    if n == 1:
        return np.where(bits[0][..., None] > 0, 255, 0).astype(np.uint8).repeat(3, axis=2)
    return PAL[bits[0] | (bits[1] << 1) | (bits[2] << 2) | (bits[3] << 3)]

if __name__ == '__main__':
    lib, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    for k, it in enumerate(srlib.load(lib)):
        Image.fromarray(render(it)).save(os.path.join(outdir, f'{k:03d}_{it.w}x{it.h}.png'))
