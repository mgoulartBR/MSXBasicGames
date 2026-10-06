#!/usr/bin/env python3
"""Memory report from the SDLD map file: code/data areas, per-module code size, ROM segment usage."""
import re, sys, collections

mp = open(sys.argv[1]).read().splitlines()
rom = sys.argv[2] if len(sys.argv) > 2 else None
areas = {}
for l in mp:
    m = re.match(r'^(_[A-Z0-9_]+)\s+([0-9A-F]{8})\s+([0-9A-F]{8}) =', l)
    if m: areas[m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
# per-module code size: take the _CODE symbol list (first occurrence)
syms = []; incode = False
for l in mp:
    if l.startswith('_CODE ') and 'bytes' in l: incode = True; continue
    if incode:
        m = re.match(r'^\s+([0-9A-F]{8})\s+(\S+)\s+(\S+)?\s*$', l)
        if m: syms.append((int(m.group(1), 16), m.group(2), m.group(3) or '?'))
        elif l.startswith('ASxxxx') and syms: break
syms.sort()
mod = collections.Counter()
code = areas.get('_CODE', (0, 0)); end = code[0] + code[1]
for i, (a, n, m) in enumerate(syms):
    nxt = syms[i + 1][0] if i + 1 < len(syms) else end
    if n.startswith('s__') or n.startswith('l__'): continue
    mod[m] += max(0, nxt - a)
print("== memory report ==")
for k in ('_CODE', '_HOME', '_DATA', '_INITIALIZED', '_INITIALIZER'):
    if k in areas: print("%-14s @%04X  %6d bytes" % (k, areas[k][0], areas[k][1]))
fixed = code[1] + areas.get('_HOME', (0, 0))[1] + areas.get('_INITIALIZER', (0, 0))[1]
print("fixed code+rodata: %d bytes of the 24576 available in banks 0-2 (%.0f%%)" % (fixed, fixed * 100.0 / 24576))
ram = areas.get('_DATA', (0, 0))[1] + areas.get('_INITIALIZED', (0, 0))[1]
print("RAM (page 3 from C000h): %d bytes static (stack grows down from F380h)" % ram)
print("top modules by code size:")
for m, s in mod.most_common(8): print("   %-12s %6d" % (m, s))
segs = sorted((k, v) for k, v in areas.items() if k.startswith('_SEG'))
print("mapper segments: %d" % len(segs))
for k, v in segs:
    print("   %-7s @%08X %5d bytes (%d%% of 8 KB)" % (k, v[0], v[1], v[1] * 100 // 8192))
if rom:
    import os; print("ROM file: %d KB" % (os.path.getsize(rom) // 1024))
