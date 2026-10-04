#!/usr/bin/env python3
"""Histogram of PC samples (one hex address per line) by function, using the linker map."""
import re, sys, collections, bisect
syms = []
for l in open(sys.argv[2]):
    m = re.match(r'\s+([0-9A-F]{8})\s+(_\w+)', l)
    if m and int(m.group(1), 16) < 0x10000:
        syms.append((int(m.group(1), 16), m.group(2)))
syms.sort()
addrs = [s[0] for s in syms]
c = collections.Counter()
n = 0
for l in open(sys.argv[1]):
    if not l.strip():
        continue
    a = int(l.strip(), 16)
    i = bisect.bisect_right(addrs, a) - 1
    c[syms[i][1] if i >= 0 else '?'] += 1
    n += 1
print("samples:", n)
for k, v in c.most_common(15):
    print("%5.1f%%  %s" % (100.0 * v / n, k))
