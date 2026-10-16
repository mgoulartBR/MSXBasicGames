#!/usr/bin/env python3
"""Pack every <gen>/*.asset into 8 KB ROM segments for the ASCII8 mapper (window 0xA000).

    make_blob.py <gen-dir>

Writes <gen>/blob.c (one `__at(0xSSA000)` array per segment) and <gen>/blob.h (segment/offset of each asset).
Segments 0-2 hold the code (24 KB, fixed); the blob starts at segment FIRST_SEG.
"""
import os, sys
FIRST_SEG = 3
SEG = 8192
gen = sys.argv[1]
names = sorted(f[:-6] for f in os.listdir(gen) if f.endswith('.asset'))
blob = bytearray(); table = []
for n in names:
    d = open(os.path.join(gen, n + '.asset'), 'rb').read()
    table.append((n.upper(), len(blob), len(d))); blob += d
nseg = (len(blob) + SEG - 1) // SEG
blob += bytes(nseg * SEG - len(blob))
with open(os.path.join(gen, 'blob.c'), 'w') as f:
    f.write('// GENERATED from the original game graphics - do not commit\n#include "blob.h"\n')
    for s in range(nseg):
        chunk = blob[s * SEG:(s + 1) * SEG]
        f.write('__at(0x%02XA000) const unsigned char g_BlobSeg%d[%d] = {\n' % (FIRST_SEG + s, s, SEG))
        for i in range(0, SEG, 32): f.write('\t' + ','.join('0x%02x' % b for b in chunk[i:i + 32]) + ',\n')
        f.write('};\n')
with open(os.path.join(gen, 'blob.h'), 'w') as f:
    f.write('#pragma once\n#define BLOB_FIRST_SEG %d\n#define BLOB_SEGMENTS %d\n' % (FIRST_SEG, nseg))
    for n, off, ln in table:
        f.write('#define BLOB_%s_SEG %d\n#define BLOB_%s_OFF 0x%04X\n#define BLOB_%s_LEN %d\n' % (n, off // SEG, n, off % SEG, n, ln))
        hdr = blob[off:off + 5]
        if n not in ('BALLS', 'PALETTE'):                      # image containers: [w][h][flags][xb][y]
            f.write('#define IMG_%s_W %d\n#define IMG_%s_H %d\n#define IMG_%s_XB %d\n#define IMG_%s_Y %d\n' % (n, hdr[0], n, hdr[1], n, hdr[3], n, hdr[4]))
    lp = os.path.join(gen, 'layout.txt')
    if os.path.exists(lp):
        for line in open(lp):
            k, v = line.split(); f.write('#define LAY_%s %s\n' % (k, v))
    cp = os.path.join(gen, 'colors.txt')
    if os.path.exists(cp):
        for line in open(cp):
            nm, v = line.split(); f.write('#define COL_%s %s\n' % (nm, v))
    fp = os.path.join(gen, 'flat.txt')
    if os.path.exists(fp):
        x0, x1, y0, y1, _ = [int(v) for v in open(fp).read().split()]
        f.write('#define FLAT_X0 %d\n#define FLAT_X1 %d\n#define FLAT_Y0 %d\n#define FLAT_Y1 %d\n' % (x0, x1, y0, y1))
print('blob: %d assets, %d bytes, %d segments (%d..%d)' % (len(table), len(blob), nseg, FIRST_SEG, FIRST_SEG + nseg - 1))
