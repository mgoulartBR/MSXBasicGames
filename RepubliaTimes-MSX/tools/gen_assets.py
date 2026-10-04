#!/usr/bin/env python3
"""Asset pipeline: original Flash/Flixel assets -> MSX (Screen 2) data.

Inputs  : original/src/*.as  (news items = source of truth) and original/src/assets/*
Outputs : src/data/*.c, src/data/*.h, build/preview/*.png

* Images are converted to 1bpp "ink masks" (1 = black ink) in TILE-MAJOR order:
  for every tile-row, for every tile, 8 bytes (one per pixel row).  That is exactly the
  byte order of the Screen 2 pattern table, so a whole tile-row is one VRAM write.
* Fonts: the original pixel TTFs are rasterised to 8-row proportional glyphs.
"""
import os, re, sys
from PIL import Image, ImageDraw, ImageFont
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ORIG = os.path.join(ROOT, "original", "src")
ASSETS = os.path.join(ORIG, "assets")
OUT = os.path.join(ROOT, "src", "data")
PREV = os.path.join(ROOT, "build", "preview")
os.makedirs(OUT, exist_ok=True)
os.makedirs(PREV, exist_ok=True)


# ----------------------------------------------------------------------------
# helpers
# ----------------------------------------------------------------------------
def c_bytes(name, data, comment=""):
    lines = []
    for i in range(0, len(data), 16):
        lines.append("\t" + ",".join("0x%02X" % b for b in data[i:i + 16]) + ",")
    return "// %s (%d bytes)\nconst unsigned char %s[] = {\n%s\n};\n" % (comment, len(data), name, "\n".join(lines))


def load_ink(fname):
    """1 = dark opaque pixel."""
    im = Image.open(os.path.join(ASSETS, fname)).convert("RGBA")
    a = np.array(im)
    lum = (a[:, :, 0].astype(int) + a[:, :, 1] + a[:, :, 2]) // 3
    ink = (a[:, :, 3] > 127) & (lum < 128)
    return ink.astype(np.float32)


def shrink(mask, nw, nh, thr=0.42):
    """Area-average resample then threshold (keeps thin pixel-art lines)."""
    h, w = mask.shape
    im = Image.fromarray((mask * 255).astype(np.uint8)).resize((nw, nh), Image.BOX)
    return (np.array(im) / 255.0 >= thr).astype(np.uint8)


def crop_bbox(mask):
    ys, xs = np.where(mask > 0)
    return mask[ys.min():ys.max() + 1, xs.min():xs.max() + 1]


