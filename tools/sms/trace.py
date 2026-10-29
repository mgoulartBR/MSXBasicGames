#!/usr/bin/env python3
"""Run a Sega Master System ROM headless and record every executed (bank, pc).

Usage: trace.py ROM.sms OUT.json [frames] [seed] [snapshot_every_n_frames]

Writes OUT.json (sorted [bank, pc] list, input for disasm.py) plus OUT.vram/.cram/
.vdpregs.json at the end (and at each snapshot) for render.py.
"""
import sys, json, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sms import Sms


def dump(s, base):
    open(base + '.vram', 'wb').write(s.vram); open(base + '.cram', 'wb').write(s.cram)
    json.dump(s.vdp['regs'], open(base + '.vdpregs.json', 'w'))


def main(rom_path, out_path, frames=3000, seed=1, snap=0):
    s = Sms(open(rom_path, 'rb').read(), seed)
    seen = set()
    base = out_path.rsplit('.', 1)[0]

    def step(pc):
        if pc < 0xC000: seen.add((s.bank_of(pc), pc))
    for f in range(frames):
        s.run_frame(step)
        if snap and f and f % snap == 0: dump(s, base + f'_f{f:05d}')
        if f % 300 == 0: print(f"frame {f}: pc={s.m.pc:04x} banks={s.slot} seen={len(seen)}", flush=True)
    json.dump(sorted(seen), open(out_path, 'w'))
    dump(s, base)
    print("unique (bank,pc):", len(seen))


if __name__ == '__main__':
    a = sys.argv
    main(a[1], a[2], int(a[3]) if len(a) > 3 else 3000, int(a[4]) if len(a) > 4 else 1, int(a[5]) if len(a) > 5 else 0)
