#!/usr/bin/env python3
"""Convert screenshots of the original screens (320x224) into MSX2 Screen 5 assets (256x212, 16 colours).

    extract_screens.py <captures-dir> <gen-dir>

Needs the PPM captures produced by tools/emu/cap_*.txt (see build_msx.sh).  Writes <name>.asset files:
    [w bytes][h][flags bit0 = palette][xb][y] [32 byte palette] [LZSS rows]
and <gen>/screens_preview.png.  States that change (menu entry, 3/7/15, blinking text) are exported as small
patches drawn over the base image.
"""
import os, sys
from PIL import Image
sys.path.insert(0, os.path.dirname(__file__))
import lzss

W, H = 256, 212
def load(d, n): return Image.open(os.path.join(d, n + '.ppm')).convert('RGB')
def snap(v): return round(v * 7 / 255)

def scale(im, box=None, size=(W, H)):
    if box: im = im.crop(box)
    return im.resize(size, Image.LANCZOS)

def make_palette(images, extra=()):
    """16-entry 3-bit palette adapted to the given RGB images (+ forced extra colours)."""
    n = 16 - len(extra)
    sheet = Image.new('RGB', (images[0].width, sum(i.height for i in images)))
    y = 0
    for i in images: sheet.paste(i, (0, y)); y += i.height
    sheet = sheet.point(lambda v: round(v * 7 / 255) * 255 // 7)
    q = sheet.quantize(colors=n, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    raw = q.getpalette()[:n * 3]
    pal = [tuple(snap(c) for c in raw[i * 3:i * 3 + 3]) for i in range(n)]
    pal += list(extra)
    while len(pal) < 16: pal.append((0, 0, 0))
    return pal

def palette_image(pal):
    p = Image.new('P', (1, 1))
    flat = []
    for r, g, b in pal: flat += [r * 255 // 7, g * 255 // 7, b * 255 // 7]
    p.putpalette(flat + [0] * (768 - len(flat)))
    return p

def to_indices(im, pal, dither=True):
    q = im.quantize(palette=palette_image(pal), dither=Image.Dither.FLOYDSTEINBERG if dither else Image.Dither.NONE)
    return q

def pack(idx_img, box=None):
    """-> (xb, y, wbytes, h, bytes) of the (even-aligned) box of an indexed image."""
    x0, y0, x1, y1 = box or (0, 0, idx_img.width, idx_img.height)
    x0 &= ~1; x1 = (x1 + 1) & ~1
    px = idx_img.load(); data = bytearray()
    for y in range(y0, y1):
        for x in range(x0, x1, 2): data.append((px[x, y] << 4) | px[x + 1, y])
    return x0 // 2, y0, (x1 - x0) // 2, y1 - y0, bytes(data)

def asset(name, outdir, xb, y, w, h, data, pal=None, raw=False):
    flags = (1 if pal else 0) | (2 if raw else 0)
    hdr = bytes((w, h, flags, xb, y))
    if pal: hdr += b''.join(bytes(((r << 4) | b, g)) for r, g, b in pal)
    comp = data if raw else lzss.compress(data)
    if not raw: assert lzss.decompress(comp, len(data)) == data
    open(os.path.join(outdir, name + '.asset'), 'wb').write(hdr + comp)
    print('  %-14s %3dx%3d px at (%3d,%3d)  %6d -> %6d B' % (name, w * 2, h, xb * 2, y, len(data), len(comp)))
    return len(comp)

def diff_box(a, b, margin=0):
    pa, pb = a.load(), b.load(); xs, ys = [], []
    for y in range(a.height):
        for x in range(a.width):
            if pa[x, y] != pb[x, y]: xs.append(x); ys.append(y)
    if not xs: return None
    return (max(0, min(xs) - margin), max(0, min(ys) - margin), min(a.width, max(xs) + 1 + margin), min(a.height, max(ys) + 1 + margin))

def diff_box_rgb(a, b, thr=60, margin=2):
    """bounding box where two RGB images differ clearly (immune to dithering noise)"""
    pa, pb = a.load(), b.load(); xs, ys = [], []
    for y in range(a.height):
        for x in range(a.width):
            if sum(abs(pa[x, y][i] - pb[x, y][i]) for i in range(3)) > thr: xs.append(x); ys.append(y)
    if not xs: return None
    return (max(0, min(xs) - margin), max(0, min(ys) - margin), min(a.width, max(xs) + 1 + margin), min(a.height, max(ys) + 1 + margin))

def union(boxes):
    boxes = [b for b in boxes if b]
    return (min(b[0] for b in boxes), min(b[1] for b in boxes), max(b[2] for b in boxes), max(b[3] for b in boxes))

def preview(imgs, outdir):
    sheet = Image.new('RGB', (W * 3, H * ((len(imgs) + 2) // 3)))
    for i, im in enumerate(imgs): sheet.paste(im, ((i % 3) * W, (i // 3) * H))
    sheet.save(os.path.join(outdir, 'screens_preview.png'))

def main(cap, out):
    total = 0; prev = []; layout = {}
    def rgb(idx, pal):
        im = idx.convert('RGB'); return im
    # ---- title (+ blinking "PRESS START BUTTON" patches)
    t_on, t_bg = scale(load(cap, 'title')), scale(load(cap, 'title_bg'))
    pal = make_palette([t_on])
    i_on, i_bg = to_indices(t_on, pal), to_indices(t_bg, pal)
    total += asset('title', out, *pack(i_on)[:4], pack(i_on)[4], pal)
    band = (16, 152, 112, 168)        # "PRESS START BUTTON" in the 256x212 image
    total += asset('title_on', out, *pack(i_on, band)[:4], pack(i_on, band)[4], raw=True)
    total += asset('title_off', out, *pack(i_bg, band)[:4], pack(i_bg, band)[4], raw=True)
    prev.append(i_on.convert('RGB'))
    # ---- main menu: base + one patch per highlighted entry
    ms = [scale(load(cap, 'menu%d' % k)) for k in range(5)]
    pal = make_palette([ms[0]])
    idx = [to_indices(m, pal) for m in ms]
    box = union([diff_box_rgb(ms[0], ms[k]) for k in range(1, 5)])
    print('  menu patch box', box)
    total += asset('menu', out, *pack(idx[0])[:4], pack(idx[0])[4], pal)
    for k in range(5): total += asset('menu_s%d' % k, out, *pack(idx[k], box)[:4], pack(idx[k], box)[4], raw=True)
    prev.append(idx[0].convert('RGB'))
    # ---- 2P VS screens: base (nine ball, 3 selected) + 3/7/15 patches + "pocket game" patch
    vs = {k: scale(load(cap, n)) for k, n in (('3', 'vs_9ball3'), ('7', 'vs_9ball7'), ('15', 'vs_9ball15'), ('p', 'vs_pocket'))}
    pal = make_palette([vs['3'], vs['7'], vs['15'], vs['p']])
    iv = {k: to_indices(v, pal) for k, v in vs.items()}
    total += asset('vs', out, *pack(iv['3'])[:4], pack(iv['3'])[4], pal)
    box = union([diff_box_rgb(vs['3'], vs['7']), diff_box_rgb(vs['3'], vs['15'])])
    print('  race patch box', box)
    for k in ('3', '7', '15'): total += asset('vs_r%s' % k, out, *pack(iv[k], box)[:4], pack(iv[k], box)[4], raw=True)
    box = diff_box_rgb(vs['3'], vs['p'], thr=90)
    print('  pocket patch box', box)
    total += asset('vs_pocket', out, *pack(iv['p'], box)[:4], pack(iv['p'], box)[4], raw=True)
    prev += [iv['3'].convert('RGB'), iv['p'].convert('RGB')]
    # ---- trick select
    tk = scale(load(cap, 'trick0')); pal = make_palette([tk], extra=[(7, 7, 7), (0, 0, 0), (7, 7, 1), (0, 7, 0)])   # 12 white, 13 black, 14 yellow title, 15 cursor green
    it = to_indices(tk, pal)
    px = tk.load(); bx = [(x, y) for y in range(55, H) for x in range(W) if px[x, y][2] > 150 and px[x, y][2] > px[x, y][0] + 80 and px[x, y][2] > px[x, y][1] + 40]
    gx0, gx1 = min(p[0] for p in bx), max(p[0] for p in bx) + 1; gy0, gy1 = min(p[1] for p in bx), max(p[1] for p in bx) + 1
    layout['TRICK_GX'] = gx0; layout['TRICK_GY'] = gy0; layout['TRICK_GW'] = gx1 - gx0; layout['TRICK_GH'] = gy1 - gy0
    layout['TK_GREEN'] = 15; layout['TK_YELLOW'] = 14; layout['TK_BLACK'] = 13; layout['TK_WHITE'] = 12
    total += asset('trick', out, *pack(it)[:4], pack(it)[4], pal)
    prev.append(it.convert('RGB'))
    # ---- jukebox (the name box interior is redrawn with text from the game)
    jk = scale(load(cap, 'juke0')); pal = make_palette([jk], extra=[(7, 7, 7)])     # index 15 = white text
    ij = to_indices(jk, pal)
    from collections import Counter
    jb = (62, 143, 194, 152); jp = ij.load()
    bgc = Counter(jp[x, y] for y in range(jb[1], jb[3]) for x in range(jb[0], jb[2])).most_common(1)[0][0]
    layout.update(JK_X=jb[0], JK_Y=jb[1], JK_W=jb[2] - jb[0], JK_H=jb[3] - jb[1], JK_BG=bgc, JK_FG=15)
    total += asset('juke', out, *pack(ij)[:4], pack(ij)[4], pal)
    prev.append(ij.convert('RGB'))
    # ---- stage 1 portrait (left half of the intro screen)
    st = scale(load(cap, 'stage1'), box=(0, 0, 150, 224), size=(120, 212)); pal = make_palette([st], extra=[(6, 0, 0), (7, 7, 7), (0, 0, 0)])   # 13 red, 14 white, 15 black
    layout.update(ST_RED=13, ST_WHITE=14, ST_BLACK=15)
    isg = to_indices(st, pal)
    total += asset('stage1', out, *pack(isg)[:4], pack(isg)[4], pal)
    prev.append(isg.convert('RGB').resize((W, H), Image.NEAREST))
    preview(prev, out)
    with open(os.path.join(out, 'layout.txt'), 'w') as lf:
        for k, v in layout.items(): lf.write('%s %d\n' % (k, v))
    print('screens total: %d bytes' % total)

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
