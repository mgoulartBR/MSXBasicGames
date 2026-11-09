#!/usr/bin/env python3
"""P1 fog check on the dumps written by fog011.tcl: every cell's layer B name must match the fog state
(unknown, remembered, visible) and the remembered patterns must really carry the dither."""
from pathlib import Path
import sys
d=Path(__file__).resolve().parent
PB_PLAIN,PB_DITH=8,800
vram=(d/'fog011-V9990P1-remembered-vram.bin').read_bytes()
seen=(d/'fog011-V9990P1-remembered-seen.bin').read_bytes()
vis=(d/'fog011-V9990P1-remembered-visible.bin').read_bytes()
def state(x,y):
    c=y*32+x
    return 2 if vis[c>>3]>>(c&7)&1 else 1 if seen[c>>3]>>(c&7)&1 else 0
def pattern_row(n,k):
    a=0x40000+(n>>5)*1024+k*128+(n&31)*4
    return vram[a:a+4]
bad=0
for y in range(32):
    for x in range(32):
        a=0x7E000+y*256+x*4
        n=vram[a]+256*vram[a+1]
        st=state(x,y)
        kind=0 if n==1 else (1 if n>=PB_DITH else 2)
        if kind!=st:bad+=1;continue
        if st==1 and not ((pattern_row(n,0)[0]>>4)==1 and (pattern_row(n,1)[0]&15)==1):bad+=1
print('fog P1 names/patterns: mismatches=%d'%bad)
sys.exit(1 if bad else 0)
