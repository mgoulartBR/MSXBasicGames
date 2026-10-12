#!/usr/bin/env python3
"""Gera as cenas das salas (chefe, escritorio, agencia de publicidade) a partir de capturas do Mad TV guardadas LOCALMENTE.
Entradas: capturas 320x200 do Mad TV em original/dos/*.png (DOSBox; ver SCENES).
Saidas (NAO versionadas, ver .gitignore): src/data/art_scenes.h e src/seg/seg_s19..s23_b3.c. Sem elas o jogo usa as telas de texto (ART_SCENES 0).
Uso: python3 -I tools/make_scenes.py original/dos"""
import sys
from PIL import Image

P3 = [(2,2,2),(0,0,0),(2,5,2),(4,6,4),(1,2,3),(2,4,5),(3,1,0),(3,5,6),(6,1,1),(7,3,3),(6,5,0),(7,7,0),(4,2,0),(4,1,1),(5,5,5),(7,7,7)]
PAL = [tuple(round(c * 255 / 7) for c in p) for p in P3]
def nearest(c): return min(range(16), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, PAL[i])))

H = 111
# (nome, arquivo em original/dos, caixa (x0,y0,x1,y1) na captura nativa 320x200; o interior da cena fica em y=4..114)
SCENES = [("boss", "boss.png", (4, 4, 260, 4 + H)), ("office", "office.png", (40, 4, 296, 4 + H)),
          ("ads", "ads.png", (70, 4, 190, 4 + H)), ("movie", "movie.png", (0, 4, 120, 4 + H)), ("script", "script.png", (100, 4, 220, 4 + H)),
          ("studio", "studio.png", (0, 4, 120, 4 + H)), ("library", "library.png", (110, 4, 230, 4 + H)), ("realty", "realestate.png", (160, 4, 280, 4 + H)),
          ("betty", "betty.png", (55, 4, 175, 4 + H)), ("porter", "porter.png", (100, 4, 220, 4 + H)), ("super", "super.png", (20, 4, 140, 4 + H)),
          ("news", "news.png", (150, 4, 270, 4 + H)),
          ("gridcol", "grid1.png", None),                      # colunas da grade (setas+lixeira a esquerda, 6 icones a direita) lado a lado: 28 + 20 = 48 px
          ("title", "../user/title.webp", (130, 10, 1456, 981))]   # tela de abertura (arte enviada pelo usuario, ampliada ~5x): reduzida para 256x196
RESIZE = {"title": (256, 196)}
FIXED_SEG = {"news": 15, "gridcol": 2, "title": 32}     # segmento 15 estava livre: evita ROM de 512 KB

def main(d):
    first_seg = 19
    seg = first_seg
    table = []
    for name, fn, box in SCENES:
        if name in FIXED_SEG: seg_save, seg = seg, FIXED_SEG[name]
        if name == "gridcol":
            src = Image.open(d + "/" + fn).convert("RGB")
            im = Image.new("RGB", (48, 106)); im.paste(src.crop((6, 6, 34, 112)), (0, 0)); im.paste(src.crop((294, 6, 314, 112)), (28, 0))
        else: im = Image.open(d + "/" + fn).convert("RGB").crop(box)
        if name == "title": im.paste((0, 0, 0), (270, 625, 870, 740))      # credito "MSX Port by BigFive" some: o jogo o escreve com a fonte propria (legivel a 256 px)
        if name in RESIZE: im = im.resize(RESIZE[name], Image.LANCZOS)
        w, h = im.size
        stride = w // 2
        rps = min(255, 8192 // stride)             # u8 na tabela de cenas
        nseg = (h + rps - 1) // rps
        px = [[nearest(im.getpixel((x, y))) for x in range(w)] for y in range(h)]
        q = Image.new("RGB", (w, h))
        for y in range(h):
            for x in range(w): q.putpixel((x, y), PAL[px[y][x]])
        q.resize((w * 3, h * 3), Image.NEAREST).save("/tmp/scene_%s.png" % name)
        for s in range(nseg):
            data = []
            for r in range(rps):
                y = s * rps + r
                data += [(px[y][x] << 4) | px[y][x + 1] for x in range(0, w, 2)] if y < h else [0] * stride
            data += [0] * (8192 - len(data))
            with open("src/seg/seg_s%d_b3.c" % (seg + s), "w") as f:
                f.write("// GERADO por tools/make_scenes.py (arte do jogo original) - NAO versionar. Cena '%s', parte %d.\n#include \"msxgl.h\"\nconst u8 g_ArtScene%d[8192] = {\n" % (name, s, seg + s))
                for i in range(0, 8192, 32): f.write("\t" + ", ".join(str(v) for v in data[i:i + 32]) + ",\n")
                f.write("};\n")
        if name == "title": splash_bg = nearest(Image.open(d + "/" + fn).convert("RGB").getpixel((60, 400)))
        table.append((seg, stride, h, rps))
        seg = seg_save if name in FIXED_SEG else seg + nseg
    with open("src/data/art_scenes.h", "w") as f:
        f.write("// GERADO por tools/make_scenes.py - NAO versionar.\n#define ART_SCENES 1\n")
        for i, (name, _, _) in enumerate(SCENES): f.write("#define SCN_%s %d\n" % (name.upper(), i))
        f.write("#define SPLASH_BG %d      // cor da borda da tela de abertura\n" % splash_bg)
        f.write("#define SCN_INIT { %s }   // segmento, bytes por linha, linhas, linhas por segmento\n" % ", ".join("{%d, %d, %d, %d}" % t for t in table))
    print("ok: segmentos %d..%d" % (first_seg, seg - 1))

if __name__ == "__main__":
    main(sys.argv[1])
