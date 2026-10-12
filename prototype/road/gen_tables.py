#!/usr/bin/env python3
"""Gera tables.inc (dados SINTÉTICOS e originais) para o protótipo de pista MSX2+."""
import math

ROWS, BANDS = 112, 56                      # linhas de pista / bandas de 2 linhas

# largura (meia pista, px) por linha da textura: 0 = horizonte, 111 = base da tela
hw = [min(127, int(6 + r * 1.08)) for r in range(ROWS)]

# fase (8.8) das listras por banda: z ~ 1/profundidade; k=0 é a banda mais distante
zt = [int(150000 / (2 * k + 14)) & 0xFFFF for k in range(BANDS)]

# fator de deslocamento da curva por banda (0..255): maior no horizonte
qt = [int(255 * ((BANDS - 1 - k) / (BANDS - 1)) ** 2) for k in range(BANDS)]

# perfil de curvatura da pista sintética: 64 passos, suavizado
segs = [(0, 10), (80, 8), (0, 6), (-100, 10), (0, 8), (50, 6), (-50, 6), (0, 10)]
raw = [v for v, n in segs for _ in range(n)]
assert len(raw) == 64
curve = []
for i in range(64):
    s = sum(raw[(i + d) % 64] for d in range(-3, 4)) / 7
    curve.append(max(-127, min(127, int(round(s)))))

# paleta (R,G,B 0..7)
pal = [(0, 0, 0), (2, 4, 7), (1, 5, 1), (1, 4, 1), (4, 4, 4), (3, 3, 3), (6, 1, 1),
       (7, 7, 7), (5, 0, 0), (4, 5, 7), (6, 6, 7), (2, 2, 3), (0, 0, 0), (0, 0, 0),
       (0, 0, 0), (7, 7, 7)]


def db(name, vals, per=16):
    out = [name + ":"]
    for i in range(0, len(vals), per):
        out.append("        defb " + ",".join(str(v & 0xFF) for v in vals[i:i + per]))
    return out


def dw(name, vals, per=8):
    out = [name + ":"]
    for i in range(0, len(vals), per):
        out.append("        defw " + ",".join(str(v) for v in vals[i:i + per]))
    return out


lines = ["; gerado por gen_tables.py - dados sintéticos"]
lines += db("hwtab", hw) + dw("zt", zt) + db("qt", qt) + db("curvetab", curve)
lines += db("palette", [b for r, g, bl in pal for b in ((r << 4) | bl, g)])
open("tables.inc", "w").write("\n".join(lines) + "\n")
print("tables.inc gerado:", len(hw), len(zt), len(qt), len(curve))
