#!/usr/bin/env python3
"""Gera hills.inc (dados SINTÉTICOS e originais): pista com curvas E relevo.

Textura de 144 linhas (i = 0..143) em VRAM y = 100 + i. Com R#23 = 16 o
horizonte mostra i = 16 (pista plana). Cada zona soma um deslocamento vertical
dv (-16..+16) a R#23 para simular subidas/descidas.

uso: gen_hills.py [ILADJ]
"""
import sys

ILADJ = int(sys.argv[1]) if len(sys.argv) > 1 else -1
ROWS, ROADY, TEX, BASE = 112, 100, 144, 16

# meia largura por linha da textura (pista plana: 6 + 1.08*(i-BASE); acima do horizonte fica estreita)
hw = [max(2, min(127, int(6 + (i - BASE) * 1.08))) for i in range(TEX)]   # mín. 2: NX=0 no VDP significa 256 px

# subfaixa (0..3) por linha: profundidade u = i - 8; perto do horizonte vira "névoa" (i % 4)
K = 1416.0
sb = []
for i in range(TEX):
    u = i - 8
    t = (u * u) / K if u > 0 else 0
    sb.append(i % 4 if (u < 2 or t < 1.2) else int(K / u) % 4)

# zonas (mais finas perto do horizonte)
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


def smooth(raw, n=64, w=3):
    return [max(-127, min(127, int(round(sum(raw[(i + d) % n] for d in range(-w, w + 1)) / (2 * w + 1)))))
            for i in range(n)]


# perfil de curvatura e de relevo (64 passos cada), sintéticos
cseg = [(0, 10), (80, 8), (0, 6), (-100, 10), (0, 8), (50, 6), (-50, 6), (0, 10)]
curve = smooth([v for v, n in cseg for _ in range(n)])
hseg = [(0, 8), (16, 10), (0, 6), (-16, 12), (0, 8), (12, 8), (-12, 6), (0, 6)]
hill = smooth([v for v, n in hseg for _ in range(n)])
assert len(curve) == 64 and len(hill) == 64

# paleta (igual à v2)
L, D, RED, W = (4, 4, 4), (3, 3, 3), (6, 1, 1), (7, 7, 7)
pats = [[L, L, D, D], [RED, RED, W, W], [W, W, D, D]]
base = [(0, 0, 0), (2, 4, 7), (5, 6, 7), (1, 5, 1)]


def rgb(c):
    return [(c[0] << 4) | c[2], c[1]]


def phase(p):
    return [x for pat in pats for i in range(4) for x in rgb(pat[(i + p) & 3])]


palbase = [x for c in base for x in rgb(c)] + phase(0)
paltab = [x for p in range(4) for x in phase(p) + [0] * 8]


def db(name, vals, per=16):
    out = [name + ":"]
    for i in range(0, len(vals), per):
        out.append("        defb " + ",".join(str(v & 0xFF) for v in vals[i:i + per]))
    return out


lines = ["; gerado por gen_hills.py - dados sintéticos",
         "ZN      equ %d" % ZN, "IL0     equ %d" % il[0], "TEXROWS equ %d" % TEX]
lines += db("hwtab", hw) + db("sbt", sb) + db("zt2", zt2, 8) + db("curvetab", curve)
lines += db("hilltab", hill) + db("palbase", palbase) + db("paltab", paltab)
open("hills.inc", "w").write("\n".join(lines) + "\n")
print("hills.inc: zonas=%d texrows=%d il0=%d" % (ZN, TEX, il[0]))
