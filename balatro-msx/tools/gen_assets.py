#!/usr/bin/env python3
"""Asset pipeline: Balatro PNG/TTF -> MSX2 Screen 5 (16 colours, 4 bpp) VRAM atlas.

  original/resources/textures/1x/*.png ─┐
  original/resources/fonts/m6x11plus.ttf ├─> palette + atlas (VRAM pages 1-3) + font + logo
                                          └─> src/seg/gfx_s*_b3.asm  (ASCII-8 mapper segments)
                                              include/assets_gen.h, src/gen/font_gen.c

If ./original is absent the previously generated files are kept (committed outputs)."""
import os, sys, json, re
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
        k = (a[:, 3] > 200) & (a[:, :3].min(1) < 215)            # ignore cream/white backgrounds
        px.append(a[k][:, :3])
    return np.concatenate(px)

jk = load('Jokers.png'); tr = load('Tarots.png'); dk = load('8BitDeck.png'); en = load('Enhancers.png'); bc = load('BlindChips.png')
from PIL import ImageEnhance, ImageFilter
JW, JH = 22, 30                              # interior of a 24x32 card cell (1 px border)
def fit_art(c, w=JW, h=JH, inner=(13, 8, 58, 87)):
    """Sprite cell -> w x h art. The cell is a full card (white, with JOKER side text): crop the interior,
    trim to the non-white bounding box, scale to fit, boost contrast/saturation, sharpen."""
    white = Image.new('RGBA', c.size, (255, 255, 255, 255)); white.alpha_composite(c)
    c = white.crop(inner)
    px = np.asarray(c.convert('RGB')).astype(int)
    ys, xs = np.where(px.min(2) < 225)
    c = c.crop((max(xs.min() - 1, 0), max(ys.min() - 1, 0), min(xs.max() + 2, c.width), min(ys.max() + 2, c.height)))
    sc = min(w / c.width, h / c.height)
    nw, nh = max(1, round(c.width * sc)), max(1, round(c.height * sc))
    r = c.convert('RGB').resize((nw, nh), Image.BOX if sc < 0.5 else Image.LANCZOS)
    r = ImageEnhance.Color(r).enhance(1.3); r = ImageEnhance.Contrast(r).enhance(1.2)
    r = r.filter(ImageFilter.UnsharpMask(radius=0.7, percent=70, threshold=2))
    ra = np.asarray(r).copy(); ra[(ra.min(2) > 238) | ((ra.min(2) > 195) & (ra.max(2) - ra.min(2) < 30))] = (245, 241, 234); r = Image.fromarray(ra)
    out = Image.new('RGBA', (w, h), (245, 241, 234, 255))
    out.paste(r.convert('RGBA'), ((w - nw) // 2, (h - nh) // 2))
    return out
def joker_art(k):
    c = cell(jk, *centers['j_' + k]['pos'])
    m = re.search(r'soul_pos\s*=\s*\{x=(\d+),\s*y=(\d+)\}', centers['j_' + k]['line'])
    if m:                                                   # legendary Jokers: floating soul layer over the base sprite
        c = c.copy(); c.alpha_composite(cell(jk, int(m.group(1)), int(m.group(2))))
    return fit_art(c)
joker_src = [joker_art(k) for k in C.JOKERS]
planet_src = [premult_resize(cell(tr, *centers['c_' + k]['pos']), (OUT_W, OUT_H)) for k, _ in C.PLANETS]
spectral_src = [premult_resize(cell(tr, *centers['c_' + k]['pos']), (OUT_W, OUT_H)) for k in C.SPECTRALS]
tarot_src = [premult_resize(cell(tr, *centers['c_' + k]['pos']), (OUT_W, OUT_H)) for k in C.TAROTS]

# learned colours: weighted k-means over the art; the anchors stay fixed
W3 = np.array([2, 4, 3], dtype=np.float32)
samples = gather(joker_src + planet_src + tarot_src + spectral_src)
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
BACKDROP = 3                       # slate: joker/voucher backdrop (art pops better than on cream)
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
    # corners stay opaque (the card is blitted with HMMM, which has no transparency)
    return a

# 5x5 pips: columns x = 8 / 13 / 18 never overlap; five y-slots
PX = {0: 7, 1: 12, 2: 17}
SY = [3, 9, 14, 20, 25]
def _p(spec): return [(c, SY[k] if isinstance(k, int) else int((SY[int(k)] + SY[int(k) + 1]) / 2)) for c, k in spec]
PIPS = {
 '2': _p([(1, 0), (1, 4)]), '3': _p([(1, 0), (1, 2), (1, 4)]),
 '4': _p([(0, 0), (2, 0), (0, 4), (2, 4)]), '5': _p([(0, 0), (2, 0), (1, 2), (0, 4), (2, 4)]),
 '6': _p([(0, 0), (2, 0), (0, 2), (2, 2), (0, 4), (2, 4)]),
 '7': _p([(0, 0), (2, 0), (1, 1), (0, 2), (2, 2), (0, 4), (2, 4)]),
 '8': _p([(0, 0), (2, 0), (1, 1), (0, 2), (2, 2), (1, 3), (0, 4), (2, 4)]),
 '9': _p([(0, 0), (2, 0), (0, 1), (2, 1), (1, 2), (0, 3), (2, 3), (0, 4), (2, 4)]),
 '10': _p([(0, 0), (2, 0), (1, .5), (0, 1), (2, 1), (0, 3), (2, 3), (1, 3.5), (0, 4), (2, 4)]),
}
BIG = {  # 5x7 face-card letters
 'J': ["..###", "...#.", "...#.", "...#.", "#..#.", "#..#.", ".##.."],
 'Q': [".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"],
 'K': ["#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"],
}
G3['1'] = ['#', '#', '#', '#', '#']          # slim '1' so "10" fits the index column

def rect(a, x0, y0, x1, y1, col):
    a[y0:y1 + 1, x0] = col; a[y0:y1 + 1, x1] = col; a[y0, x0:x1 + 1] = col; a[y1, x0:x1 + 1] = col

def make_card(si, ri):
    s, r = SUITS[si], RANKS[ri]
    col = SUIT_COL[s]
    a = card_base()
    x = 2                                         # corner index: rank over small suit
    for ch in r:
        blit(a, x, 3, G3[ch], col); x += len(G3[ch][0]) + 1
    blit(a, 2, 10, SUIT5[s], col)
    if r == 'A':
        blit(a, 11, 11, SUIT9[s], col)
    elif r in 'JQK':
        rect(a, 7, 3, 21, 28, 3)                  # framed big letter + suit marks
        blit(a, 9, 5, SUIT5[s], col); blit(a, 16, 22, SUIT5[s], col)
        for j, rr in enumerate(BIG[r]):           # letter drawn 2x wide x 1.. : 5x7 at (13,11) scaled 1x
            pass
        blit(a, 12, 11, BIG[r], col)
        blit(a, 11, 11, BIG[r], col)               # double-strike -> bold letter
    else:
        for (cx, py) in PIPS[r]:
            blit(a, PX[cx], py, SUIT5[s], col)
    return a

cards = [make_card(si, ri) for si in range(4) for ri in range(13)]
back = pal.quantize(premult_resize(cell(en, 0, 0), (OUT_W, OUT_H)))
blank = card_base()

# -------------------------------------------------------------- the atlas ----
CELLS_PER_ROW = 10
ATLAS_Y0 = 256                         # VRAM page 1
ATLAS_LINES = 512                      # pages 1-2 (8 mapper segments); joker art is streamed from ROM instead of living here
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
layout['planet'] = n
for im in planet_src: put_cell(n, pal.quantize(im)); n += 1
joker_cells = []                       # 24x32 joker cards: not in VRAM, streamed from ROM segments (see Vid_Joker)
for im in joker_src:
    c = card_base(); q = pal.quantize(im)
    c[1:31, 1:23] = q
    joker_cells.append(c)
layout['tarot'] = n
for im in tarot_src: put_cell(n, pal.quantize(im)); n += 1
layout['spectral'] = n
for im in spectral_src: put_cell(n, pal.quantize(im)); n += 1
layout['voucher'] = n
vc_img = load('Vouchers.png')
for k, _ in C.VOUCHERS:
    px, py = centers['v_' + k]['pos']
    im = fit_art(cell(vc_img, px, py))
    c = card_base(); q = pal.quantize(im)
    c[1:31, 1:23] = q
    put_cell(n, c); n += 1
layout['end'] = n
assert n <= 120, n

# blind chips: 16x16 icons in the right-hand column (x 240..255)
BLIND_ORDER = ['bl_small', 'bl_big'] + [k for k, b in sorted(blinds.items(), key=lambda kv: kv[1]['order']) if b['boss']]
blind_row = {}
BLIND_COL_N = 24                          # icons stacked in column x=240 (rows 0..383; the font strips start at row 384)
for i, k in enumerate(BLIND_ORDER):
    px, py = blinds[k]['pos']
    ic = premult_resize(bc.crop((0, py * 34, 34, py * 34 + 34)), (16, 16))
    if i < BLIND_COL_N:
        atlas[i * 16:(i + 1) * 16, 240:256] = pal.quantize(ic)
    else:                                     # the rest go into the free cells after the last card cell (two 16 px icons per cell)
        j = i - BLIND_COL_N; cell = layout['end'] + j // 2
        assert cell < 120 and cell // CELLS_PER_ROW == layout['end'] // CELLS_PER_ROW
        cx, cy = (cell % CELLS_PER_ROW) * OUT_W, (cell // CELLS_PER_ROW) * OUT_H + (j & 1) * 16
        atlas[cy:cy + 16, cx:cx + 16] = pal.quantize(ic)
    blind_row[k] = i

# --------------------------------------------------------------- previews ----
_jsheet = np.zeros((((len(joker_cells) + 14) // 15) * OUT_H, 15 * OUT_W), dtype=np.uint8)
for _i, _c in enumerate(joker_cells): _jsheet[(_i // 15) * OUT_H:(_i // 15 + 1) * OUT_H, (_i % 15) * OUT_W:(_i % 15 + 1) * OUT_W] = _c
_jrender = _jsheet
def render(idx, scale=3):
    rgb = np.array([to8(c) for c in PAL], dtype=np.uint8)
    im = Image.fromarray(rgb[idx], 'RGB')
    return im.resize((im.width * scale, im.height * scale), Image.NEAREST)
render(atlas[:, :]).save(os.path.join(PREVIEW, 'atlas.png'))
render(_jrender, 2).save(os.path.join(PREVIEW, 'jokers.png'))

# ------------------------------------------------------------------ tags -----
tg = load('tags.png')
tag_img = np.zeros((16, 256), dtype=np.uint8)
for i, (k, _) in enumerate(C.TAGS):
    tx, ty = centers['tag_' + k]['pos']
    ic = premult_resize(tg.crop((tx * 34, ty * 34, tx * 34 + 34, ty * 34 + 34)), (16, 16))
    tag_img[:, i * 16:(i + 1) * 16] = pal.quantize(ic)
render(tag_img, 4).save(os.path.join(PREVIEW, 'tags.png'))

# ------------------------------------------------------------------ logo ----
# The wordmark is redrawn natively for 16 colours instead of down-scaling the original art (which turned muddy): hand-drawn letters
# at 3x with a bounce, a 1 px ink outline, a slate drop shadow and a gold lower half, then the four suits between two gold rules.
import pixfont
lw, lh = 152, 60
logo = np.zeros((lh, lw), dtype=np.uint8)
def letter_mask(ch, k=3):
    top, rows = pixfont.GLYPHS[ch]
    w = max(len(r) for r in rows)
    m = np.zeros((7 * k, w * k), dtype=bool)
    for j, r in enumerate(rows):
        for i, c in enumerate(r):
            if c == '#': m[(top + j) * k:(top + j + 1) * k, i * k:(i + 1) * k] = True
    return m
def paint(dst, mask, x, y, col):
    h, w = mask.shape
    for j in range(h):
        for i in range(w):
            if mask[j, i] and 0 <= y + j < dst.shape[0] and 0 <= x + i < dst.shape[1]: dst[y + j, x + i] = col
def dilate(mask):
    out = np.pad(mask, 1)
    big = np.zeros_like(out)
    for dj in (-1, 0, 1):
        for di in (-1, 0, 1): big |= np.roll(np.roll(out, dj, 0), di, 1)
    return big
word, bounce = 'BALATRO', [0, -2, 1, -1, 2, -2, 1]
masks = [letter_mask(c) for c in word]
pitch = 18
x0 = (lw - (len(word) * pitch - 3)) // 2
for i, m in enumerate(masks):
    x, y = x0 + i * pitch, 13 + bounce[i]
    paint(logo, m, x + 2, y + 3, 3)                       # drop shadow (slate)
for i, m in enumerate(masks):
    x, y = x0 + i * pitch, 13 + bounce[i]
    paint(logo, dilate(m), x - 1, y - 1, 1)               # outline (ink)
for i, m in enumerate(masks):
    x, y = x0 + i * pitch, 13 + bounce[i]
    paint(logo, m, x, y, 2)                               # cream face
    low = m.copy(); low[:13, :] = False
    paint(logo, low, x, y, 6)                             # gold lower part
logo[48, 6:42] = 6; logo[48, 110:146] = 6                  # gold rules
suits = [('H', 4), ('C', 8), ('D', 7), ('S', 2)]
for n, (su, col) in enumerate(suits):
    sx = 44 + n * 18
    for j, row in enumerate(SUIT5[su]):
        for i, c in enumerate(row):
            if c == '#': logo[44 + j * 2:46 + j * 2, sx + i * 2:sx + i * 2 + 2] = col
# A hand-made / AI-redrawn logo can replace the procedural one: put a PNG (any size, white or transparent background) at
# assets/logo_src.png. It is trimmed, box-downscaled to fit 152x60, snapped to the 16-colour palette without dithering.
_src = os.path.join(ROOT, 'assets', 'logo_src.png')
if os.path.exists(_src):
    im = Image.open(_src).convert('RGBA')
    from collections import deque
    def _bgmask(arr):
        mn, mx = arr[..., :3].min(2), arr[..., :3].max(2)
        loose = (mn > 215) & (mx - mn < 14)                  # whitish: background if reachable from the border
        strict = (mn > 244) & (mx - mn < 7)                  # pure white: background anywhere (holes of B, R, O)
        H, W = loose.shape
        reach = np.zeros_like(loose)
        q = deque((y, x) for y in range(H) for x in (0, W - 1) if loose[y, x])
        q.extend((y, x) for x in range(W) for y in (0, H - 1) if loose[y, x])
        for y, x in q: reach[y, x] = True
        while q:
            y, x = q.popleft()
            for yy, xx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
                if 0 <= yy < H and 0 <= xx < W and loose[yy, xx] and not reach[yy, xx]:
                    reach[yy, xx] = True; q.append((yy, xx))
        return reach | strict | (arr[..., 3] < 20)
    px = np.asarray(im).astype(int)
    bg = _bgmask(px)
    ys, xs = np.where(~bg)
    im = im.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    a = np.asarray(im).astype(np.float32)
    a[..., 3] = np.where(_bgmask(a.astype(int)), 0, 255)
    im = Image.fromarray(a.astype(np.uint8), 'RGBA')
    sc = min((lw - 4) / im.width, (lh - 2) / im.height)
    sz = (max(2, int(round(im.width * sc)) // 2 * 2), max(2, int(round(im.height * sc))))
    q = pal.quantize(premult_resize(im, sz), alpha_cut=110)
    logo = np.zeros((lh, lw), dtype=np.uint8)
    ox, oy = (lw - sz[0]) // 2 // 2 * 2, (lh - sz[1]) // 2
    logo[oy:oy + sz[1], ox:ox + sz[0]] = q
    print('logo: using assets/logo_src.png ->', sz)
render(logo, 4).save(os.path.join(PREVIEW, 'logo.png'))

# ------------------------------------------------------------------ font ----
# m6x11plus glyphs -> proportional strips pre-rendered in several colours inside the VRAM atlas,
# so text is drawn with the VDP (one LMMM per character) instead of streaming pixels from the CPU.
import pixfont
GLYPH_TOP, GLYPH_ROWS = 0, 9
FONT_COLORS = [2, 1, 6, 4, 5, 8, 3]          # palette index: cream, ink, gold, red, blue, green, slate
glyphs = []
for code in range(32, 127):
    w, grid = pixfont.bitmap(chr(code), GLYPH_ROWS)
    glyphs.append((w, np.array(grid, dtype=bool)))
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
TAG_SEG = LOGO_SEG + 1
tag_bytes = pack4(tag_img)
# joker cards: only the 24x30 body (rows 1..30 of the cell; the top/bottom border rows are drawn with fills),
# 360 bytes each, packed into the free tails of the logo and tag segments and then into whole segments (not 2/3: main code)
JOKER_BYTES = OUT_W * (OUT_H - 2) // 2
spans = [(LOGO_SEG, len(logo_bytes)), (TAG_SEG, len(tag_bytes))] + [(sg, 0) for sg in range(TAG_SEG + 1, 20)]
seg_buf = {LOGO_SEG: bytearray(logo_bytes), TAG_SEG: bytearray(tag_bytes)}
joker_seg, joker_off = [], []
si = 0
for c in joker_cells:
    while si < len(spans) and len(seg_buf.get(spans[si][0], b'')) + JOKER_BYTES > SEG and spans[si][1] >= 0 and (spans[si][0] in seg_buf or JOKER_BYTES > SEG):
        si += 1
    assert si < len(spans), 'out of ROM segments for joker art'
    sg = spans[si][0]
    buf = seg_buf.setdefault(sg, bytearray())
    if len(buf) + JOKER_BYTES > SEG:
        si += 1; assert si < len(spans), 'out of ROM segments for joker art'
        sg = spans[si][0]; buf = seg_buf.setdefault(sg, bytearray())
    joker_seg.append(sg); joker_off.append(len(buf))
    buf += pack4(c[1:OUT_H - 1, :])
for sg, buf in seg_buf.items():
    assert len(buf) <= SEG
    write_seg(sg, bytes(buf))
JOKER_SEGS = sorted({x for x in joker_seg})
# remove stale seg files
for fn in os.listdir(OUT_SEG):
    if fn.startswith('seg_s') and fn.endswith('_b3.asm') and os.path.join(OUT_SEG, fn) not in seg_files:
        os.remove(os.path.join(OUT_SEG, fn))

with open(os.path.join(OUT_GEN, 'font_gen.c'), 'w') as f:
    f.write('// GENERATED by tools/gen_assets.py from m6x11plus.ttf (see LICENSES.md)\n#include "gtypes.h"\n')
    f.write('const u8 g_FontW[95] = {' + ','.join(str(w) for w, _ in glyphs) + '};\n')
    f.write('const u8 g_FontX[95] = {' + ','.join(str(p[0]) for p in pos) + '};\n')
    f.write('const u8 g_FontL[95] = {' + ','.join(str(p[1]) for p in pos) + '};\n')
    f.write('// joker art location in ROM: mapper segment and offset inside bank 3 (0xA000 + offset)\n')
    f.write('const u8 g_JokerSeg[%d] = {' % len(joker_seg) + ','.join(str(x) for x in joker_seg) + '};\n')
    f.write('const u16 g_JokerOff[%d] = {' % len(joker_off) + ','.join(str(x) for x in joker_off) + '};\n')

with open(os.path.join(OUT_INC, 'assets_gen.h'), 'w') as f:
    f.write('// GENERATED by tools/gen_assets.py - do not edit\n#pragma once\n')
    f.write('#define GFX_PALETTE_INIT { %s }\n' % ', '.join('0x%02X, 0x%02X' % (((r << 4) | b), g) for r, g, b in PAL))
    f.write('#define GFX_ATLAS_FIRST_SEG %d\n#define GFX_ATLAS_SEGS %d\n#define GFX_ATLAS_LINES %d\n#define GFX_ATLAS_Y0 %d\n' % (FIRST_SEG, nseg, ATLAS_LINES, ATLAS_Y0))
    f.write('#define GFX_LOGO_SEG %d\n#define GFX_LOGO_W %d\n#define GFX_LOGO_H %d\n' % (LOGO_SEG, lw, lh))
    f.write('#define GFX_TAG_SEG %d\n#define GFX_TAG_Y 212\n' % TAG_SEG)
    f.write('#define GFX_CELL_W %d\n#define GFX_CELL_H %d\n#define GFX_CELLS_PER_ROW %d\n' % (OUT_W, OUT_H, CELLS_PER_ROW))
    for k in ('card', 'back', 'blank', 'planet', 'tarot', 'spectral', 'voucher'):
        f.write('#define CELL_%s %d\n' % (k.upper(), layout[k]))
    f.write('#define FONT_ROWS %d\n#define FONT_Y0 %d\n#define FONT_STRIP_H %d\n#define FONT_COLOR_COUNT %d\n' % (GLYPH_ROWS, FONT_Y0, FONT_STRIP_H, len(FONT_COLORS)))
    f.write('#define BLIND_ICON_COUNT %d\n#define BLIND_COL_N %d\n#define BLIND_EXTRA_CELL %d\n' % (len(BLIND_ORDER), BLIND_COL_N, layout['end']))
    f.write('#define GFX_JOKER_BYTES %d\n' % JOKER_BYTES)
    for nm, rgb in (('ICE', (170, 215, 250)), ('PURPLE', (150, 90, 210)), ('STEEL', (110, 130, 145)), ('STONE', (130, 130, 130))):
        f.write('#define COL_%s %d\n' % (nm, int(pal.nearest(np.array(rgb, dtype=np.float32)))))
    f.write('// blind icon rows (16px each) in atlas column x=240: ' + ', '.join('%s=%d' % (k, v) for k, v in blind_row.items()) + '\n')
json.dump(dict(blind_order=BLIND_ORDER), open(os.path.join(ROOT, 'build', 'blind_order.json'), 'w'))
print("joker art: %d cards in segments %s" % (len(joker_cells), JOKER_SEGS))
print("atlas: %d cells used, %d bytes in %d segments; logo seg %d; font %d glyphs" % (layout['end'], len(blob), nseg, LOGO_SEG, len(glyphs)))
