#!/usr/bin/env python3
"""Render VRAM/CRAM dumps from trace.py into PNGs: tileset sheets and a screenshot.

Usage: render.py BASE OUTDIR     (BASE.vram, BASE.cram, BASE.vdpregs.json)
"""
import sys, json, os
from PIL import Image


def rgb(c):  # SMS CRAM: --BBGGRR
    return tuple(((c >> s) & 3) * 85 for s in (0, 2, 4))


def tile_pixels(vram, n):
    t = vram[n * 32:(n + 1) * 32]
    rows = []
    for y in range(8):
        b0, b1, b2, b3 = t[y*4:y*4+4]
        rows.append([((b0 >> (7-x)) & 1) | (((b1 >> (7-x)) & 1) << 1) | (((b2 >> (7-x)) & 1) << 2) | (((b3 >> (7-x)) & 1) << 3) for x in range(8)])
    return rows


def main(base, out):
    os.makedirs(out, exist_ok=True)
    vram = open(base + '.vram', 'rb').read(); cram = open(base + '.cram', 'rb').read()
    regs = json.load(open(base + '.vdpregs.json'))
    pal = [rgb(c) for c in cram]
    for pi, name in ((0, 'bg'), (1, 'spr')):
        P = pal[pi*16:(pi+1)*16]
        img = Image.new('RGB', (16*8, 32*8))
        for n in range(512):
            tp = tile_pixels(vram, n)
            for y in range(8):
                for x in range(8):
                    img.putpixel(((n % 16)*8 + x, (n // 16)*8 + y), P[tp[y][x]])
        img.resize((img.width*3, img.height*3), Image.NEAREST).save(f"{out}/tiles_{name}.png")
    # screenshot
    nt = ((regs[2] >> 1) & 7) << 11
    img = Image.new('RGB', (256, 224))
    for ty in range(28):
        for tx in range(32):
            e = vram[nt + (ty*32+tx)*2] | (vram[nt + (ty*32+tx)*2 + 1] << 8)
            n = e & 0x1FF; hf = e & 0x200; vf = e & 0x400; pl = 16 if e & 0x800 else 0
            tp = tile_pixels(vram, n)
            for y in range(8):
                for x in range(8):
                    c = tp[y if not vf else 7-y][x if not hf else 7-x]
                    img.putpixel((tx*8+x, ty*8+y), pal[pl + c])
    sat = ((regs[5] >> 1) & 0x3F) << 8
    sbase = 256 if regs[6] & 4 else 0
    tall = regs[1] & 2
    for i in range(64):
        y = vram[sat + i]
        if y == 0xD0: break
        x = vram[sat + 128 + i*2]; n = vram[sat + 128 + i*2 + 1]
        for h in range(2 if tall else 1):
            tn = sbase + (n & (0xFE if tall else 0xFF)) + h
            tp = tile_pixels(vram, tn)
            for yy in range(8):
                for xx in range(8):
                    c = tp[yy][xx]
                    px, py = x + xx, y + 1 + h*8 + yy
                    if c and 0 <= px < 256 and 0 <= py < 224: img.putpixel((px, py), pal[16 + c])
    img.resize((768, 672), Image.NEAREST).save(f"{out}/screen.png")


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
