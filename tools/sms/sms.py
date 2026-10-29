"""Minimal headless Sega Master System model (Z80 + Sega mapper + fake VDP + joypad)."""
import random
from z80 import Z80Machine

BANK = 0x4000


class Sms:
    def __init__(self, rom, seed=1):
        self.rom = rom
        self.nbanks = len(rom) // BANK
        self.rnd = random.Random(seed)
        self.m = m = Z80Machine()
        self.slot = [0, 1, 2]
        self.vram = bytearray(0x4000)
        self.cram = bytearray(32)
        self.vdp = {'addr': 0, 'code': 0, 'latch': None, 'regs': [0] * 16, 'buf': 0}
        self.vbl = False
        self.pad = 0xFF
        self.pad_fn = None            # optional: callable(frame) -> pad byte
        self.frame = 0
        self._load()
        m.set_write_callback(self._on_write)
        m.mark_addrs(0xFFFC, 4, m.WRITE_MARK)
        m.set_input_callback(self._on_in)
        m.set_output_callback(self._on_out)

    def bank_bytes(self, b):
        b %= self.nbanks
        return self.rom[b * BANK:(b + 1) * BANK]

    def _load(self):
        m, s = self.m, self.slot
        m.set_memory_block(0x0000, self.bank_bytes(s[0]))
        m.set_memory_block(0x4000, self.bank_bytes(s[1]))
        m.set_memory_block(0x8000, self.bank_bytes(s[2]))
        m.set_memory_block(0x0000, self.rom[:0x400])

    def _on_write(self, addr, n):
        i = addr - 0xFFFC
        if i == 1: self.slot[0] = n & 0x3F
        elif i == 2: self.slot[1] = n & 0x3F
        elif i == 3: self.slot[2] = n & 0x3F
        if i > 0: self._load()
        self.m.memory[addr] = n
        self.m.memory[addr - 0x2000] = n

    def _on_in(self, port):
        p, v = port & 0xFF, self.vdp
        if p == 0xBE:
            r = v['buf']; v['buf'] = self.vram[v['addr'] & 0x3FFF]
            v['addr'] = (v['addr'] + 1) & 0x3FFF; v['latch'] = None
            return r
        if p == 0xBF:
            v['latch'] = None
            r = 0x80 if self.vbl else 0
            self.vbl = False
            return r | 0x1F
        if p == 0x7E: return 0x40
        if p in (0xDC, 0xC0): return self.pad
        return 0xFF

    def _on_out(self, port, val):
        p, v = port & 0xFF, self.vdp
        if p == 0xBE:
            v['latch'] = None
            if v['code'] == 3: self.cram[v['addr'] & 31] = val
            else: self.vram[v['addr'] & 0x3FFF] = val
            v['buf'] = val; v['addr'] = (v['addr'] + 1) & 0x3FFF
        elif p == 0xBF:
            if v['latch'] is None: v['latch'] = val
            else:
                lo, v['latch'] = v['latch'], None
                v['code'] = val >> 6
                v['addr'] = ((val & 0x3F) << 8) | lo
                if v['code'] == 2: v['regs'][val & 0x0F] = lo
                elif v['code'] == 0:
                    v['buf'] = self.vram[v['addr']]; v['addr'] = (v['addr'] + 1) & 0x3FFF

    def bank_of(self, pc):
        if pc < 0x400: return 0
        return self.slot[0] if pc < 0x4000 else self.slot[1] if pc < 0x8000 else self.slot[2]

    def mash_pad(self):
        r = self.rnd
        if self.frame % 20 == 0:
            self.pad = 0xFF ^ (1 << r.randrange(6)) ^ (1 << r.randrange(6)) if r.random() < 0.9 else 0xFF

    def run_frame(self, on_step=None, steps=6000):
        """Raise VBlank, fire the interrupt, and execute ~one frame of instructions."""
        m = self.m
        self.vbl = True
        if self.pad_fn: self.pad = self.pad_fn(self.frame)
        else: self.mash_pad()
        m.on_handle_active_int()
        for _ in range(steps):
            if on_step: on_step(m.pc)
            m.ticks_to_stop = 1
            m.run()
            if m.halted: break
        self.frame += 1

    def ram(self):
        return bytes(self.m.memory[0xC000:0xE000])