def to_tiles(mask, pad_w=None, pad_h=None):
    h, w = mask.shape
    W = ((w + 7) // 8) * 8 if pad_w is None else pad_w
    H = ((h + 7) // 8) * 8 if pad_h is None else pad_h
    m = np.zeros((H, W), dtype=np.uint8)
    m[:h, :w] = mask
    out = bytearray()
    for ty in range(H // 8):
        for tx in range(W // 8):
            for y in range(8):
                row = m[ty * 8 + y, tx * 8:tx * 8 + 8]
                v = 0
                for b in range(8):
                    v = (v << 1) | int(row[b])
                out.append(v)
    return bytes(out), W // 8, H // 8


def save_preview(name, mask, scale=3):
    im = Image.fromarray(((1 - mask) * 255).astype(np.uint8)).resize(
        (mask.shape[1] * scale, mask.shape[0] * scale), Image.NEAREST)
    im.save(os.path.join(PREV, name + ".png"))


# ----------------------------------------------------------------------------
# fonts
# ----------------------------------------------------------------------------
def make_font(ttf, size, top_row, name):
    """8-row proportional font, chars 32..126. Record = [width, row0..row7]."""
    font = ImageFont.truetype(os.path.join(ASSETS, ttf), size)
    data = bytearray()
    sheet = Image.new("L", (96 * 8, 10), 255)
    for code in range(32, 127):
        ch = chr(code)
        adv = int(round(font.getlength(ch)))
        if code == 32:
            adv = 3
        adv = max(1, min(8, adv))
        im = Image.new("1", (16, 20), 0)
        d = ImageDraw.Draw(im)
        d.fontmode = "1"
        d.text((2, 2), ch, font=font, fill=1)
        px = np.array(im, dtype=np.uint8)
        data.append(adv)
        for r in range(8):
            row = px[top_row + r, 2:2 + 8]
            v = 0
            for b in range(8):
                bit = int(row[b]) if b < adv else 0
                v = (v << 1) | bit
            data.append(v)
    # preview
    return bytes(data)


def font_preview(name, data):
    im = Image.new("L", (600, 10 * 6), 255)
    d = im.load()
    s = "The Republia Times: Rebels Routed At Factory! quick brown fox 0123456789 gypq"
    for line in range(1):
        x = 2
        for ch in s:
            g = data[(ord(ch) - 32) * 9:(ord(ch) - 32) * 9 + 9]
            for r in range(8):
                for b in range(g[0]):
                    if g[1 + r] & (0x80 >> b):
                        d[x + b, 1 + r] = 0
            x += g[0]
    im = im.crop((0, 0, x + 4, 10)).resize(((x + 4) * 4, 40), Image.NEAREST)
    im.save(os.path.join(PREV, "font_%s.png" % name))


# ----------------------------------------------------------------------------
# news items (parsed from the original ActionScript)
# ----------------------------------------------------------------------------
def parse_news():
    src = open(os.path.join(ORIG, "NewsItem.as"), encoding="utf-8").read()
    body = src[src.index("allNewsItems:Array = new Array("):src.index("public function NewsItem(")]
    items = []
    rx = re.compile(
        r'new NewsItem\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(kLoyalty\w+)\s*,\s*(kInteresting|kUninteresting)\s*,'
        r'\s*"((?:[^"\\]|\\.)*)"\s*,\s*(null|"(?:[^"\\]|\\.)*")\s*\)')
    for m in rx.finditer(body):
        ds, de, loy, inter, blurb, art = m.groups()
        loy = {"kLoyaltyUp": 1, "kLoyaltyDown": -1, "kLoyaltyNone": 0}[loy]
        art = None if art == "null" else art[1:-1]
        items.append((int(ds), int(de), loy, 1 if inter == "kInteresting" else 0, blurb, art))
    return items


def c_str(s):
    # s is already AS3-escaped (\" ), valid in C as well
    return '"' + s + '"'


def gen_news():
    items = parse_news()
    expected = open(os.path.join(ORIG, 'NewsItem.as'), encoding='utf-8').read().count('new NewsItem(')
    assert len(items) == expected, (len(items), expected)
    lines = ['// GENERATED by tools/gen_assets.py from original/src/NewsItem.as - do not edit',
             '#include "../game_data.h"', "",
             "const NewsDef g_News[NEWS_COUNT] = { // NEWS_COUNT == %d" % len(items)]
    for i, (ds, de, loy, inter, blurb, art) in enumerate(items):
        lines.append("\t{ %d, %d, %d, %d, %s, %s }, // %d" % (
            ds, de, loy, inter, c_str(blurb), "0" if art is None else c_str(art), i))
    lines.append("};")
    open(os.path.join(OUT, "news_data.c"), "w").write("\n".join(lines) + "\n")
    return len(items)


# ----------------------------------------------------------------------------
# graphics
# ----------------------------------------------------------------------------
def gen_gfx():
    out = ['// GENERATED by tools/gen_assets.py - do not edit', '#include "../game_data.h"', ""]
    hdr = []
    imgs = []

    def add(name, mask, **kw):
        data, wt, ht = to_tiles(mask, **kw)
        out.append(c_bytes("g_Gfx_" + name, data, "%s %dx%d tiles" % (name, wt, ht)))
        hdr.append("extern const unsigned char g_Gfx_%s[];" % name)
        hdr.append("#define GFX_%s_W %d" % (name.upper(), wt))
        hdr.append("#define GFX_%s_H %d" % (name.upper(), ht))
        save_preview(name, np.pad(mask, 0), 2)

    # Title / morning logo (native 250x41, fits 256 px wide)
    add("logo", load_ink("Logo.png").astype(np.uint8))
    add("logo2", load_ink("Logo2.png").astype(np.uint8))
    # Paper masthead: 140x23 -> 128x21
    add("logo_small", shrink(load_ink("LogoSmall.png"), 128, 21, 0.40))
    add("logo_small2", shrink(load_ink("LogoSmall2.png"), 128, 21, 0.40))
    # Ministry of Media building (left copy on Morning.png)
    mm = load_ink("Morning.png")
    left = crop_bbox(mm[:, :270])
    add("ministry", left.astype(np.uint8))   # native size (title screen)
    # Printed paper presses band 335x79 -> 256x60
    add("presses", shrink(load_ink("PrintedPaper.png"), 248, 59, 0.45))
    # Stat meter dome 52x52 -> 40x40
    add("meter", shrink(load_ink("StatMeter.png"), 40, 40, 0.45), pad_w=40, pad_h=40)
    # Mute / cursor are not used on MSX (no mouse UI); skipped on purpose.
    open(os.path.join(OUT, "gfx_data.c"), "w").write("\n".join(out))
    return hdr


def main():
    n = gen_news()
    hdr = gen_gfx()
    sw = make_font("SILKWONDER.ttf", 8, 4, "silk")   # cap-height 7, 8 rows (descender clipped to 1 row)
    font_preview("silk", sw)
    # Mouse pointer: the original hand cursor (20x20) cropped to 16x16, hotspot = finger tip (7,0).
    # Two stacked 16x16 hardware sprites: white fill in front, black outline behind.
    cur = Image.open(os.path.join(ASSETS, "Cursor.png")).convert("RGBA")
    a = np.array(cur)[0:16, 1:17]
    opaque = (a[:, :, 3] > 0)
    dark = opaque & ((a[:, :, 0].astype(int) + a[:, :, 1] + a[:, :, 2]) < 300)
    fill = opaque & ~dark
    def spr(mask):
        out = bytearray()
        for (x0, y0) in ((0, 0), (0, 8), (8, 0), (8, 8)):   # 16x16 sprite = 4 quadrants
            for y in range(8):
                v = 0
                for b in range(8):
                    v = (v << 1) | int(mask[y0 + y, x0 + b])
                out.append(v)
        return bytes(out)
    cursor = spr(fill) + spr(opaque)
    fo = ['// GENERATED by tools/gen_assets.py - do not edit', '#include "../game_data.h"', "",
          "// Font record: [advance, row0..row7], chars 32..126 (9 bytes each)",
          c_bytes("g_Font_Silk", sw, "SILKWONDER 8px"),
          c_bytes("g_Sprite_Cursor", cursor, "pointer: fill sprite (white) + outline sprite (black), 16x16 each")]
    open(os.path.join(OUT, "font_data.c"), "w").write("\n".join(fo))
    open(os.path.join(OUT, "gfx_data.h"), "w").write(
        "#pragma once\n// GENERATED by tools/gen_assets.py\n" + "\n".join(hdr) + "\n")
    print("news items:", n)


if __name__ == "__main__":
    main()
