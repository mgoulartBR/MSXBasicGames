#!/usr/bin/env python3
"""Gera cars.inc (dados SINTÉTICOS e originais): textura, zonas, paleta e SPRITES de carros.

Arte dos carros: desenho original (carro de corrida genérico visto por trás), em 2 camadas
(escuro: asa/pneus/câmbio; corpo) por tamanho. uso: gen_cars.py [ILADJ]
"""
import math
import sys

ILADJ = int(sys.argv[1]) if len(sys.argv) > 1 else -1
ROWS, ROADY, TEX, BASE = 112, 100, 132, 16      # TEX 132: linhas 232+ da página 0 guardam as tabelas de sprites

hw = [max(2, min(127, int(6 + (i - BASE) * 1.08))) for i in range(TEX)]
K = 1416.0
sb = []
for i in range(TEX):
    u = i - 8
    t = (u * u) / K if u > 0 else 0
    sb.append(i % 4 if (u < 2 or t < 1.2) else int(K / u) % 4)

heights = [4] * 10 + [8] * 5 + [16] * 2
assert sum(heights) == ROWS
zones, off = [], 0
for h in heights:
    zones.append((ROADY + off, h, off + h / 2))
    off += h
ZN = len(zones)
il = [y - 1 + ILADJ for y, _, _ in zones]
zt3 = []
for k, (y, h, rc) in enumerate(zones):
    q = int(255 * ((ROWS - 1 - rc) / (ROWS - 1)) ** 2)
    ahead = int(0.5 + 48 * ((ROWS - rc) / ROWS) ** 1.6)
    zt3 += [q, ahead, il[k + 1] if k + 1 < ZN else 0]

# ---- paleta: 0 céu, 1 névoa, 2 grama, 3 carro azul, 4-7 asfalto (gira), 8-11 zebra (gira),
#              12 carro vermelho, 13 escuro, 14 carro amarelo, 15 branco (faixa central)
L, D, RED, W = (4, 4, 4), (3, 3, 3), (6, 1, 1), (7, 7, 7)
pats = [[L, L, D, D], [RED, RED, W, W]]
head = [(2, 4, 7), (5, 6, 7), (1, 5, 1), (1, 2, 6)]
tail = [(7, 1, 1), (1, 1, 1), (7, 6, 0), (7, 7, 7)]


def rgb(c):
    return [(c[0] << 4) | c[2], c[1]]


def phase(p):
    return [x for pat in pats for i in range(4) for x in rgb(pat[(i + p) & 3])]


palbase = [x for c in head for x in rgb(c)] + phase(0) + [x for c in tail for x in rgb(c)]
paltab = [x for p in range(4) for x in phase(p) + [0] * 16]            # 4 x 32 bytes (16 usados)

# ---- sprites (16x16) ---------------------------------------------------------------------
BODY = ["................", "................", "......####......", ".....######.....",
        ".....######.....", "....########....", "....########....", "....########....",
        "....########....", "....########....", "....########....", ".....######.....",
        ".....######.....", "....########....", "................", "................"]
DARK = [".##############.", ".##############.", "................", "................",
        "................", "................", "................", "###..........###",
        "####........####", "####........####", "####..####..####", "####..####..####",
        "####..####..####", "####........####", "####........####", ".###........###."]
DARK = [r.ljust(16, ".") for r in DARK]
assert all(len(r) == 16 for r in BODY + DARK) and len(BODY) == len(DARK) == 16


def scaled(art, s):
    """reduz a arte 16x16 para sxs (OR dos pixels do bloco), alinhada embaixo e centrada."""
    out = [[0] * 16 for _ in range(16)]
    ox, oy = (16 - s) // 2, 16 - s
    for y in range(s):
        for x in range(s):
            y0, y1 = y * 16 // s, max(y * 16 // s + 1, (y + 1) * 16 // s)
            x0, x1 = x * 16 // s, max(x * 16 // s + 1, (x + 1) * 16 // s)
            out[oy + y][ox + x] = int(any(art[yy][xx] == "#" for yy in range(y0, y1) for xx in range(x0, x1)))
    return out


def pattern_bytes(bits):
    left = [sum(bits[y][x] << (7 - x) for x in range(8)) for y in range(16)]
    right = [sum(bits[y][8 + x] << (7 - x) for x in range(8)) for y in range(16)]
    return left + right


SIZES = [16, 11, 7, 4]
sprpat = []
for s in SIZES:                               # slot 2*i = escuro, 2*i+1 = corpo
    sprpat += pattern_bytes(scaled(DARK, s)) + pattern_bytes(scaled(BODY, s))
assert len(sprpat) == 256
# cores por linha dos 6 planos (3 carros x escuro/corpo): vermelho, azul, amarelo
sprcol = []
for body in (12, 3, 14):
    sprcol += [13] * 16 + [body] * 16

# ---- projeção de carros: d (posições à frente, 1/4 de posição) -> linha de base, tamanho
ytab, sizetab = [], []
for dq in range(256):
    d = dq / 4.0
    rc = ROWS - ROWS * ((min(d, 48) / 48.0) ** (1 / 1.6)) if d > 0 else ROWS - 1
    ytab.append(max(ROADY, min(ROADY + ROWS - 1, int(round(ROADY + rc - 1)))))
    sizetab.append(8 * (0 if d < 8 else 1 if d < 18 else 2 if d < 30 else 3))
zline = []
for u in range(ROWS):
    zline.append(max(k for k, (y, h, rc) in enumerate(zones) if ROADY + u >= y))


def db(name, vals, per=16):
    out = [name + ":"]
    for i in range(0, len(vals), per):
        out.append("        defb " + ",".join(str(v & 0xFF) for v in vals[i:i + per]))
    return out


lines = ["; gerado por gen_cars.py - dados sintéticos", "ZN      equ %d" % ZN, "IL0     equ %d" % il[0],
         "TEXROWS equ %d" % TEX]
lines += db("hwtab", hw) + db("sbt", sb) + db("zt3", zt3, 9) + db("palbase", palbase) + db("paltab", paltab)
lines += db("sprpat", sprpat) + db("sprcol", sprcol) + db("ytab", ytab) + db("sizetab", sizetab) + db("zlinetab", zline)
open("cars.inc", "w").write("\n".join(lines) + "\n")
print("cars.inc: zonas=%d texrows=%d sprites=%d bytes" % (ZN, TEX, len(sprpat)))
