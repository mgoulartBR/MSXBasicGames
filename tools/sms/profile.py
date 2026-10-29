#!/usr/bin/env python3
"""Profile gameplay: PC hit counts and RAM change statistics.

Usage: profile.py ROM.sms OUT.json [frames] [seed] [from_frame]
"""
import sys, os, json, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sms import Sms


def main(rom, out, frames, seed, start):
    s = Sms(open(rom, 'rb').read(), seed)
    hits = collections.Counter()
    changed = collections.Counter()
    vmin = [255] * 0x2000; vmax = [0] * 0x2000
    prev = s.ram()
    writers = {}
    for f in range(frames):
        rec = f >= start
        s.run_frame((lambda pc: hits.update([(s.bank_of(pc), pc)])) if rec else None)
        cur = s.ram()
        if rec:
            for i in range(0x2000):
                if cur[i] != prev[i]: changed[i] += 1
                if cur[i] < vmin[i]: vmin[i] = cur[i]
                if cur[i] > vmax[i]: vmax[i] = cur[i]
        prev = cur
    json.dump({'hits': [[b, p, n] for (b, p), n in hits.most_common()],
               'changed': changed.most_common(),
               'vmin': vmin, 'vmax': vmax, 'final_ram': prev.hex()}, open(out, 'w'))


if __name__ == '__main__':
    a = sys.argv
    main(a[1], a[2], int(a[3]), int(a[4]), int(a[5]))
