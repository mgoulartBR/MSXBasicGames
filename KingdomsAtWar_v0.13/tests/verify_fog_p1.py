#!/usr/bin/env python3
"""P1 fog check on the dumps written by fog011.tcl: every cell's layer B name must match the fog state
(unknown, remembered, visible) and the remembered patterns must really carry the dither."""
from pathlib import Path
import sys
d=Path(__file__).resolve().parent
PB_PLAIN,PB_DITH=8,776
vram=(d/'fog011-V9990P1-remembered-vram.bin').read_bytes()
seen=(d/'fog011-V9990P1-remembered-seen.bin').read_bytes()   # explored flag, 1 byte per cell (32x32 window of the map)
src=[tuple(map(int,l.split())) for l in (d/'fog011-V9990P1-remembered-sources.txt').read_text().splitlines() if l.strip()]
win=tuple(map(int,(d/'fog011-V9990P1-remembered-window.txt').read_text().split()))  # rendered rectangle x0 y0 x1 y1
def state(x,y):
    for sx,sy,r in src:
        dx,dy=abs(x-sx),abs(y-sy)
        if dx<=r and dy<=r and dx+dy<=r+(r>>1):return 2
    return 1 if seen[y*32+x] else 0
def pattern_row(n,k):
    a=0x40000+(n>>5)*1024+k*128+(n&31)*4
    return vram[a:a+4]
bad=0
for y in range(win[1],win[3]):
    for x in range(win[0],win[2]):
        a=0x7E000+(y&31)*256+(x&31)*4
        n=vram[a]+256*vram[a+1]
        st=state(x,y)
        kind=0 if n==1 else (1 if n>=PB_DITH else 2)
        if kind!=st:bad+=1;continue
        if st==1 and not ((pattern_row(n,0)[0]>>4)==1 and (pattern_row(n,1)[0]&15)==1):bad+=1
print('fog P1 names/patterns: mismatches=%d'%bad)
sys.exit(1 if bad else 0)
