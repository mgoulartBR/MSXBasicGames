#!/usr/bin/env python3
"""Turn the extracted binaries into C arrays for the MSXGL project.  usage: gen_c_assets.py <gen-dir>"""
import sys, os
g = sys.argv[1]
def rle(d):
    out = bytearray(); i = 0
    while i < len(d):
        j = i
        while j + 1 < len(d) and d[j + 1] == d[i] and j - i < 127: j += 1
        run = j - i + 1
        if run >= 3: out += bytes([0x80 | (run - 1), d[i]]); i += run
        else:
            k = i; lit = bytearray()
            while k < len(d) and len(lit) < 128:
                r = 1
                while k + r < len(d) and d[k + r] == d[k] and r < 3: r += 1
                if r >= 3: break
                lit.append(d[k]); k += 1
            out += bytes([len(lit) - 1]) + lit; i = k
    return bytes(out)
def arr(name, d):
    s = 'const unsigned char %s[%d] = {\n' % (name, len(d))
    for i in range(0, len(d), 24): s += '\t' + ','.join('0x%02x' % b for b in d[i:i + 24]) + ',\n'
    return s + '};\n\n'
t = open(os.path.join(g, 'table.bin'), 'rb').read(); b = open(os.path.join(g, 'balls.bin'), 'rb').read(); p = open(os.path.join(g, 'palette.bin'), 'rb').read()
r = rle(t)
open(os.path.join(g, 'assets.c'), 'w').write('// GENERATED from the original game graphics - do not commit\n' + arr('g_TableRLE', r) + arr('g_BallStrip', b) + arr('g_Palette', p))
open(os.path.join(g, 'assets.h'), 'w').write('#pragma once\nextern const unsigned char g_TableRLE[%d];\nextern const unsigned char g_BallStrip[%d];\nextern const unsigned char g_Palette[%d];\n#define TABLE_RLE_SIZE %d\n' % (len(r), len(b), len(p), len(r)))
fl = [int(v) for v in open(os.path.join(g, 'flat.txt')).read().split()]
open(os.path.join(g, 'assets.h'), 'a').write('#define FLAT_X0 %d\n#define FLAT_X1 %d\n#define FLAT_Y0 %d\n#define FLAT_Y1 %d\n' % tuple(fl[:4]))
print('RLE', len(t), '->', len(r), 'flat', fl)
