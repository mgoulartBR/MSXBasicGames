#!/usr/bin/env python3
"""Gera os sprites dos bonequinhos do predio (2.3+). Cada boneco = 16x24 px = 2 sprites 16x16 (cabeca/tronco e pernas) x 2 camadas
(preenchimento com cor por linha + contorno preto), 2 quadros de caminhada, 2 direcoes (a esquerda e o espelho da direita).
Saidas: src/sprites_fig.h (VERSIONADO: bonecos desenhados a mao, sem arte de terceiros) e, se existir a folha de referencia
(original/user/sprites_sheet.webp, enviada pelo usuario, git-ignorada), src/data/sprites_art.h (SPR_ART 1, tambem ignorado).
Uso: python3 -I tools/make_sprites.py [folha.webp]"""
import os, sys
from PIL import Image

W, H = 16, 24
P3 = [(2,2,2),(0,0,0),(2,5,2),(4,6,4),(1,2,3),(2,4,5),(3,1,0),(3,5,6),(6,1,1),(7,3,3),(6,5,0),(7,7,0),(4,2,0),(4,1,1),(5,5,5),(7,7,7)]
PAL = [tuple(round(c * 255 / 7) for c in p) for p in P3]
def nearest(c, allowed): return min(allowed, key=lambda i: sum((a - b) ** 2 for a, b in zip(c, PAL[i])))
FILL_COLORS = [i for i in range(16) if i not in (0, 1)]          # 0 = transparente em sprite; 1 = preto (contorno)

def blank(): return [[0] * W for _ in range(H)]

# ---------------------------------------------------------------- fallback desenhado a mao (o boneco 16x16 da 2.3 inicial, rebaixado para 16x24)
F0 = ["                ", "      hhhh      ", "     hhhhhh     ", "     hhhhhh     ", "     sesses     ", "      ssss      ", "    bbbbbbbb    ", "   bbbbbbbbbb   ",
      "   sbbbbbbbbs   ", "   sbbbbbbbbs   ", "    bbbbbbbb    ", "     tttttt     ", "     ttt ttt    ", "     ttt ttt    ", "     fff fff    ", "                "]
F1 = ["                ", "      hhhh      ", "     hhhhhh     ", "     hhhhhh     ", "     sesses     ", "      ssss      ", "    bbbbbbbb    ", "   bbbbbbbbbb   ",
      "   sbbbbbbbbs   ", "    bbbbbbbb    ", "    bbbbbbbb    ", "     tttttt     ", "    ttt  ttt    ", "   ttt    ttt   ", "  fff      fff  ", "                "]

def dil(mask):
    h, w = len(mask), len(mask[0])
    out = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            if mask[y][x]: continue
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    yy, xx = y + dy, x + dx
                    if 0 <= yy < h and 0 <= xx < w and mask[yy][xx]: out[y][x] = 1
    return out

def hand_frame(fr, hair, shirt):
    fill, edge = blank(), blank()
    rows = [None] * H
    for y, row in enumerate(fr):
        for x, c in enumerate(row):
            Y = y + 7                                           # figura 16 linhas encostada embaixo (linhas 7..22)
            if Y >= H: continue
            if c not in " e": fill[Y][x] = 1
    for y, row in enumerate(fr):
        for x, c in enumerate(row):
            if c == "e" and y + 7 < H: edge[y + 7][x] = 1
    body = [[1 if fill[y][x] or edge[y][x] else 0 for x in range(W)] for y in range(H)]
    ring = dil(body)
    for y in range(H):
        for x in range(W):
            if ring[y][x] and not body[y][x]: edge[y][x] = 1
    cols = [0] * H
    for y, row in enumerate(fr):
        k = set(row) - {" "}
        c = 0
        if "h" in k: c = hair
        elif "e" in k or ("s" in k and "b" not in k): c = 9
        elif "b" in k: c = shirt
        elif "t" in k: c = 4
        elif "f" in k: c = 6
        if y + 7 < H: cols[y + 7] = c
    return fill, edge, cols

