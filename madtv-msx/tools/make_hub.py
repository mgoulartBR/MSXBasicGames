#!/usr/bin/env python3
"""Gera o fundo do predio (256x160, 4 bpp) a partir de uma captura do Mad TV original guardada LOCALMENTE.
Entrada: original/user/n1.png (320x200 nativo; a captura da vista do predio). Saida (NAO versionada, ver .gitignore):
  src/data/art_hub.h            #define ART_HUB 1 ...
  src/seg/seg_s16_b3.c .. s18   dados em segmentos do mapper (64 linhas de 128 bytes por segmento)
  /tmp/hub_preview.png          previa
Sem esses arquivos o jogo desenha o predio proceduralmente (ART_HUB 0). Uso: python3 -I tools/make_hub.py original/user/n1.png"""
import sys
from PIL import Image, ImageDraw

W, H = 256, 160
CONTENT_TOP = 26
# paleta global (3 bits por canal expandidos), igual a k_Pal em src/chrome.c
P3 = [(2,2,2),(0,0,0),(2,5,2),(4,6,4),(1,2,3),(2,4,5),(3,1,0),(3,5,6),(6,1,1),(7,3,3),(6,5,0),(7,7,0),(4,2,0),(4,1,1),(5,5,5),(7,7,7)]
PAL = [tuple(round(c * 255 / 7) for c in p) for p in P3]

def nearest(c):
    return min(range(16), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, PAL[i])))

def FY(k): return 182 - 31 * k        # igual a building.c
ROOMS = [(0,40,'booth'),(0,84,'a'),(0,172,'c'),(1,40,'a'),(1,84,'c'),(1,172,'b'),(2,40,'c'),(2,84,'a'),(2,172,'b'),
         (3,84,'b'),(3,172,'a'),(3,216,'c'),(4,40,'a'),(4,172,'b')]
DECOR = {0: [('pic',216)], 1: [('plant2',104),('plant1',152),('frame',200)], 2: [('frame',60),('plant1',104),('plant2',152),('plant2',212)],
         3: [('plant1',50),('plant2',104),('frame',194),('plant1',152)], 4: [('plant2',84),('plant1',152),('window',220)]}
KEYED = {'plant1': (142,223,130), 'plant2': (142,223,130), 'frame': (142,223,130), 'window': (162,219,243), 'pic': (178,178,178)}   # cor de fundo do recorte = transparente

