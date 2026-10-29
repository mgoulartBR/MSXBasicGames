#!/usr/bin/env python3
"""Run a Sega Master System ROM headless and record every executed (bank, pc).

Usage: trace.py ROM.sms OUT.json [frames] [seed] [snapshot_every_n_frames]

Minimal SMS model: Sega mapper, fake VDP (VBlank flag + data port swallow), joypad
inputs driven by a pseudo-random "button masher" so gameplay code paths get hit.
The result feeds `disasm.py --trace` to seed the recursive disassembly with real
bank/address pairs for the code whose slot-2 bank is chosen by data at run time.
"""
import sys, json, random
from z80 import Z80Machine

BANK = 0x4000


def main(rom_path, out_path, frames=3000, seed=1, snap=0):
    rom = open(rom_path, 'rb').read()
    nbanks = len(rom) // BANK
    rnd = random.Random(seed)
    m = Z80Machine()
    regs = [0x80, 0, 1, 2]          # FFFC..FFFF
    slot = [0, 1, 2]

    def bank_bytes(b): return rom[(b % nbanks) * BANK:(b % nbanks + 1) * BANK]

    def load():
        m.set_memory_block(0x0000, bank_bytes(slot[0]))
        m.set_memory_block(0x4000, bank_bytes(slot[1]))
        m.set_memory_block(0x8000, bank_bytes(slot[2]))
        # first 1 KB of slot 0 is always bank 0
        m.set_memory_block(0x0000, rom[:0x400])
    load()

    def on_write(addr, n):
        i = addr - 0xFFFC
        regs[i] = n
        if i == 1: slot[0] = n & 0x3F
        elif i == 2: slot[1] = n & 0x3F
        elif i == 3: slot[2] = n & 0x3F
        if i > 0: load()
        m.memory[addr] = n
        m.memory[addr - 0x2000] = n  # RAM mirror
    m.set_write_callback(on_write)
    m.mark_addrs(0xFFFC, 4, m.WRITE_MARK)

    st = {'vbl': False, 'pad': 0xFF, 'frame': 0}
    vram = bytearray(0x4000)
    cram = bytearray(32)
    vdp = {'addr': 0, 'code': 0, 'latch': None, 'regs': [0] * 16, 'buf': 0}

    def on_in(port):
        p = port & 0xFF
        if p == 0xBE:
            v = vdp['buf']; vdp['buf'] = vram[vdp['addr'] & 0x3FFF]; vdp['addr'] = (vdp['addr'] + 1) & 0x3FFF
            vdp['latch'] = None
            return v
        if p == 0xBF:
            vdp['latch'] = None
            v = 0x80 if st['vbl'] else 0
            st['vbl'] = False
            return v | 0x1F
        if p == 0x7E: return 0x40
        if p == 0xDC or p == 0xC0: return st['pad']
        if p == 0xDD or p == 0xC1: return 0xFF
        return 0xFF
    m.set_input_callback(on_in)
    def on_out(port, v):
        p = port & 0xFF
        if p == 0xBE:
            vdp['latch'] = None
            if vdp['code'] == 3: cram[vdp['addr'] & 31] = v
            else: vram[vdp['addr'] & 0x3FFF] = v
            vdp['buf'] = v
            vdp['addr'] = (vdp['addr'] + 1) & 0x3FFF
        elif p == 0xBF:
            if vdp['latch'] is None: vdp['latch'] = v
            else:
                lo, hi = vdp['latch'], v
                vdp['latch'] = None
                vdp['code'] = hi >> 6
                vdp['addr'] = ((hi & 0x3F) << 8) | lo
                if vdp['code'] == 2: vdp['regs'][hi & 0x0F] = lo
                elif vdp['code'] == 0:
                    vdp['buf'] = vram[vdp['addr']]; vdp['addr'] = (vdp['addr'] + 1) & 0x3FFF
    m.set_output_callback(on_out)

    seen = set()
    EV_FRAME = 1 << 3
    budget_per_frame = 1
    for frame in range(frames):
        st['vbl'] = True
        st['frame'] = frame
        # joypad: change state every ~20 frames; bit=0 means pressed. Press START periodically.
        if frame % 20 == 0:
            st['pad'] = rnd.getrandbits(8) | 0x00
            st['pad'] = 0xFF ^ (1 << rnd.randrange(6)) ^ (1 << rnd.randrange(6)) if rnd.random() < 0.9 else 0xFF
        m.on_handle_active_int()
        m.ticks_to_stop = 1
        steps = 0
        # run instruction by instruction for ~ one frame worth of CPU time (59736 T-states)
        t = 0
        while steps < 6000:
            pc = m.pc
            b = slot[2] if pc >= 0x8000 and pc < 0xC000 else (slot[1] if pc >= 0x4000 and pc < 0x8000 else (0 if pc < 0x400 else slot[0]))
            if pc < 0xC000:
                seen.add((b, pc))
            m.ticks_to_stop = 1
            m.run()
            steps += 1
            if m.halted:
                break
        if snap and frame % snap == 0 and frame:
            b = out_path.rsplit('.', 1)[0] + f'_f{frame:05d}'
            open(b + '.vram', 'wb').write(vram); open(b + '.cram', 'wb').write(cram)
            json.dump(vdp['regs'], open(b + '.vdpregs.json', 'w'))
        if frame % 300 == 0:
            print(f"frame {frame}: pc={m.pc:04x} banks={slot} seen={len(seen)}", flush=True)
    json.dump(sorted(seen), open(out_path, 'w'))
    base = out_path.rsplit('.', 1)[0]
    open(base + '.vram', 'wb').write(vram)
    open(base + '.cram', 'wb').write(cram)
    json.dump(vdp['regs'], open(base + '.vdpregs.json', 'w'))
    print("unique (bank,pc):", len(seen))


if __name__ == '__main__':
    a = sys.argv
    main(a[1], a[2], int(a[3]) if len(a) > 3 else 3000, int(a[4]) if len(a) > 4 else 1, int(a[5]) if len(a) > 5 else 0)
