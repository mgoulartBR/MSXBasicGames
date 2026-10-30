#!/usr/bin/env python3
"""Converts the title/end illustrations into MSX1 SCREEN 2 (TMS9918) data.

Usage: python3 tools/make_screens.py            (run from the kapiroto folder)
Reads  assets/title_src.webp, assets/end_src.webp
Writes screens.h (RLE-compressed pattern + color tables) and assets/*_msx.png (preview).

Pipeline: nearest MSX palette color per source pixel -> mode filter down to 256x192 ->
text overlay (game font) -> max. 2 colors per 8x1 pixel run (hardware rule) -> tables.
"""
import numpy as np
from PIL import Image

PAL = [(0,0,0),(0,0,0),(62,184,73),(116,208,125),(89,85,224),(128,118,241),(185,94,81),(101,219,239),
       (219,101,89),(255,137,125),(204,195,94),(222,208,135),(58,162,65),(183,102,181),(204,204,204),(255,255,255)]
W, H = 256, 192
WHITE, BLACK, LYELLOW = 15, 1, 11

GLYPHS = {
    ' ': [0, 0, 0, 0, 0, 0, 0, 0],
    'A': [56, 68, 68, 124, 68, 68, 68, 0],
    'C': [56, 68, 64, 64, 64, 68, 56, 0],
    'E': [124, 64, 64, 112, 64, 64, 124, 0],
    'I': [56, 16, 16, 16, 16, 16, 56, 0],
    'N': [68, 100, 100, 84, 76, 76, 68, 0],
    'O': [56, 68, 68, 68, 68, 68, 56, 0],
    'P': [120, 68, 68, 120, 64, 64, 64, 0],
    'R': [120, 68, 68, 120, 68, 68, 68, 0],
    'S': [56, 68, 64, 56, 4, 68, 56, 0],
    'U': [68, 68, 68, 68, 68, 68, 56, 0],
    'W': [68, 68, 68, 68, 84, 84, 40, 0],
    'Y': [68, 68, 40, 16, 16, 16, 16, 0]
}

def quantize(path):
    im = np.asarray(Image.open(path).convert('RGB')).astype(np.int32)
    sh, sw = im.shape[:2]
    pal = np.array(PAL[1:], dtype=np.int32)             # colors 1..15
    d = ((im[:, :, None, :] - pal[None, None, :, :]) ** 2).sum(-1)
    idx = d.argmin(-1) + 1                              # palette index per source pixel
    out = np.zeros((H, W), dtype=np.uint8)
    for y in range(H):
        y0, y1 = y * sh // H, max((y + 1) * sh // H, y * sh // H + 1)
        for x in range(W):
            x0, x1 = x * sw // W, max((x + 1) * sw // W, x * sw // W + 1)
            blk = idx[y0:y1, x0:x1].ravel()
            out[y, x] = np.bincount(blk, minlength=16).argmax()
    return out

def text(img, s, x, y, color, scale=1):
    for ci, ch in enumerate(s):
        for r, b in enumerate(GLYPHS[ch]):
            for c in range(8):
                if b & (0x80 >> c):
                    for dy in range(scale):
                        for dx in range(scale):
                            img[y + r * scale + dy, x + (ci * 8 + c) * scale + dx] = color

def two_colors(img):
    """Hardware rule: at most 2 colors per horizontal run of 8 pixels."""
    for y in range(H):
        for x0 in range(0, W, 8):
            run = img[y, x0:x0 + 8]
            cnt = np.bincount(run, minlength=16)
            if (cnt > 0).sum() <= 2:
                continue
            keep = list(np.argsort(-cnt)[:2])
            for i in range(8):
                if run[i] not in keep:
                    pr = np.array(PAL[run[i]])
                    run[i] = min(keep, key=lambda k: ((np.array(PAL[k]) - pr) ** 2).sum())

def tables(img):
    pat, col = bytearray(), bytearray()
    for ty in range(H // 8):
        for tx in range(W // 8):
            for r in range(8):
                run = img[ty * 8 + r, tx * 8:tx * 8 + 8]
                cs = sorted(set(int(c) for c in run), reverse=True)
                fg = cs[0]; bg = cs[1] if len(cs) > 1 else fg
                p = 0
                for i in range(8):
                    if len(cs) > 1 and run[i] == fg:
                        p |= 0x80 >> i
                pat.append(p); col.append((fg << 4) | bg)
    return bytes(pat), bytes(col)

def rle(data):
    """0x00-0x7F: literal run of n+1 bytes; 0x80-0xFE: repeat next byte (n-0x80+3) times; 0xFF: end."""
    out, i, lit = bytearray(), 0, bytearray()
    def flush():
        nonlocal lit
        while lit:
            n = min(len(lit), 128); out.append(n - 1); out.extend(lit[:n]); lit = lit[n:]
    while i < len(data):
        j = i
        while j < len(data) and data[j] == data[i] and j - i < 129: j += 1
        if j - i >= 3:
            flush(); out.append(0x80 + (j - i - 3)); out.append(data[i]); i = j
        else:
            lit.append(data[i]); i += 1
    flush(); out.append(0xFF)
    return bytes(out)

def carray(name, data):
    rows = [', '.join('0x%02X' % b for b in data[i:i + 16]) for i in range(0, len(data), 16)]
    return 'const u8 %s[%d] = {\n    %s\n};\n' % (name, len(data), ',\n    '.join(rows))

def build(name, src, overlay):
    img = quantize(src)
    overlay(img)
    two_colors(img)
    Image.fromarray(np.array(PAL, dtype=np.uint8)[img]).resize((W * 4, H * 4), Image.NEAREST).save('assets/%s_msx.png' % name)
    pat, col = tables(img)
    p, c = rle(pat), rle(col)
    print('%s: pattern %d -> %d bytes, color %d -> %d bytes' % (name, len(pat), len(p), len(col), len(c)))
    return carray('g_%s_Pat' % name, p) + carray('g_%s_Col' % name, c)

def remap(img, x0, y0, x1, y1, src, dst):
    area = img[y0:y1, x0:x1]
    area[np.isin(area, src)] = dst

def title_overlay(img):
    remap(img, 0, 25, 256, 90, [6, 8], 1)        # logo: drop the red 3D shadow (keeps yellow + black outline)
    text(img, 'PRESS SPACE', 88, 176, BLACK)

def end_overlay(img):
    remap(img, 0, 110, 150, 143, [1], 4)         # drop black outlines of the defeated traps (red/gray on sky only)
    text(img, 'YOU WIN', 72, 24, LYELLOW, 2)
    text(img, 'PRESS SPACE', 88, 176, WHITE)

if __name__ == '__main__':
    out = '// Generated by tools/make_screens.py - do not edit.\n// SCREEN 2 pattern/color tables (6144 bytes each, tile-linear order), RLE-packed.\n#pragma once\n\n'
    out += build('Title', 'assets/title_src.webp', title_overlay)
    out += build('End', 'assets/end_src.webp', end_overlay)
    open('screens.h', 'w').write(out)
