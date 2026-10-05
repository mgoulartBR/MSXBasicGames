#!/usr/bin/env python3
"""Asset pipeline: Balatro PNG/TTF -> MSX2 Screen 5 (16 colours, 4 bpp) VRAM atlas.

  original/resources/textures/1x/*.png ─┐
  original/resources/fonts/m6x11plus.ttf ├─> palette + atlas (VRAM pages 1-3) + font + logo
                                          └─> src/seg/gfx_s*_b3.asm  (ASCII-8 mapper segments)
                                              include/assets_gen.h, src/gen/font_gen.c

If ./original is absent the previously generated files are kept (committed outputs)."""
import os, sys, json
import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lua_data as L
import content as C
from gfxlib import *

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
OUT_SEG = os.path.join(ROOT, 'src', 'seg')
OUT_GEN = os.path.join(ROOT, 'src', 'gen')
OUT_INC = os.path.join(ROOT, 'include')
PREVIEW = os.path.join(ROOT, 'build', 'gfx_preview')

if not L.have_original():
    print("gen_assets: ./original not present - keeping committed generated assets")
    sys.exit(0)
for d in (OUT_SEG, OUT_GEN, OUT_INC, PREVIEW):
    os.makedirs(d, exist_ok=True)

def load(name):
    return Image.open(os.path.join(L.TEX, name)).convert('RGBA')

centers = L.centers()
blinds = L.blinds()

# ---------------------------------------------------------------- palette ----
ANCHORS = ['#2f5a46',  # 0 felt (also "transparent" in sprite copies)
           '#1f2c34',  # 1 ink / spades
           '#f4f1ea',  # 2 card cream
           '#5d7077',  # 3 slate
           '#fe5f55',  # 4 red   (mult / hearts)
           '#0b9cfb',  # 5 blue  (chips)
           '#f3b43a',  # 6 gold  (money)
           '#fd6a2a',  # 7 orange (diamonds)
           '#2e8b5f']  # 8 green (clubs)
NLEARN = 16 - len(ANCHORS)
pal = Palette(ANCHORS)

def gather(imgs):
    px = []
    for im in imgs:
        a = np.asarray(im.convert('RGBA'), dtype=np.float32).reshape(-1, 4)
        px.append(a[a[:, 3] > 200][:, :3])
    return np.concatenate(px)

jk = load('Jokers.png'); tr = load('Tarots.png'); dk = load('8BitDeck.png'); en = load('Enhancers.png'); bc = load('BlindChips.png')
def joker_art(k):
    c = cell(jk, *centers['j_' + k]['pos'])
    return premult_resize(c.crop((9, 7, 62, 88)), (20, 28))
joker_src = [joker_art(k) for k in C.JOKERS]
planet_src = [premult_resize(cell(tr, *centers['c_' + k]['pos']), (OUT_W, OUT_H)) for k, _ in C.PLANETS]
tarot_src = [premult_resize(cell(tr, *centers['c_' + k]['pos']), (OUT_W, OUT_H)) for k in C.TAROTS]

# learned colours: weighted k-means over the art; the anchors stay fixed
W3 = np.array([2, 4, 3], dtype=np.float32)
samples = gather(joker_src + planet_src + tarot_src)
rs = np.random.RandomState(7)
samples = samples[rs.permutation(len(samples))[:30000]]
anchors = np.array([to8(hex3(a)) for a in ANCHORS], dtype=np.float32)
cent = []
allc = list(anchors)
for _ in range(NLEARN):                      # k-means++ style seeding on the worst-represented pixels
    dd = np.array([(((samples - c) ** 2) * W3).sum(1) for c in allc]).min(0)
    pick = samples[rs.choice(len(samples), p=dd / dd.sum())]
    allc.append(pick)
cent = np.array(allc[len(ANCHORS):])
for _ in range(30):
    allc = np.concatenate([anchors, cent])
    lab = (((samples[:, None, :] - allc[None]) ** 2) * W3).sum(-1).argmin(1)
    for i in range(NLEARN):
        m = lab == len(ANCHORS) + i
        if m.any():
            cent[i] = samples[m].mean(0)
learned = [tuple(int(round(v * 7 / 255)) for v in c) for c in cent]
pal.finish(learned)
PAL = pal.colors
print("palette:", ['%d%d%d' % c for c in PAL])

