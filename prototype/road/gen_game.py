#!/usr/bin/env python3
"""Gera gamedefs.inc/gamedata.inc (dados SINTÉTICOS e arte ORIGINAL) para o protótipo "game" (v6).

Layout (212 linhas):  0..55 céu | 56..83 skyline (scroll próprio) | 84..159 pista | 160..211 cockpit/HUD
Páginas de VRAM: pág. 0 = textura da pista; pág. 1 = céu/skyline (topo) + cockpit/HUD (base).
Cada região usa sua paleta (entradas 4..11 trocadas por interrupção de linha).
uso: gen_game.py [ILADJ]
"""
import math
import random
import sys

ILADJ = int(sys.argv[1]) if len(sys.argv) > 1 else -1
SKYY, ROADY, ROWS, COCKY = 56, 84, 76, 160
BASE, TEX = 16, 116                       # R#23 base e linhas da textura (VRAM y = ROADY + i)

# ---------------------------------------------------------------- pista (textura e zonas)
hw = [max(2, min(66, int(6 + (i - BASE) * 0.68))) for i in range(TEX)]
K = 689.0
sb = []
for i in range(TEX):
    u = i - 8
    t = (u * u) / K if u > 0 else 0
    sb.append(i % 4 if (u < 2 or t < 1.2) else int(K / u) % 4)

heights = [4] * 8 + [8] * 3 + [10] * 2
assert sum(heights) == ROWS
zones, off = [], 0
for h in heights:
    zones.append((ROADY + off, h, off + h / 2))
    off += h
ZN = len(zones)
il = [y - 1 + ILADJ for y, _, _ in zones] + [COCKY - 1 + ILADJ]
zt3 = []
for k, (y, h, rc) in enumerate(zones):
    q = int(255 * ((ROWS - 1 - rc) / (ROWS - 1)) ** 2)
    ahead = int(0.5 + 48 * ((ROWS - rc) / ROWS) ** 1.6)
    zt3 += [q, ahead, il[k + 1], hw[int(rc) + BASE]]
ILSKY, ILROAD0 = SKYY - 1 + ILADJ, il[0]

# ---------------------------------------------------------------- paletas
STATIC = {0: (2, 4, 7), 1: (5, 6, 7), 2: (1, 5, 1), 3: (1, 2, 6),
          12: (7, 1, 1), 13: (1, 1, 1), 14: (7, 6, 0), 15: (7, 7, 7)}
L, D, RED, W = (4, 4, 4), (3, 3, 3), (6, 1, 1), (7, 7, 7)
pats = [[L, L, D, D], [RED, RED, W, W]]
TOP = [(3, 4, 5), (1, 3, 2), (6, 6, 6), (4, 4, 5), (2, 2, 3), (6, 5, 3), (2, 3, 6), (0, 3, 1)]      # 4..11 no topo
BOT = [(1, 1, 2), (3, 3, 3), (5, 5, 5), (0, 0, 0), (4, 0, 0), (0, 7, 1), (7, 5, 0), (5, 5, 6)]      # 4..11 no cockpit


def rgb(c):
    return [(c[0] << 4) | c[2], c[1]]


def phase(p):
    return [x for pat in pats for i in range(4) for x in rgb(pat[(i + p) & 3])]


pal_top = [x for c in TOP for x in rgb(c)]
pal_bot = [x for c in BOT for x in rgb(c)]
palbase = ([x for i in range(4) for x in rgb(STATIC[i])] + pal_top +
           [x for i in range(12, 16) for x in rgb(STATIC[i])])
paltab = [x for p in range(4) for x in phase(p) + [0] * 16]

# ---------------------------------------------------------------- arte (listas de retângulos x,y,w,h,cor)
rng = random.Random(1990)


def runs(vals, color, base):
    """colunas -> retângulos (agrupa colunas vizinhas de mesma altura)."""
    out, x = [], 0
    while x < len(vals):
        y = x
        while y < len(vals) and vals[y] == vals[x]:
            y += 1
        if vals[x] > 0:
            out.append((x, base - vals[x], y - x, vals[x], color))
        x = y
    return out


