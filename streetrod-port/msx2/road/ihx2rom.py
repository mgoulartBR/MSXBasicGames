#!/usr/bin/env python3
"""Converte Intel HEX do SDCC em ROM de 32 KB mapeada em 0x4000 (codigo+tabelas a partir de 0x4000; assets.bin, se houver, no fim: ate 0xBFFF)."""
import sys, os
img = bytearray(b'\xff' * 0x8000)
top = 0
for line in open(sys.argv[1]):
    line = line.strip()
    if not line.startswith(':'): continue
    n = int(line[1:3], 16); addr = int(line[3:7], 16); typ = int(line[7:9], 16)
    if typ != 0: continue
    for i in range(n):
        a = addr + i
        if a < 0x4000 or a >= 0xC000: raise SystemExit('endereco fora da ROM: %04X' % a)
        img[a - 0x4000] = int(line[9 + 2 * i:11 + 2 * i], 16); top = max(top, a - 0x4000 + 1)

if len(sys.argv) > 3 and os.path.exists(sys.argv[3]):
    blob = open(sys.argv[3], 'rb').read(); base = 0xC000 - len(blob)
    if top > base - 0x4000: raise SystemExit('codigo (ate %04X) invade os assets (a partir de %04X)' % (top + 0x4000, base))
    img[base - 0x4000:] = blob
open(sys.argv[2], 'wb').write(img)
print('ROM: %d bytes usados de 32768 (codigo+tabelas; assets no fim)' % top)
