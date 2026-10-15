#!/usr/bin/env python3
"""Verify the Side Pocket (MD) findings in docs/spec.md against your own ROM dump.
Usage: rom_probe.py <rom.md>
"""
import math, struct, sys

rom = open(sys.argv[1], 'rb').read()
print("header:", rom[0x120:0x150].decode('latin1').strip())
print("entry : $%X" % struct.unpack('>I', rom[4:8])[0])
print("vint  : $%X" % struct.unpack('>I', rom[0x78:0x7C])[0])
tab = rom[0x7B1C:0x7B1C + 64]
ref = bytes(min(255, round(255 * math.sin(math.pi / 2 * i / 64))) for i in range(64))
diff = [i for i in range(64) if abs(tab[i] - ref[i]) > 1]
print("sine table $7B1C matches 255*sin(pi/2*i/64):", "yes" if not diff else "no at %s" % diff)
