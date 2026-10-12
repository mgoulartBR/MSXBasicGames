#!/usr/bin/env python3
"""Gera zones.inc (dados SINTÉTICOS e originais) para o protótipo paleta + zonas R#27.

uso: gen_zones.py [ILADJ]   (ILADJ: ajuste fino da linha da interrupção; padrão -1)
"""
import sys

ILADJ = int(sys.argv[1]) if len(sys.argv) > 1 else -1
ROWS, ROADY = 112, 100

# meia largura da pista por linha (0 = horizonte)
hw = [min(127, int(6 + r * 1.08)) for r in range(ROWS)]

# subfaixa (0..3) de listra por linha: período de 4 subfaixas, perspectiva 1/z.
# Perto do horizonte as faixas ficam mais finas que 1 linha: usa-se r%4 (aspecto de névoa).
K, R0 = 1416.0, 8
sb = []
for r in range(ROWS):
    t = (r + R0) ** 2 / K                      # linhas por subfaixa
    sb.append(r % 4 if t < 1.2 else int(K / (r + R0)) % 4)

# zonas de scroll: mais finas perto do horizonte
heights = [4] * 10 + [8] * 5 + [16] * 2
assert sum(heights) == ROWS
zones, off = [], 0
for h in heights:
    zones.append((ROADY + off, h, off + h / 2))
    off += h
ZN = len(zones)
il = [y - 1 + ILADJ for y, _, _ in zones]
zt2 = []
for k, (y, h, rc) in enumerate(zones):
    q = int(255 * ((ROWS - 1 - rc) / (ROWS - 1)) ** 2)
    zt2 += [q, il[k + 1] if k + 1 < ZN else 0]

# perfil de curvatura sintético (64 passos, suavizado)
segs = [(0, 10), (80, 8), (0, 6), (-100, 10), (0, 8), (50, 6), (-50, 6), (0, 10)]
raw = [v for v, n in segs for _ in range(n)]
curve = [max(-127, min(127, int(round(sum(raw[(i + d) % 64] for d in range(-3, 4)) / 7))))
         for i in range(64)]

# paleta: 0 preto, 1 céu, 2 névoa, 3 grama; 4-7 asfalto, 8-11 zebra, 12-15 faixa central
L, D, RED, W = (4, 4, 4), (3, 3, 3), (6, 1, 1), (7, 7, 7)
pats = [[L, L, D, D], [RED, RED, W, W], [W, W, D, D]]
base = [(0, 0, 0), (2, 4, 7), (5, 6, 7), (1, 5, 1)]


def rgb(c):
    return [(c[0] << 4) | c[2], c[1]]


def phase(p):
    return [x for pat in pats for i in range(4) for x in rgb(pat[(i + p) & 3])]


palbase = [x for c in base for x in rgb(c)] + phase(0)
paltab = [x for p in range(4) for x in phase(p) + [0] * 8]      # 4 x 32 bytes


def db(name, vals, per=16):
    out = [name + ":"]
    for i in range(0, len(vals), per):
        out.append("        defb " + ",".join(str(v & 0xFF) for v in vals[i:i + per]))
    return out


lines = ["; gerado por gen_zones.py - dados sintéticos",
         "ZN      equ %d" % ZN, "IL0     equ %d" % il[0]]
lines += db("hwtab", hw) + db("sbt", sb) + db("zt2", zt2, 8) + db("curvetab", curve)
lines += db("palbase", palbase) + db("paltab", paltab)
open("zones.inc", "w").write("\n".join(lines) + "\n")
print("zones.inc: zonas=%d il0=%d" % (ZN, il[0]), "linhas:", [y for y, _, _ in zones])