top = [(0, 0, 0, 40, 0), (0, 40, 0, 44, 1)]                              # w=0 => 256
mt = [int(round(11 + 6 * math.sin(2 * math.pi * 2 * x / 256) + 4 * math.sin(2 * math.pi * 5 * x / 256 + 1))) for x in range(256)]
hl = [int(round(5 + 3 * math.sin(2 * math.pi * 3 * x / 256 + 2) + 2 * math.sin(2 * math.pi * 7 * x / 256))) for x in range(256)]
top += runs(mt, 4, 84) + runs(hl, 5, 84)
x = 6
while x < 246:                                                          # prédios (não cruzam a borda: o scroll dá a volta)
    w = rng.randint(6, 13)
    h = rng.randint(7, 22)
    col = rng.choice([6, 6, 7])
    top.append((x, 84 - h, w, h, col))
    for wy in range(84 - h + 2, 82, 4):
        for wx in range(x + 1, x + w - 1, 3):
            top.append((wx, wy, 2, 2, 8))
    x += w + rng.randint(2, 12)
for tx in range(3, 250, 17):                                            # árvores
    top.append((tx, 79, 4, 3, 7 + 4))
top.append((0, 82, 0, 2, 2))                                            # faixa de grama junto ao horizonte

cock = [(0, COCKY, 0, 52, 4), (0, COCKY, 0, 1, 6), (0, COCKY + 1, 0, 1, 7)]
cock += [(8, 164, 104, 12, 7), (118, 164, 20, 16, 7), (166, 164, 82, 16, 7),            # painéis do HUD
         (8, 180, 72, 12, 7), (176, 180, 72, 12, 7)]
cock += [(12 + 8 * i, 167, 6, 6, 5) for i in range(12)]                               # LEDs do conta-giros (apagados)
cock += [(52, 200, 50, 2, 6), (154, 200, 50, 2, 6)]                                    # braços da suspensão
for y in range(190, 212):                                                             # bico do carro
    t = (y - 190) / 21.0
    w = 14 + int(t * 60)
    x0 = 128 - w // 2
    cock += [(x0, y, w, 1, 12), (x0, y, 3, 1, 8), (x0 + w - 3, y, 3, 1, 8)]
    if y >= 193:
        cock.append((126, y, 4, 1, 15))
cock += [(60, 208, 136, 4, 13), (60, 207, 136, 1, 5)]                                  # asa dianteira
cock += [(22, 194, 30, 18, 13), (24, 196, 2, 14, 5), (204, 194, 30, 18, 13), (230, 196, 2, 14, 5)]   # pneus
cock.append((0, 0, 0, 0, 0))                                                          # fim (h=0)
top.append((0, ROADY, 0, ROWS, 2))                                       # linhas 84..159 da página 1 = grama (esconde a troca de página no meio da linha)
top.append((0, 0, 0, 0, 0))

# ---------------------------------------------------------------- sprites (carros)
BODY = ["................", "................", "......####......", ".....######.....",
        ".....######.....", "....########....", "....########....", "....########....",
        "....########....", "....########....", "....########....", ".....######.....",
        ".....######.....", "....########....", "................", "................"]
DARK = [".##############.", ".##############.", "................", "................",
        "................", "................", "................", "###..........###",
        "####........####", "####........####", "####..####..####", "####..####..####",
        "####..####..####", "####........####", "####........####", ".###........###."]
DARK = [r.ljust(16, ".") for r in DARK]


def scaled(art, s):
    out = [[0] * 16 for _ in range(16)]
    ox, oy = (16 - s) // 2, 16 - s
    for y in range(s):
        for x in range(s):
            y0, y1 = y * 16 // s, max(y * 16 // s + 1, (y + 1) * 16 // s)
            x0, x1 = x * 16 // s, max(x * 16 // s + 1, (x + 1) * 16 // s)
            out[oy + y][ox + x] = int(any(art[yy][xx] == "#" for yy in range(y0, y1) for xx in range(x0, x1)))
    return out


def pattern_bytes(bits):
    return ([sum(bits[y][x] << (7 - x) for x in range(8)) for y in range(16)] +
            [sum(bits[y][8 + x] << (7 - x) for x in range(8)) for y in range(16)])


def hdouble(art):
    return ["".join(c + c for c in r) for r in art]


def sub16(bits32, c0):
    return [row[c0:c0 + 16] for row in bits32]


def scaled_xl(art):
    wide = hdouble(art)                                         # 32x16
    return [[int(c == "#") for c in r] for r in wide]


slots = []                                                      # ordem dos padrões na tabela (cada um = 1 slot de 32 bytes)
xd, xb = scaled_xl(DARK), scaled_xl(BODY)
slots += [pattern_bytes(sub16(xd, 0)), pattern_bytes(sub16(xb, 0)),
          pattern_bytes(sub16(xd, 16)), pattern_bytes(sub16(xb, 16))]      # XL: Dl, Bl, Dr, Br