# ---------------------------------------------------------------- a partir da folha (preview / fill / outline por personagem)
CELLS = {3: (730, 182), 5: (10, 542)}
PANEL = {"fill": 122, "out": 237}
def sheet_shape(im, ox, oy):
    def panel(name):
        x0 = PANEL[name]
        return im.crop((ox + x0, oy + 75, ox + x0 + 106, oy + 281))
    out, fil = panel("out"), panel("fill")
    px = out.load()
    dark = [[1 if sum(px[x, y]) < 330 else 0 for x in range(out.width)] for y in range(out.height)]
    # silhueta = contorno + buracos preenchidos (flood fill do fundo a partir das bordas)
    h, w = len(dark), len(dark[0])
    outside = [[0] * w for _ in range(h)]
    stack = [(x, y) for x in range(w) for y in (0, h - 1)] + [(x, y) for y in range(h) for x in (0, w - 1)]
    while stack:
        x, y = stack.pop()
        if not (0 <= x < w and 0 <= y < h) or outside[y][x] or dark[y][x]: continue
        outside[y][x] = 1
        stack += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    sil = [[0 if outside[y][x] else 1 for x in range(w)] for y in range(h)]
    ys = [y for y in range(h) for x in range(w) if sil[y][x]]; xs = [x for y in range(h) for x in range(w) if sil[y][x]]
    x0, x1, y0, y1 = min(xs), max(xs) + 1, min(ys), max(ys) + 1
    bw, bh = x1 - x0, y1 - y0
    th = 22; tw = min(14, max(8, round(bw * th / bh)))
    def mask_img(m):
        im2 = Image.new("L", (w, h))
        im2.putdata([255 if m[y][x] else 0 for y in range(h) for x in range(w)])
        return im2.crop((x0, y0, x1, y1)).resize((tw, th), Image.BOX)
    sil_t, out_t = mask_img(sil), mask_img(dark)
    col_t = fil.crop((x0, y0, x1, y1)).resize((tw, th), Image.BOX)
    ox0 = (W - tw) // 2; oy0 = H - 1 - th                      # centralizado, pes na linha 22 (a 23 fica vazia)
    fill, edge, cols = blank(), blank(), [0] * H
    for y in range(th):
        votes = {}
        for x in range(tw):
            s = sil_t.getpixel((x, y)) > 128; e = out_t.getpixel((x, y)) > 165
            if e: edge[oy0 + y][ox0 + x] = 1
            elif s:
                fill[oy0 + y][ox0 + x] = 1
                c = nearest(col_t.getpixel((x, y)), FILL_COLORS); votes[c] = votes.get(c, 0) + 1
        if votes: cols[oy0 + y] = max(votes, key=votes.get)
    for y in range(12, H):                                      # abaixo do rosto: pele/cinza (mao e caixa da folha) vira a cor do tronco/calca
        if cols[y] in (9, 14, 15) and y < 21: cols[y] = cols[y - 1]
    last = [y for y in range(H) if cols[y]]
    for y in last[-2:]: cols[y] = 6                              # sapatos: marrom escuro
    return fill, edge, cols

def stride(frame):                                              # quadro 2: pernas espelhadas (passo oposto)
    fill, edge, cols = frame
    LEG = 16
    f2 = [row[:] for row in fill]; e2 = [row[:] for row in edge]
    for y in range(LEG, H):
        f2[y] = fill[y][::-1]; e2[y] = edge[y][::-1]
    return f2, e2, cols[:]

def mirror(frame):
    fill, edge, cols = frame
    return [r[::-1] for r in fill], [r[::-1] for r in edge], cols

def pat(mask, y0):                                              # 32 bytes (16 linhas a partir de y0; linhas alem de H = 0)
    def row(y): return mask[y] if y < H else [0] * W
    left = [sum(row(y0 + i)[x] << (7 - x) for x in range(8)) for i in range(16)]
    right = [sum(row(y0 + i)[8 + x] << (7 - x) for x in range(8)) for i in range(16)]
    return left + right