# --------------------------------------------------------- playing cards ----
G3 = {  # 3x5 rank glyphs
 '0': ["###", "#.#", "#.#", "#.#", "###"], '1': [".#.", "##.", ".#.", ".#.", "###"],
 '2': ["###", "..#", "###", "#..", "###"], '3': ["###", "..#", "###", "..#", "###"],
 '4': ["#.#", "#.#", "###", "..#", "..#"], '5': ["###", "#..", "###", "..#", "###"],
 '6': ["###", "#..", "###", "#.#", "###"], '7': ["###", "..#", ".#.", ".#.", ".#."],
 '8': ["###", "#.#", "###", "#.#", "###"], '9': ["###", "#.#", "###", "..#", "###"],
 'A': [".#.", "#.#", "###", "#.#", "#.#"], 'J': ["..#", "..#", "..#", "#.#", ".#."],
 'Q': ["###", "#.#", "#.#", "###", "..#"], 'K': ["#.#", "#.#", "##.", "#.#", "#.#"],
}
SUIT5 = {  # 5x5 suit marks, order: H C D S
 'H': ["##.##", "#####", "#####", ".###.", "..#.."],
 'C': [".###.", ".###.", "#####", "#.#.#", "..#.."],
 'D': ["..#..", ".###.", "#####", ".###.", "..#.."],
 'S': ["..#..", ".###.", "#####", "#####", "..#.."],
}
SUIT9 = {  # 9x9 marks for aces
 'H': [".##...##.", "####.####", "#########", "#########", ".#######.", "..#####..", "...###...", "....#....", "........."],
 'C': ["...###...", "..#####..", "..#####..", ".##.#.##.", "#########", "#########", ".##.#.##.", "....#....", "...###..."],
 'D': ["....#....", "...###...", "..#####..", ".#######.", "#########", ".#######.", "..#####..", "...###...", "....#...."],
 'S': ["....#....", "...###...", "..#####..", ".#######.", "#########", "#########", ".##.#.##.", "....#....", "...###..."],
}
SUITS = 'HCDS'                     # sheet rows: hearts, clubs, diamonds, spades
SUIT_COL = {'H': 4, 'C': 8, 'D': 7, 'S': 1}
RANKS = ['2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A']

def blit(a, x, y, rows, col, w=None):
    for j, r in enumerate(rows):
        for i, ch in enumerate(r):
            if ch == '#' and 0 <= y + j < a.shape[0] and 0 <= x + i < a.shape[1]:
                a[y + j, x + i] = col

def card_base():
    a = np.full((OUT_H, OUT_W), 2, dtype=np.uint8)      # cream
    a[0, :] = a[-1, :] = 3; a[:, 0] = a[:, -1] = 3       # slate outline
    for (x, y) in ((0, 0), (OUT_W - 1, 0), (0, OUT_H - 1), (OUT_W - 1, OUT_H - 1)):
        a[y, x] = 0                                       # rounded corners (transparent)
    return a

PIPS = {  # pip positions on a 3-column grid (cx: 0=left,1=mid,2=right), rows 0..4 (top..bottom)
 '2': [(1, 0), (1, 4)], '3': [(1, 0), (1, 2), (1, 4)], '4': [(0, 0), (2, 0), (0, 4), (2, 4)],
 '5': [(0, 0), (2, 0), (1, 2), (0, 4), (2, 4)], '6': [(0, 0), (2, 0), (0, 2), (2, 2), (0, 4), (2, 4)],
 '7': [(0, 0), (2, 0), (1, 1), (0, 2), (2, 2), (0, 4), (2, 4)],
 '8': [(0, 0), (2, 0), (1, 1), (0, 2), (2, 2), (1, 3), (0, 4), (2, 4)],
 '9': [(0, 0), (2, 0), (0, 1), (2, 1), (1, 2), (0, 3), (2, 3), (0, 4), (2, 4)],
 '10': [(0, 0), (2, 0), (1, 0.5), (0, 1.5), (2, 1.5), (0, 2.5), (2, 2.5), (1, 3.5), (0, 4), (2, 4)],
}

def make_card(si, ri):
    s, r = SUITS[si], RANKS[ri]
    col = SUIT_COL[s]
    a = card_base()
    # corner index: rank (3x5) + small suit (5x5 -> drawn 5 wide) under it
    x = 2
    for ch in r:
        blit(a, x, 3, G3[ch], col); x += 4
    blit(a, 1, 10, SUIT5[s], col)
    if r == 'A':
        blit(a, 10, 12, SUIT9[s], col)
    elif r in 'JQK':
        # figure from the original art, scaled to 12x20 in the centre
        f = cell(dk, 9 + 'JQK'.index(r), si).crop((10, 6, 61, 89))
        f = premult_resize(f, (12, 20))
        q = pal.quantize(f)
        for j in range(20):
            for i in range(12):
                if q[j, i]:
                    a[6 + j, 10 + i] = q[j, i]
    else:
        for (cx, cy) in PIPS[r]:
            px = 11 + (cx - 1) * 5 - 0
            py = 5 + int(round(cy * 5.5))
            blit(a, px - 1 + (1 if cx == 1 else 0) * 0, py, SUIT5[s], col)
    return a