for s in (16, 11, 7, 4):
    slots += [pattern_bytes(scaled(DARK, s)), pattern_bytes(scaled(BODY, s))]
sprpat = [b for sl in slots for b in sl]
assert len(sprpat) == 12 * 32
szpat = [0, 4, 8, 12,  16, 20, 255, 255,  24, 28, 255, 255,  32, 36, 255, 255,  40, 44, 255, 255]
sprcol = []
for body in (12, 3, 14):
    sprcol += [13] * 16 + [body] * 16 + [13] * 16 + [body] * 16     # Dl, Bl, Dr, Br

ytab, sizetab = [], []
for dq in range(256):
    d = dq / 4.0
    rc = ROWS - ROWS * ((min(d, 48) / 48.0) ** (1 / 1.6)) if d > 0 else ROWS - 1
    ytab.append(max(ROADY, min(ROADY + ROWS - 1, int(round(ROADY + rc - 1)))))
    sizetab.append(4 * (0 if d < 6 else 1 if d < 12 else 2 if d < 20 else 3 if d < 32 else 4))
zline = [max(k for k, (y, h, rc) in enumerate(zones) if ROADY + u >= y) for u in range(ROWS)]
revtab = [(r * 12) // 16 for r in range(17)]

# ---- fonte do HUD (5x7 em células de 8x8; fg=15 branco, bg=7 preto)
FONT = {
    "0": [".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."],
    "1": ["..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."],
    "2": [".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"],
    "3": [".###.", "#...#", "....#", "..##.", "....#", "#...#", ".###."],
    "4": ["...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."],
    "5": ["#####", "#....", "####.", "....#", "....#", "#...#", ".###."],
    "6": [".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."],
    "7": ["#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."],
    "8": [".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."],
    "9": [".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."],
    " ": ["....."] * 7,
    "N": ["#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"],
    "K": ["#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"],
    "M": ["#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"],
    "H": ["#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    "P": ["####.", "#...#", "#...#", "####.", "#....", "#....", "#...."],
    "L": ["#....", "#....", "#....", "#....", "#....", "#....", "#####"],
    "/": ["....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."],
}
GLYPHS = "0123456789 NKMHPL/"          # índices 0..17 (10 = branco, 11 = N, 12 = K, 13 = M, 14 = H, 15 = P, 16 = L, 17 = /)
fontdata = []
for ch in GLYPHS:
    rows = FONT[ch]
    for y in range(8):
        px = [7] * 8
        if y < 7:
            for x in range(5):
                if rows[y][x] == "#":
                    px[1 + x] = 15
        fontdata += [(px[2 * j] << 4) | px[2 * j + 1] for j in range(4)]
assert len(fontdata) == 18 * 32


def db(name, vals, per=16):
    out = [name + ":"]
    for i in range(0, len(vals), per):
        out.append("        defb " + ",".join(str(v & 0xFF) for v in vals[i:i + per]))
    return out


def art(name, rects):
    return db(name, [v for r in rects for v in r], 5)


defs_ = ["; gerado por gen_game.py - constantes (sem dados: pode ser incluído antes do ORG)",
         "ROADY   equ %d" % ROADY, "ROWS    equ %d" % ROWS, "TEXROWS equ %d" % TEX, "ZN      equ %d" % ZN,
         "SKYY    equ %d" % SKYY, "COCKY   equ %d" % COCKY, "ILSKY   equ %d" % ILSKY, "ILROAD0 equ %d" % ILROAD0]
open("gamedefs.inc", "w").write("\n".join(defs_) + "\n")
lines = ["; gerado por gen_game.py - dados sintéticos e arte original (incluir DEPOIS do ORG)"]
lines += db("hwtab", hw) + db("sbt", sb) + db("zt3", zt3, 8) + db("pal_top", pal_top) + db("pal_bot", pal_bot)
lines += db("palbase", palbase) + db("paltab", paltab) + db("sprpat", sprpat) + db("sprcol", sprcol) + db("szpat", szpat, 4)
lines += db("fontdata", fontdata) + db("revtab", revtab)
lines += db("ytab", ytab) + db("sizetab", sizetab) + db("zlinetab", zline)
lines += art("arttop", top) + art("artcock", cock)
open("gamedata.inc", "w").write("\n".join(lines) + "\n")
print("gamedefs.inc + gamedata.inc: zonas=%d  arte: topo=%d ret., cockpit=%d ret. (%d bytes)" % (
    ZN, len(top), len(cock), 5 * (len(top) + len(cock))))
