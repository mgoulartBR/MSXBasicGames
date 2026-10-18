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
    blob = open(sys.argv[3], 'rb').read()          # bancos de assets (16 KB cada), a partir do banco 2 (offset 0x8000 do arquivo)
    img += blob
    size = 0x10000
    while size < len(img): size *= 2
    img += b'\xff' * (size - len(img))
open(sys.argv[2], 'wb').write(img)
print('ROM: %d bytes de codigo+tabelas (max 32768); arquivo de %d KB (mapper ASCII16)' % (top, len(img) // 1024))
