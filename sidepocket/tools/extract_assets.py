#!/usr/bin/env python3
"""Build the MSX2 (Screen 5) assets from a VRAM/CRAM dump of YOUR OWN Mega Drive ROM.

    extract_assets.py <dump-prefix> <outdir>      (dump-prefix: files <prefix>.vram/.cram/.vsram/.reg)

Outputs (never commit these; they derive from the original game's graphics):
    table.bin   256x166 Screen-5 bitmap of the table (x 32..288, y 56..222 of the MD screen)
    balls.bin   12 cells of 8x8 (96x8): cue, balls 1..9, aim dot, target ring (96x8)
    palette.bin 16 MSX2 palette entries (2 bytes each: 0RRR0BBB, 00000GGG)
    preview.png quick visual check
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'emu'))
from mdvdp import load, compose, cram_colors, tile_px
from PIL import Image

CROP = (32, 56, 288, 222)                 # 256 x 166
CUE = 160
NUMBERED = [174, 175, 188, 189, 190, 191, 204, 205, 206]   # balls 1..9 (VRAM tile indices)
DOT, RING = 254, 253

def snap3(c): return tuple(round(v * 7 / 255) for v in c)       # MD/MSX2 both have 3 bits per channel

def main(prefix, outdir):
    vram, cram, vsram, reg = load(prefix)
    cols = cram_colors(cram, 'le')
    bg = compose(vram, cram, vsram, reg, layers=('B', 'A')).convert('RGB').crop(CROP)

    # ball cells from tile data (palette line 0). value 0 = transparent
    cells = [CUE] + NUMBERED + [DOT, RING]
    ball_px = []; mask = []
    for t in cells:
        px = tile_px(vram, t, 0, 0)
        for y in range(8):
            for x in range(8):
                pass
        ball_px.append(px)
    strip = Image.new('RGB', (8 * len(cells), 8)); sp = strip.load(); alpha = {}
    for i, px in enumerate(ball_px):
        for y in range(8):
            for x in range(8):
                v = px[y][x]
                sp[i * 8 + x, y] = cols[v] if v else (0, 0, 0)
                alpha[(i * 8 + x, y)] = v != 0

    # joint 16-colour palette over table + balls (opaque pixels), 3 bits per channel
    sample = Image.new('RGB', (256, 166 + 8)); sample.paste(bg, (0, 0)); sample.paste(strip, (0, 166))
    q = sample.quantize(colors=16, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = [snap3(tuple(q.getpalette()[i * 3:i * 3 + 3])) for i in range(16)]
    pal += [(0, 0, 0)] * (16 - len(pal))
    # force index 0 = black (transparent colour for ball blits)
    dark = min(range(16), key=lambda i: sum(pal[i]))
    pal[0], pal[dark] = pal[dark], pal[0]; pal[0] = (0, 0, 0)
    def nearest(c, skip0=False):
        c3 = snap3(c)
        return min(range(1 if skip0 else 0, 16), key=lambda i: sum((a - b) ** 2 for a, b in zip(pal[i], c3)))

    tbl = bytearray(); tp = bg.load()
    for y in range(166):
        row = [nearest(tp[x, y]) for x in range(256)]
        tbl += bytes((row[i] << 4) | row[i + 1] for i in range(0, 256, 2))
    bls = bytearray(); bp = strip.load()
    for y in range(8):
        row = [(nearest(bp[x, y], skip0=True) if alpha[(x, y)] else 0) for x in range(8 * len(cells))]
        bls += bytes((row[i] << 4) | row[i + 1] for i in range(0, len(row), 2))
    # --- fast-blit variants: transparent pixels replaced by the cloth colour (valid on flat cloth only)
    idx_tbl = [[(tbl[y * 128 + x // 2] >> (4 if x % 2 == 0 else 0)) & 15 for x in range(256)] for y in range(166)]
    from collections import Counter
    cloth = Counter(idx_tbl[y][x] for y in range(70, 100) for x in range(100, 160)).most_common(1)[0][0]
    # largest centred rectangle of cloth (a few stray spot pixels per edge are tolerated)
    x0, x1, y0, y1 = 120, 136, 76, 92
    def bad_col(x): return sum(1 for y in range(y0, y1 + 1) if idx_tbl[y][x] != cloth) > 5
    def bad_row(y): return sum(1 for x in range(x0, x1 + 1) if idx_tbl[y][x] != cloth) > 5
    grow = True
    while grow:
        grow = False
        if x0 > 0 and not bad_col(x0 - 1): x0 -= 1; grow = True
        if x1 < 255 and not bad_col(x1 + 1): x1 += 1; grow = True
        if y0 > 0 and not bad_row(y0 - 1): y0 -= 1; grow = True
        if y1 < 165 and not bad_row(y1 + 1): y1 += 1; grow = True
    ncell = len(cells)
    cellpx = []                         # cellpx[k][y][x] palette index, 0 = transparent
    for k in range(ncell):
        cellpx.append([[ (bls[y * (ncell * 4) + (k * 8 + x) // 2] >> (4 if (k * 8 + x) % 2 == 0 else 0)) & 15 for x in range(8)] for y in range(8)])
    even = bytearray(); odd = bytearray()
    for y in range(8):
        row = []
        for k in range(ncell): row += [(v if v else cloth) for v in cellpx[k][y]]
        even += bytes((row[i] << 4) | row[i + 1] for i in range(0, len(row), 2))
        row = []
        for k in range(ncell): row += [cloth] + [(v if v else cloth) for v in cellpx[k][y]] + [cloth]
        odd += bytes((row[i] << 4) | row[i + 1] for i in range(0, len(row), 2))
    bls_all = bytes(bls) + bytes(even) + bytes(odd)      # transparent 8x(96) | solid even 8x(96) | solid odd 8x(120)
    open(os.path.join(outdir, 'flat.txt'), 'w').write('%d %d %d %d %d\n' % (x0, x1, y0, y1, cloth))
    pb = bytearray()
    for r, g, b in pal: pb += bytes(((r << 4) | b, g))

    os.makedirs(outdir, exist_ok=True)
    open(os.path.join(outdir, 'table.bin'), 'wb').write(tbl)
    open(os.path.join(outdir, 'balls.bin'), 'wb').write(bls_all)
    open(os.path.join(outdir, 'palette.bin'), 'wb').write(pb)

    prev = Image.new('RGB', (256, 166 + 12)); pp = prev.load()
    for y in range(166):
        for x in range(256):
            v = (tbl[y * 128 + x // 2] >> (4 if x % 2 == 0 else 0)) & 15; pp[x, y] = tuple(min(255, c * 255 // 7) for c in pal[v])
    for y in range(8):
        for x in range(8 * len(cells)):
            v = (bls[y * (len(cells) * 4) + x // 2] >> (4 if x % 2 == 0 else 0)) & 15; pp[x, 168 + y] = tuple(min(255, c * 255 // 7) for c in pal[v])
    prev.resize((768, (166 + 12) * 3), Image.NEAREST).save(os.path.join(outdir, 'preview.png'))
    print('table %d B, balls %d B, flat cloth rect x %d..%d y %d..%d (colour %d)' % (len(tbl), len(bls_all), x0, x1, y0, y1, cloth))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
