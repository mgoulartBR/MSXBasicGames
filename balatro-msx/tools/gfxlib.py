"""Shared helpers for the Balatro-MSX asset pipeline (Pillow + NumPy)."""
import numpy as np
from PIL import Image

CELL_W, CELL_H = 71, 95          # source card cell (1x textures)
OUT_W, OUT_H = 24, 32            # MSX card cell

def hex3(h):
    """'#rrggbb' -> 3-bit-per-channel tuple (MSX2 V9938 palette)."""
    h = h.lstrip('#')
    return tuple(int(round(int(h[i:i+2], 16) * 7 / 255)) for i in (0, 2, 4))

def to8(c):
    return tuple(int(round(v * 255 / 7)) for v in c)

class Palette:
    def __init__(self, anchors):
        self.colors = [hex3(a) for a in anchors]            # 16 x (r,g,b) 0..7
        self.rgb8 = np.array([to8(c) for c in self.colors], dtype=np.float32)

    def finish(self, learned):
        for c in learned:
            self.colors.append(tuple(int(v) for v in c))
        assert len(self.colors) == 16, len(self.colors)
        self.rgb8 = np.array([to8(c) for c in self.colors], dtype=np.float32)

    def nearest(self, rgb, skip0=True):
        """rgb: (...,3) float array 0..255 -> palette index array."""
        w = np.array([2.0, 4.0, 3.0], dtype=np.float32)
        d = (((rgb[..., None, :] - self.rgb8) ** 2) * w).sum(-1)
        if skip0:
            d[..., 0] = 1e18
        return d.argmin(-1)

    def quantize(self, img, dither=False, alpha_cut=128):
        """RGBA PIL image -> uint8 index array (0 = transparent)."""
        a = np.asarray(img.convert('RGBA'), dtype=np.float32)
        rgb, al = a[..., :3].copy(), a[..., 3]
        h, w = al.shape
        out = np.zeros((h, w), dtype=np.uint8)
        if not dither:
            out[:] = self.nearest(rgb)
        else:
            buf = rgb.copy()
            for y in range(h):
                for x in range(w):
                    old = buf[y, x]
                    i = int(self.nearest(old[None, :])[0])
                    out[y, x] = i
                    err = old - self.rgb8[i]
                    for dx, dy, f in ((1, 0, 7/16), (-1, 1, 3/16), (0, 1, 5/16), (1, 1, 1/16)):
                        xx, yy = x + dx, y + dy
                        if 0 <= xx < w and 0 <= yy < h:
                            buf[yy, xx] += err * f * 0.6
        out[al < alpha_cut] = 0
        return out

def premult_resize(img, size):
    """Alpha-aware box/area downscale (avoids dark fringes)."""
    im = img.convert('RGBA')
    a = np.asarray(im, dtype=np.float32)
    al = a[..., 3:4] / 255.0
    pm = np.concatenate([a[..., :3] * al, a[..., 3:4]], axis=-1)
    t = Image.fromarray(pm.clip(0, 255).astype(np.uint8), 'RGBA')
    t = t.resize(size, Image.BOX if min(size) * 2 < min(im.size) else Image.LANCZOS)
    b = np.asarray(t, dtype=np.float32)
    aa = b[..., 3:4] / 255.0
    rgb = np.where(aa > 0, b[..., :3] / np.maximum(aa, 1e-6), 0)
    out = np.concatenate([rgb, b[..., 3:4]], axis=-1).clip(0, 255).astype(np.uint8)
    return Image.fromarray(out, 'RGBA')

def cell(sheet, cx, cy, w=CELL_W, h=CELL_H):
    return sheet.crop((cx * w, cy * h, (cx + 1) * w, (cy + 1) * h))

def pack4(idx):
    """index array (h,w) -> bytes, two pixels per byte, high nibble first (Screen 5)."""
    h, w = idx.shape
    assert w % 2 == 0
    hi, lo = idx[:, 0::2].astype(np.uint8), idx[:, 1::2].astype(np.uint8)
    return ((hi << 4) | lo).tobytes()
