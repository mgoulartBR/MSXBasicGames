#!/usr/bin/env python3
"""Gera src/data/art_icons.h a partir de capturas do Mad TV original (DOSBox) guardadas LOCALMENTE em original/ref2/.
As imagens do original NAO fazem parte do repositorio (original/ e src/data/art_*.h estao no .gitignore); sem elas o jogo usa
simbolos desenhados em codigo (ART_ICONS = 0). Uso: python3 -I tools/make_art.py original/ref2/mtv_000.png src/data/art_icons.h
Paleta alvo = a paleta global do jogo (src/chrome.c, k_Pal)."""
import sys
from PIL import Image

# paleta global (8 bits) - espelha k_Pal em chrome.c
PAL = [(0x49,0x49,0x49),(0,0,0),(0x41,0xA2,0x41),(0x8E,0xDF,0x82),(0x2C,0x51,0x79),(0x55,0x92,0xBE),(0x69,0x24,0),(0x6D,0xBA,0xDF),
       (0xDF,0x24,0x24),(0xEB,0x7D,0x6D),(0xD3,0xAE,0),(0xF3,0xF3,0),(0x9A,0x4D,0),(0x82,0x34,0x30),(0xB2,0xB2,0xB2),(0xF3,0xF3,0xF3)]
# icones: nome, caixa (x0,y0,x1,y1) na captura nativa 320x200 e tamanho final (w par: 2 pixels por byte)
ICONS = [("dollar", (128,124,146,146), 8, 10), ("sofa", (127,147,147,163), 12, 8), ("heart", (178,161,196,177), 10, 8), ("clock", (125,176,143,195), 10, 10)]

def nearest(c):
    return min(range(16), key=lambda i: sum((a-b)**2 for a,b in zip(c,PAL[i])))

def main(src, out):
    im = Image.open(src).convert("RGB")
    lines = ["// GERADO por tools/make_art.py a partir de capturas do jogo original - NAO versionar, NAO editar.", "#define ART_ICONS 1"]
    for name, box, w, h in ICONS:
        c = im.crop(box).resize((w, h), Image.LANCZOS)
        px = [nearest(c.getpixel((x, y))) for y in range(h) for x in range(w)]
        data = []
        for y in range(h):
            for x in range(0, w, 2):
                data.append((px[y*w+x] << 4) | px[y*w+x+1])
        lines.append("#define ART_%s_W %d\n#define ART_%s_H %d" % (name.upper(), w, name.upper(), h))
        lines.append("static const unsigned char k_Art_%s[%d] = { %s };" % (name, len(data), ", ".join(str(v) for v in data)))
    open(out, "w").write("\n".join(lines) + "\n")
    print("icones:", [n for n,_,_,_ in ICONS], "->", out)

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