cards = [make_card(si, ri) for si in range(4) for ri in range(13)]
back = pal.quantize(premult_resize(cell(en, 0, 0), (OUT_W, OUT_H)))
blank = card_base()

# -------------------------------------------------------------- the atlas ----
CELLS_PER_ROW = 10
ATLAS_Y0 = 256                         # VRAM page 1
ATLAS_LINES = 736                      # pages 1-3 minus 32 lines reserved for the sprite tables
atlas = np.zeros((ATLAS_LINES, 256), dtype=np.uint8)

def put_cell(idx, arr):
    cx, cy = (idx % CELLS_PER_ROW) * OUT_W, (idx // CELLS_PER_ROW) * OUT_H
    atlas[cy:cy + arr.shape[0], cx:cx + arr.shape[1]] = arr

layout = {}
n = 0
layout['card'] = n
for c in cards: put_cell(n, c); n += 1
layout['back'] = n; put_cell(n, back); n += 1
layout['blank'] = n; put_cell(n, blank); n += 1
n = 60
layout['planet'] = n
for im in planet_src: put_cell(n, pal.quantize(im)); n += 1
n = 80
layout['joker'] = n
for im in joker_src:
    c = card_base(); q = pal.quantize(im, alpha_cut=100)
    sub = c[2:30, 2:22]; sub[q != 0] = q[q != 0]
    put_cell(n, c); n += 1
n = max(n, 160)
layout['tarot'] = n
for im in tarot_src: put_cell(n, pal.quantize(im)); n += 1
layout['end'] = n
assert n <= 230, n

# blind chips: 16x16 icons in the right-hand column (x 240..255)
BLIND_ORDER = ['bl_small', 'bl_big'] + [k for k, b in sorted(blinds.items(), key=lambda kv: kv[1]['order']) if b['boss']]
blind_row = {}
for i, k in enumerate(BLIND_ORDER):
    px, py = blinds[k]['pos']
    ic = premult_resize(bc.crop((0, py * 34, 34, py * 34 + 34)), (16, 16))
    atlas[i * 16:(i + 1) * 16, 240:256] = pal.quantize(ic)
    blind_row[k] = i
assert len(BLIND_ORDER) * 16 <= ATLAS_LINES

# --------------------------------------------------------------- previews ----
def render(idx, scale=3):
    rgb = np.array([to8(c) for c in PAL], dtype=np.uint8)
    im = Image.fromarray(rgb[idx], 'RGB')
    return im.resize((im.width * scale, im.height * scale), Image.NEAREST)
render(atlas[:, :]).save(os.path.join(PREVIEW, 'atlas.png'))

# ------------------------------------------------------------------ logo ----
logo_im = load('balatro.png')
lw, lh = 152, 98
logo = pal.quantize(premult_resize(logo_im, (lw, lh)))
render(logo, 4).save(os.path.join(PREVIEW, 'logo.png'))

# ------------------------------------------------------------------ font ----
# m6x11plus glyphs -> proportional strips pre-rendered in several colours inside the VRAM atlas,
# so text is drawn with the VDP (one LMMM per character) instead of streaming pixels from the CPU.
font = ImageFont.truetype(os.path.join(L.ORIG, 'resources', 'fonts', 'm6x11plus.ttf'), 11)
GLYPH_TOP, GLYPH_ROWS = 3, 9
FONT_COLORS = [2, 1, 6, 4, 5, 8, 3]          # palette index: cream, ink, gold, red, blue, green, slate
glyphs = []
for code in range(32, 127):
    ch = chr(code)
    im = Image.new('L', (12, 14), 0)
    ImageDraw.Draw(im).text((0, 0), ch, font=font, fill=255)
    a = np.asarray(im) > 127
    cols = np.where(a.any(0))[0]
    width = int(cols.max()) + 2 if len(cols) else 3          # ink + 1px spacing
    if ch == ' ': width = 3
    glyphs.append((width, a[GLYPH_TOP:GLYPH_TOP + GLYPH_ROWS, :width].copy()))
# lay glyphs on strip lines of <= 256 px
pos = []; x = 0; line = 0
for w, _ in glyphs:
    if x + w > 256: line += 1; x = 0
    pos.append((x, line)); x += w
FONT_LINES = line + 1
FONT_STRIP_H = FONT_LINES * GLYPH_ROWS
FONT_ROW0 = ((layout['end'] + CELLS_PER_ROW - 1) // CELLS_PER_ROW) * OUT_H      # first free atlas row
assert FONT_ROW0 + FONT_STRIP_H * len(FONT_COLORS) <= ATLAS_LINES, (FONT_ROW0, FONT_STRIP_H)
for ci, col in enumerate(FONT_COLORS):
    for (w, bits), (gx, gl) in zip(glyphs, pos):
        y0 = FONT_ROW0 + ci * FONT_STRIP_H + gl * GLYPH_ROWS
        for r in range(GLYPH_ROWS):
            for c in range(w):
                if bits[r, c]: atlas[y0 + r, gx + c] = col
FONT_Y0 = ATLAS_Y0 + FONT_ROW0
render(atlas[FONT_ROW0:FONT_ROW0 + FONT_STRIP_H * 2], 3).save(os.path.join(PREVIEW, 'font.png'))

# ------------------------------------------------------------- write out ----
blob = pack4(atlas)                         # 128 bytes per line
assert len(blob) == ATLAS_LINES * 128
SEG = 8192
seg_files = []
def write_seg(segno, data):
    p = os.path.join(OUT_SEG, 'seg_s%d_b3.asm' % segno)
    with open(p, 'w') as f:
        f.write('; GENERATED by tools/gen_assets.py - do not edit\n\t.area _SEG%d\n' % segno)
        for i in range(0, len(data), 16):
            f.write('\t.db ' + ','.join('0x%02X' % b for b in data[i:i + 16]) + '\n')
    seg_files.append(p)
FIRST_SEG = 4
nseg = (len(blob) + SEG - 1) // SEG
for s in range(nseg):
    write_seg(FIRST_SEG + s, blob[s * SEG:(s + 1) * SEG])
LOGO_SEG = FIRST_SEG + nseg
logo_bytes = pack4(logo)
assert len(logo_bytes) <= SEG
write_seg(LOGO_SEG, logo_bytes)
# remove stale seg files
for fn in os.listdir(OUT_SEG):
    if fn.startswith('seg_s') and fn.endswith('_b3.asm') and os.path.join(OUT_SEG, fn) not in seg_files:
        os.remove(os.path.join(OUT_SEG, fn))

with open(os.path.join(OUT_GEN, 'font_gen.c'), 'w') as f:
    f.write('// GENERATED by tools/gen_assets.py from m6x11plus.ttf (see LICENSES.md)\n#include "gtypes.h"\n')
    f.write('const u8 g_FontW[95] = {' + ','.join(str(w) for w, _ in glyphs) + '};\n')
    f.write('const u8 g_FontX[95] = {' + ','.join(str(p[0]) for p in pos) + '};\n')
    f.write('const u8 g_FontL[95] = {' + ','.join(str(p[1]) for p in pos) + '};\n')

with open(os.path.join(OUT_INC, 'assets_gen.h'), 'w') as f:
    f.write('// GENERATED by tools/gen_assets.py - do not edit\n#pragma once\n')
    f.write('#define GFX_PALETTE_INIT { %s }\n' % ', '.join('0x%02X, 0x%02X' % (((r << 4) | b), g) for r, g, b in PAL))
    f.write('#define GFX_ATLAS_FIRST_SEG %d\n#define GFX_ATLAS_SEGS %d\n#define GFX_ATLAS_LINES %d\n#define GFX_ATLAS_Y0 %d\n' % (FIRST_SEG, nseg, ATLAS_LINES, ATLAS_Y0))
    f.write('#define GFX_LOGO_SEG %d\n#define GFX_LOGO_W %d\n#define GFX_LOGO_H %d\n' % (LOGO_SEG, lw, lh))
    f.write('#define GFX_CELL_W %d\n#define GFX_CELL_H %d\n#define GFX_CELLS_PER_ROW %d\n' % (OUT_W, OUT_H, CELLS_PER_ROW))
    for k in ('card', 'back', 'blank', 'planet', 'joker', 'tarot'):
        f.write('#define CELL_%s %d\n' % (k.upper(), layout[k]))
    f.write('#define FONT_ROWS %d\n#define FONT_Y0 %d\n#define FONT_STRIP_H %d\n#define FONT_COLOR_COUNT %d\n' % (GLYPH_ROWS, FONT_Y0, FONT_STRIP_H, len(FONT_COLORS)))
    f.write('#define BLIND_ICON_COUNT %d\n' % len(BLIND_ORDER))
    f.write('// blind icon rows (16px each) in atlas column x=240: ' + ', '.join('%s=%d' % (k, v) for k, v in blind_row.items()) + '\n')
json.dump(dict(blind_order=BLIND_ORDER), open(os.path.join(ROOT, 'build', 'blind_order.json'), 'w'))
print("atlas: %d cells used, %d bytes in %d segments; logo seg %d; font %d glyphs" % (layout['end'], len(blob), nseg, LOGO_SEG, len(glyphs)))
