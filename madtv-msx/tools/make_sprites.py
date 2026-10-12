#!/usr/bin/env python3
"""Gera os sprites dos bonequinhos do predio (arte propria, 2 camadas: preenchimento com cor por linha + contorno preto).
Saida: src/sprites_fig.h (versionado) e /tmp/sprites_preview.png.  Uso: python3 -I tools/make_sprites.py"""
from PIL import Image

# silhueta (16 colunas x 16 linhas): '#' = corpo; o contorno e calculado por dilatacao; 'e' = olho (preto, vai na camada de contorno)
F0 = [
 "                ",
 "      hhhh      ",
 "     hhhhhh     ",
 "     hhhhhh     ",
 "     sesses     ",
 "      ssss      ",
 "    bbbbbbbb    ",
 "   bbbbbbbbbb   ",
 "   sbbbbbbbbs   ",
 "   sbbbbbbbbs   ",
 "    bbbbbbbb    ",
 "     tttttt     ",
 "     ttt ttt    ",
 "     ttt ttt    ",
 "     fff fff    ",
 "                ",
]
F1 = [
 "                ",
 "      hhhh      ",
 "     hhhhhh     ",
 "     hhhhhh     ",
 "     sesses     ",
 "      ssss      ",
 "    bbbbbbbb    ",
 "   bbbbbbbbbb   ",
 "   sbbbbbbbbs   ",
 "    bbbbbbbb    ",
 "    bbbbbbbb    ",
 "     tttttt     ",
 "    ttt  ttt    ",
 "   ttt    ttt   ",
 "  fff      fff  ",
 "                ",
]
ROWCOL = None
# paleta global do jogo (src/chrome.c k_Pal), para a previa
P3 = [(2,2,2),(0,0,0),(2,5,2),(4,6,4),(1,2,3),(2,4,5),(3,1,0),(3,5,6),(6,1,1),(7,3,3),(6,5,0),(7,7,0),(4,2,0),(4,1,1),(5,5,5),(7,7,7)]
PAL = [tuple(round(c * 255 / 7) for c in p) for p in P3]
SKIN, TROUSER, SHOE = 9, 4, 6
# (cabelo, camisa)
CHARS = [(12, 8), (11, 2), (14, 5)]          # jogador (ruivo/castanho, vermelho), rival 1 (loiro, verde), rival 2 (grisalho, azul)
ROW_KIND = "ohhhsshbbbbbtttffo"              # classe de cada linha do desenho (o = vazio)

def dil(mask):
    h, w = len(mask), len(mask[0])
    out = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            if mask[y][x]: continue
            for dy, dx in ((1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)):
                yy, xx = y + dy, x + dx
                if 0 <= yy < h and 0 <= xx < w and mask[yy][xx]: out[y][x] = 1; break
    return out

def layers(frame):
    body = [[1 if c not in " e" else 0 for c in row] for row in frame]
    eyes = [[1 if c == "e" else 0 for c in row] for row in frame]
    body_all = [[body[y][x] or eyes[y][x] for x in range(16)] for y in range(16)]
    ring = dil(body_all)
    outline = [[ring[y][x] or eyes[y][x] for x in range(16)] for y in range(16)]
    return body, outline

def pat(mask):                                # 32 bytes: coluna esquerda (16) + coluna direita (16)
    left = [sum(mask[y][x] << (7 - x) for x in range(8)) for y in range(16)]
    right = [sum(mask[y][8 + x] << (7 - x) for x in range(8)) for y in range(16)]
    return left + right

def rowcolors(frame, hair, shirt):
    cols = []
    for y, row in enumerate(frame):
        k = set(row) - {" "}
        c = 0
        if "h" in k: c = hair
        elif "e" in k or "s" in k and "b" not in k: c = SKIN
        elif "b" in k: c = shirt
        elif "t" in k: c = TROUSER
        elif "f" in k: c = SHOE
        cols.append(c)
    return cols

def main():
    out = ["// GERADO por tools/make_sprites.py (arte propria dos bonequinhos) - nao edite a mao.",
           "// Cada boneco = 2 sprites 16x16: preenchimento (cor por linha, 3 variantes) + contorno preto; 2 quadros de caminhada."]
    fills, outs = [], []
    for fr in (F0, F1):
        b, o = layers(fr); fills.append(pat(b)); outs.append(pat(o))
    out.append("static const u8 k_SprFill[2][32] = {")
    for p in fills: out.append("\t{ " + ",".join("0x%02X" % v for v in p) + " },")
    out.append("};")
    out.append("static const u8 k_SprEdge[2][32] = {")
    for p in outs: out.append("\t{ " + ",".join("0x%02X" % v for v in p) + " },")
    out.append("};")
    out.append("static const u8 k_FigCol[3][16] = {          // cor por linha do preenchimento (0 = transparente): jogador, rival 1, rival 2")
    for hair, shirt in CHARS:
        cols = rowcolors(F0, hair, shirt)
        out.append("\t{ " + ",".join(str(c) for c in cols) + " },")
    out.append("};")
    open("src/sprites_fig.h", "w").write("\n".join(out) + "\n")
    # previa 8x
    im = Image.new("RGB", (16 * 6 * 8 + 20, 16 * 8 + 4), (60, 90, 60))
    for ci, (hair, shirt) in enumerate(CHARS):
        for fi, fr in enumerate((F0, F1)):
            b, o = layers(fr); cols = rowcolors(fr, hair, shirt)
            ox = (ci * 2 + fi) * 16 * 8 + ci * 4
            for y in range(16):
                for x in range(16):
                    c = None
                    if o[y][x]: c = 1
                    elif b[y][x] and cols[y]: c = cols[y]
                    if c is not None:
                        for dy in range(8):
                            for dx in range(8): im.putpixel((ox + x * 8 + dx, 2 + y * 8 + dy), PAL[c])
    im.save("/tmp/sprites_preview.png")
main()
