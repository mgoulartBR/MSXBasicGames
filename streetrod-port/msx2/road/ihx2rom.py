#!/usr/bin/env python3
"""Converte Intel HEX do SDCC na ROM ASCII16.
Banco 0 = nucleo em 0x4000-0x7FFF (pagina 1, fixo). Bancos de codigo 1..ABASE-1: enderecos virtuais (banco<<16)|0x8000 -> janela 0x8000-0xBFFF.
Bancos ABASE.. = assets.bin (16 KB cada). Arquivo preenchido ate potencia de 2 (>= 64 KB)."""
import sys, os
ABASE = int(os.environ.get('ASSET_BANK0', '6'))
img = bytearray(b'\xff' * (ABASE * 0x4000))
used = {}
ext = 0
for line in open(sys.argv[1]):
    line = line.strip()
    if not line.startswith(':'): continue
    n = int(line[1:3], 16); addr = int(line[3:7], 16); typ = int(line[7:9], 16)
    if typ == 4: ext = int(line[9:13], 16) << 16; continue
    if typ != 0: continue
    for i in range(n):
        a = ext + addr + i
        bank, lo = a >> 16, a & 0xFFFF
        if bank == 0:
            if not 0x4000 <= lo < 0x8000: raise SystemExit('nucleo fora da pagina 1: %05X (cresceu demais?)' % a)
            off = lo - 0x4000
        else:
            if not 0x8000 <= lo < 0xC000: raise SystemExit('banco %d estourou 16 KB: endereco %04X' % (bank, lo))
            if bank >= ABASE: raise SystemExit('banco de codigo %d colide com os assets (ASSET_BANK0=%d)' % (bank, ABASE))
            off = bank * 0x4000 + lo - 0x8000
        img[off] = int(line[9 + 2 * i:11 + 2 * i], 16); used[bank] = max(used.get(bank, 0), (lo & 0x3FFF) + 1)
code_end = len(img)
if len(sys.argv) > 3 and os.path.exists(sys.argv[3]):
    img += open(sys.argv[3], 'rb').read()
size = 0x10000
while size < len(img): size *= 2
img += b'\xff' * (size - len(img))
# deteccao automatica de mapper dos emuladores: conta `LD (nn),A` (32 lo hi) para enderecos de registradores de banco. Para ser reconhecido como
# ASCII16 so podem existir 0x6000/0x7000; padroes casuais nos dados (ex.: tabela de precos 50,80 = 32 00 50 00) confundem com Konami/ASCII8.
bad = []
for a_ in (0x5000, 0x9000, 0xB000, 0x6800, 0x7800, 0x5800, 0x9800, 0xB800):
    pat = bytes([0x32, a_ & 255, a_ >> 8]); i_ = img.find(pat, 0, code_end)
    while i_ >= 0:
        bad.append('banco %d +%04X: LD (%04X),A' % (i_ // 0x4000, i_ % 0x4000, a_)); i_ = img.find(pat, i_ + 1, 0, code_end) if False else img.find(pat, i_ + 1, code_end)
if bad: raise SystemExit('padrao que confunde a deteccao de mapper: ' + ', '.join(bad))
open(sys.argv[2], 'wb').write(img)
print('ROM: ' + ', '.join('%s %d B' % ('nucleo' if b == 0 else 'banco %d' % b, u) for b, u in sorted(used.items())) + '; arquivo de %d KB (mapper ASCII16)' % (len(img) // 1024))
