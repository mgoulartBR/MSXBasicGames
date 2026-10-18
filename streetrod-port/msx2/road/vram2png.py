#!/usr/bin/env python3
"""Converte um dump de VRAM (Screen 5) em PNG. uso: vram2png.py vram.bin r2 out.png"""
import sys, re
from PIL import Image
vram = open(sys.argv[1], 'rb').read()
r2 = int(sys.argv[2])
page = (r2 >> 5) & 3
garage = len(sys.argv) > 4 and sys.argv[4] == 'garage'
src = open('assets.h' if garage else 'tables.h').read()
pal_bytes = list(map(int, re.search(r'pal_garage\[32\] = \{([^}]*)\}' if garage else r'palette\[32\] = \{([^}]*)\}', src).group(1).split(',')))
pal = []
for i in range(16):
    b0, g = pal_bytes[2*i], pal_bytes[2*i+1]
    r, b = (b0 >> 4) & 7, b0 & 7
    pal.append(tuple(int(v * 255 / 7) for v in (r, g, b)))
base = page * 0x8000
img = Image.new('RGB', (256, 212))
px = img.load()
for y in range(212):
    row = vram[base + y*128: base + y*128 + 128]
    for x in range(128):
        v = row[x]
        px[2*x, y] = pal[v >> 4]; px[2*x+1, y] = pal[v & 15]
if 'spr' in sys.argv[4:]:        # sprites modo 2 (16x16): tabela de atributos 0x7600, cores 0x7400, padroes 0x7800; plano menor por cima
    nmax = next((k for k in range(32) if vram[0x7600 + 4*k] == 0xD8), 32)
    for n in range(nmax - 1, -1, -1):
        y, x, pt = vram[0x7600 + 4*n], vram[0x7601 + 4*n], vram[0x7602 + 4*n]
        
        for ly in range(16):
            col = vram[0x7400 + 16*n + ly] & 15
            if not col: continue
            for lx in range(16):
                q = (pt & 0xFC) * 8 + ly % 8 + (8 if ly >= 8 else 0) + (16 if lx >= 8 else 0)
                if vram[0x7800 + q] >> (7 - lx % 8) & 1:
                    X, Y = x + lx, (y + 1 + ly) & 255
                    if X < 256 and Y < 212: px[X, Y] = pal[col]
img.resize((768, 636), Image.NEAREST).save(sys.argv[3])