def emit(path, shapes, chars, tag):
    # shapes: lista de (frameA, frameB) voltados para a DIREITA; chars: lista de (indice do shape, [24 cores])
    out = ["// GERADO por tools/make_sprites.py (%s) - nao edite a mao." % tag]
    out.append("// k_SprPat[forma][direcao 0=direita 1=esquerda][quadro][parte: 0 preench.cima 1 preench.baixo 2 contorno cima 3 contorno baixo][32]")
    out.append("static const u8 k_SprPat[%d][2][2][4][32] = {" % len(shapes))
    for fa, fb in shapes:
        out.append("\t{")
        for d in range(2):
            out.append("\t\t{")
            for fr in (fa, fb):
                fill, edge, _ = mirror(fr) if d else fr
                parts = [pat(fill, 0), pat(fill, 16), pat(edge, 0), pat(edge, 16)]
                out.append("\t\t\t{ " + ", ".join("{" + ",".join("0x%02X" % v for v in p) + "}" for p in parts) + " },")
            out.append("\t\t},")
        out.append("\t},")
    out.append("};")
    out.append("static const u8 k_FigShape[%d] = { %s };" % (len(chars), ",".join(str(c[0]) for c in chars)))
    out.append("static const u8 k_FigCol[%d][24] = {          // cor por linha do preenchimento (0 = transparente): jogador, rival 1, rival 2" % len(chars))
    for _, cols in chars: out.append("\t{ " + ",".join(str(c) for c in cols) + " },")
    out.append("};")
    open(path, "w").write("\n".join(out) + "\n")

def preview(path, shapes, chars):
    im = Image.new("RGB", (len(chars) * 4 * 16 * 6 + 20, H * 6 + 8), (70, 100, 70))
    for ci, (si, cols) in enumerate(chars):
        fa, fb = shapes[si]
        for k, fr in enumerate((fa, fb, mirror(fa), mirror(fb))):
            fill, edge, _ = fr
            ox = (ci * 4 + k) * 16 * 6 + 4
            for y in range(H):
                for x in range(W):
                    c = 1 if edge[y][x] else (cols[y] if fill[y][x] else None)
                    if c:
                        for dy in range(6):
                            for dx in range(6): im.putpixel((ox + x * 6 + dx, 4 + y * 6 + dy), PAL[c])
    im.save(path)

def main():
    # fallback desenhado a mao (jogador castanho/vermelho, rival 1 loiro/verde, rival 2 grisalho/azul)
    shapes, chars = [], []
    a0 = hand_frame(F0, 12, 8); a1 = hand_frame(F1, 12, 8)
    shapes.append((a0, a1))
    for hair, shirt in ((12, 8), (11, 2), (14, 5)):
        c = hand_frame(F0, hair, shirt)[2]
        chars.append((0, c))
    emit("src/sprites_fig.h", shapes, chars, "bonecos desenhados a mao")
    preview("/tmp/sprites_preview_hand.png", shapes, chars)
    sheet = sys.argv[1] if len(sys.argv) > 1 else "original/user/sprites_sheet.webp"
    if os.path.exists(sheet):
        im = Image.open(sheet).convert("RGB")
        s3 = sheet_shape(im, *CELLS[3]); s5 = sheet_shape(im, *CELLS[5])
        shapes2 = [(s3, stride(s3)), (s5, stride(s5))]
        # rival 2 = forma 3 recolorida (vermelho -> azul/cinza)
        remap = {8: 5, 13: 4, 9: 9, 2: 14}
        c3 = s3[2]; c5 = s5[2]
        chars2 = [(0, c3), (1, c5), (0, [remap.get(c, c) for c in c3])]
        os.makedirs("src/data", exist_ok=True)
        emit("src/data/sprites_art.h", shapes2, chars2, "folha enviada pelo usuario")
        open("src/data/sprites_art.h", "a").write("#define SPR_ART 1\n")
        preview("/tmp/sprites_preview_art.png", shapes2, chars2)
main()