def main(src):
    im = Image.open(src).convert("RGB")
    # recortes (x0,y0,x1,y1) em coordenadas nativas 320x200 da captura
    C = {k: im.crop(v) for k, v in {
        'a': (117, 30, 135, 52), 'c': (82, 59, 98, 81), 'b': (216, 59, 232, 81),
        'elev': (149, 58, 171, 81), 'elevg': (149, 86, 171, 107),
        'plant1': (112, 67, 131, 81), 'plant2': (181, 67, 199, 81), 'frame': (190, 58, 203, 71),
        'window': (214, 29, 244, 48), 'booth': (74, 84, 100, 105), 'pic': (217, 87, 231, 98)}.items()}
    cv = Image.new("RGB", (W, H), (0, 0, 0))
    d = ImageDraw.Draw(cv)
    GREEN, SKY, STRIP, GREY, DARK = (142,223,130), (162,219,243), (85,146,190), (178,178,178), (105,36,0)
    # viga do telhado
    d.rectangle([0, 0, W - 1, 2], fill=(154, 77, 0)); d.line([(0, 2), (W, 2)], fill=DARK)
    for k in range(5):
        yf = FY(k) - CONTENT_TOP                      # topo da laje (linha dos pes) em coordenadas do fundo
        top = max(3, yf - 29)
        wall = GREY if k == 0 else (GREEN if k % 2 == 1 else SKY)
        d.rectangle([14, top, W - 15, yf], fill=wall)
        if k >= 2 and k % 2 == 0: d.rectangle([14, top, W - 15, top + 4], fill=STRIP)
        if k == 0: d.line([(14, yf - 1), (W - 15, yf - 1)], fill=(223, 223, 223))
        d.line([(14, yf + 1), (W - 15, yf + 1)], fill=(0, 0, 0)); d.line([(14, yf + 2), (W - 15, yf + 2)], fill=DARK)
        if yf + 3 < H: d.line([(14, yf + 3), (W - 15, yf + 3)], fill=(0, 0, 0))
        # elevador
        e = C['elevg'] if k == 0 else C['elev']
        cv.paste(e, (128 - e.size[0] // 2, yf - e.size[1] + 1))
        for (fl, x, st) in ROOMS:
            if fl != k: continue
            s = C[st]
            cv.paste(s, (x - s.size[0] // 2, yf - s.size[1] + 1))
        for (nm, x) in DECOR.get(k, []):
            s = C[nm]
            key = KEYED[nm]
            mask = Image.new("L", s.size, 255)
            for yy in range(s.size[1]):
                for xx in range(s.size[0]):
                    if sum((a - b) ** 2 for a, b in zip(s.getpixel((xx, yy)), key)) < 1800: mask.putpixel((xx, yy), 0)
            cv.paste(s, (x - s.size[0] // 2, yf - s.size[1] + 1 - (4 if nm in ('frame',) else 0) - (6 if nm == 'window' else 0)), mask)
    # colunas de tijolos + placas com o numero do andar
    for x0 in (0, W - 14):
        for y in range(0, H, 4):
            off = 0 if (y // 4) % 2 == 0 else 4
            d.rectangle([x0, y, x0 + 13, y + 3], fill=(130, 52, 48))
            d.line([(x0, y + 3), (x0 + 13, y + 3)], fill=(105, 36, 0))
            for bx in range(x0 + off, x0 + 14, 8):
                d.line([(bx, y), (bx, y + 2)], fill=(105, 36, 0))
            d.point([(x0 + 2 + off % 8, y + 1)], fill=(190, 109, 85))
    for k in range(5):
        yf = FY(k) - CONTENT_TOP
        for x0 in (1, W - 13):
            d.rectangle([x0, yf - 22, x0 + 10, yf - 10], fill=(223, 223, 223), outline=(0, 0, 0))
            d.text((x0 + 3, yf - 21), str(k + 1), fill=(0, 0, 0))
    pre = cv.resize((W * 3, H * 3), Image.NEAREST); pre.save("/tmp/hub_preview_rgb.png")
    px = [[nearest(cv.getpixel((x, y))) for x in range(W)] for y in range(H)]
    q = Image.new("RGB", (W, H))
    for y in range(H):
        for x in range(W): q.putpixel((x, y), PAL[px[y][x]])
    q.resize((W * 3, H * 3), Image.NEAREST).save("/tmp/hub_preview.png")
    rows = []
    for y in range(H):
        rows.append([(px[y][x] << 4) | px[y][x + 1] for x in range(0, W, 2)])
    nseg = (H + 63) // 64
    for s in range(nseg):
        data = []
        for r in range(64):
            data += rows[s * 64 + r] if s * 64 + r < H else [0] * 128
        with open("src/seg/seg_s%d_b3.c" % (16 + s), "w") as f:
            f.write("// GERADO por tools/make_hub.py (arte do jogo original) - NAO versionar. Segmento %d (banco 3): linhas %d-%d do fundo do predio.\n" % (16 + s, s * 64, s * 64 + 63))
            f.write("#include \"msxgl.h\"\nconst u8 g_ArtHub%d[8192] = {\n" % s)
            for i in range(0, 8192, 32): f.write("\t" + ", ".join(str(v) for v in data[i:i + 32]) + ",\n")
            f.write("};\n")
    with open("src/data/art_hub.h", "w") as f:
        f.write("// GERADO por tools/make_hub.py - NAO versionar.\n#define ART_HUB 1\n#define ART_HUB_SEG 16\n#define ART_HUB_ROWS %d\n" % H)
    print("ok:", nseg, "segmentos")

if __name__ == "__main__":
    main(sys.argv[1])
