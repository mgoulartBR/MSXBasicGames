#!/usr/bin/env python3
"""Minimal MSX2 simulator for testing 16 KB cartridge ROMs headless.

Models: Z80 (z80 package), BIOS entry points as Python traps (CHGMOD, WRTVDP, GTSTCK, GTTRIG,
SNSMAT, CHPUT) plus the VBlank interrupt, and a V9938 subset: registers, 128 KB VRAM, palette,
Screen 5 bitmap, command engine (HMMV, HMMM, LMMM, LMMV, STOP), mode-2 sprites (16x16, 8 per
line), status registers 0 and 2.

It is *my reading of the V9938 documentation*, not a hardware-accurate emulator: command
timing is a rough estimate and several details are ignored (DIX/DIY, 5th-sprite flag, sprite
collision). Always confirm in openMSX.
"""
import sys
from z80 import Z80Machine

FRAME_T = 59736            # T-states per frame at 60 Hz (3.579545 MHz)
BIOS = {'CHGMOD': 0x005F, 'WRTVDP': 0x0047, 'GTSTCK': 0x00D5, 'GTTRIG': 0x00D8,
        'SNSMAT': 0x0141, 'CHPUT': 0x00A2}
# (matrix row, bit) of the keys the game uses
KEYS = {'W': (5, 4), 'S': (5, 0), 'A': (2, 6), 'D': (3, 1), 'SPACE': (8, 0),
        'LEFT': (8, 4), 'UP': (8, 5), 'DOWN': (8, 6), 'RIGHT': (8, 7)}
# approximate command cost in T-states per byte (HMMV/HMMM) or pixel (LMMM/LMMV)
CMD_COST = {0xC: 2, 0xD: 3, 0x9: 8, 0x8: 8}


class VDP:
    def __init__(self, sim):
        self.sim = sim
        self.vram = bytearray(0x20000)
        self.r = [0] * 64
        self.s0 = 0
        self.latch = None
        self.addr = 0
        self.buf = 0
        self.pal = [(0, 0, 0)] * 16
        self.pal_phase = 0
        self.pal_b0 = 0
        self.busy_until = 0
        self.cmd_log = []           # (time, cmd, nbytes)
        self.status_reads = {0: 0, 2: 0}
        self.violations = []

    # ---- ports ----
    def int_line(self):
        return bool(self.s0 & 0x80) and bool(self.r[1] & 0x20)

    def read_status(self):
        n = self.r[15] & 0x0F
        self.latch = None
        self.status_reads[n] = self.status_reads.get(n, 0) + 1
        if n == 0:
            v = self.s0
            self.s0 &= 0x1F
            return v
        if n == 2:
            ce = 1 if self.sim.cur() < self.busy_until else 0
            return 0x80 | ce | (0x40 if self.sim.in_vblank() else 0)
        return 0

    def write_ctrl(self, v):
        if self.latch is None:
            self.latch = v
            return
        lo, self.latch = self.latch, None
        if v & 0x80:
            self.write_reg(v & 0x3F, lo)
        else:
            self.addr = ((self.r[14] & 7) << 14) | ((v & 0x3F) << 8) | lo
            if not v & 0x40:
                self.buf = self.vram[self.addr]
                self._inc()

    def _inc(self):
        self.addr = (self.addr + 1) & 0x1FFFF
        self.r[14] = (self.addr >> 14) & 7

    def write_data(self, v):
        self.latch = None
        self.vram[self.addr] = v
        self.buf = v
        self._inc()

    def read_data(self):
        self.latch = None
        v = self.buf
        self.buf = self.vram[self.addr]
        self._inc()
        return v

    def write_pal(self, v):
        if self.pal_phase == 0:
            self.pal_b0 = v
            self.pal_phase = 1
        else:
            i = self.r[16] & 15
            self.pal[i] = ((self.pal_b0 >> 4) & 7, v & 7, self.pal_b0 & 7)
            self.r[16] = (i + 1) & 15
            self.pal_phase = 0

    def write_indirect(self, v):
        reg = self.r[17] & 0x3F
        self.write_reg(reg, v)
        if not self.r[17] & 0x80:
            self.r[17] = (self.r[17] & 0x80) | ((reg + 1) & 0x3F)

    def write_reg(self, reg, v):
        if reg > 46: return
        if reg == 7: self.sim.border_log.append((self.sim.cur(), v & 15))
        self.r[reg] = v
        if reg == 14: self.addr = (self.addr & 0x3FFF) | ((v & 7) << 14)
        if reg == 16: self.pal_phase = 0
        if reg == 46: self.start_cmd()

    # ---- command engine (Screen 5 only) ----
    def px(self, x, y):
        b = self.vram[(y * 128 + x // 2) & 0x1FFFF]
        return (b & 15) if x & 1 else (b >> 4)

    def put(self, x, y, c):
        a = (y * 128 + x // 2) & 0x1FFFF
        b = self.vram[a]
        self.vram[a] = (b & 0xF0) | c if x & 1 else (b & 0x0F) | (c << 4)

    def start_cmd(self):
        r = self.r
        cmd = r[46] >> 4
        lo = r[46] & 15
        sx = r[32] | ((r[33] & 1) << 8); sy = r[34] | ((r[35] & 3) << 8)
        dx = r[36] | ((r[37] & 1) << 8); dy = r[38] | ((r[39] & 3) << 8)
        nx = r[40] | ((r[41] & 1) << 8); ny = r[42] | ((r[43] & 3) << 8)
        clr, arg = r[44], r[45]
        if cmd == 0: return
        if arg & 0x0C: self.violations.append(f'DIX/DIY not modelled (cmd {cmd:X})')
        if not self.sim.cur() >= self.busy_until:
            self.violations.append('command started while the previous one was still running')
        if (r[8] & 0x10) == 0 and False: pass
        n = 0
        if cmd in (0xC, 0xD):               # high speed: whole bytes
            if dx & 1 or nx & 1 or (cmd == 0xD and sx & 1):
                self.violations.append(f'odd X/NX in byte command {cmd:X}: sx={sx} dx={dx} nx={nx}')
            for j in range(ny):
                for i in range(nx // 2):
                    a = ((dy + j) * 128 + (dx // 2) + i) & 0x1FFFF
                    if cmd == 0xC: self.vram[a] = clr
                    else: self.vram[a] = self.vram[((sy + j) * 128 + (sx // 2) + i) & 0x1FFFF]
                    n += 1
        elif cmd in (0x9, 0x8):             # logical: pixels
            for j in range(ny):
                for i in range(nx):
                    s = self.px(sx + i, sy + j) if cmd == 0x9 else clr & 15
                    d = self.px(dx + i, dy + j)
                    if lo & 8 and s == 0: continue          # TIMP / TAND... only IMP/TIMP modelled
                    if lo & 7: self.violations.append(f'logical op {lo:X} not modelled')
                    self.put((dx + i) & 255, (dy + j) & 1023, s)
                    n += 1
        else:
            self.violations.append(f'command {cmd:X} not modelled')
        cost = 100 + n * CMD_COST.get(cmd, 4)
        self.busy_until = max(self.busy_until, self.sim.cur()) + cost
        self.cmd_log.append((self.sim.cur(), cmd, n))

    # ---- rendering ----
    def lines(self):
        return 212 if self.r[9] & 0x80 else 192

    def render(self, sprites=True):
        from PIL import Image
        base = ((self.r[2] >> 5) & 3) * 0x8000
        H = self.lines()
        img = Image.new('RGB', (256, H))
        pal = [tuple(v * 255 // 7 for v in c) for c in self.pal]
        back = pal[self.r[7] & 15]
        tp = bool(self.r[8] & 0x20)
        pix = img.load()
        for y in range(H):
            row = base + y * 128
            for x in range(128):
                b = self.vram[row + x]
                hi, lo = b >> 4, b & 15
                pix[2 * x, y] = pal[hi] if (hi or tp) else back
                pix[2 * x + 1, y] = pal[lo] if (lo or tp) else back
        if sprites and not self.r[8] & 2: self.draw_sprites(pix, pal, H)
        return img

    def draw_sprites(self, pix, pal, H):
        r = self.r
        attr = ((r[11] & 3) << 15) | ((r[5] & 0xFC) << 7)
        col = attr - 0x200
        patb = (r[6] & 0x3F) << 11
        size = 16 if r[1] & 2 else 8
        planes = []
        for p in range(32):
            y = self.vram[attr + p * 4]
            if y == 216: break
            planes.append((p, y, self.vram[attr + p * 4 + 1], self.vram[attr + p * 4 + 2]))
        per_line = {}
        draw = []
        for p, y, x, n in planes:
            yy = (y + 1) & 255
            if yy >= 240: yy -= 256
            for ln in range(size):
                line = yy + ln
                if not 0 <= line < H: continue
                if per_line.get(line, 0) >= 8: continue
                per_line[line] = per_line.get(line, 0) + 1
                draw.append((p, line, ln, x, n))
        for p, line, ln, x, n in sorted(draw, key=lambda d: -d[0]):
            c = self.vram[col + p * 16 + ln]
            color = c & 15
            if not color: continue
            if c & 0x80: x -= 32
            pat = n & (0xFC if size == 16 else 0xFF)
            a = patb + pat * 8
            bits = self.vram[a + ln] << 8 | self.vram[a + 16 + ln] if size == 16 and ln < 8 else \
                   (self.vram[a + 8 + ln - 8] << 8 | self.vram[a + 24 + ln - 8]) if size == 16 else self.vram[a + ln] << 8
            for i in range(size):
                if bits >> (15 - i) & 1 if size == 16 else bits >> (15 - i) & 1:
                    px_ = x + i
                    if 0 <= px_ < 256: pix[px_, line] = pal[color]


class Sim:
    def __init__(self, rom, rom_base=0x4000):
        self.m = m = Z80Machine()
        self.vdp = VDP(self)
        self.t0 = 0
        self.limit = 0
        self.now = 0
        self.frame = 0
        self.border_log = []
        self.console = ''
        self.input = {}             # {'cursor': 0..8, 'joy1': 0..8, 'joy2': 0..8, 'trig': set()}
        self.keys = set()
        self.log = []
        self.traps = {v: k for k, v in BIOS.items()}
        mem = bytearray(0x10000)
        for a in self.traps: mem[a] = 0xC9
        mem[0x2D] = 1
        # VBlank handler: what the BIOS does that matters here (JIFFY++, ack the VDP) then EI, RET
        mem[0x38:0x38 + 11] = bytes([0xF5, 0x3A, 0x9E, 0xFC, 0x3C, 0x32, 0x9E, 0xFC, 0xF1, 0xFB, 0xC9])
        # the real handler reads status register 0 (IN A,(0x99)); insert it before the INC
        mem[0x38:0x38 + 13] = bytes([0xF5, 0xDB, 0x99, 0x3A, 0x9E, 0xFC, 0x3C, 0x32, 0x9E, 0xFC, 0xF1, 0xFB, 0xC9])
        mem[rom_base:rom_base + len(rom)] = rom
        m.set_memory_block(0, bytes(mem))
        m.set_input_callback(self._in)
        m.set_output_callback(self._out)
        for a in self.traps: m.set_breakpoint(a)
        m.sp = 0xF380
        m.pc = 0
        self.jiffy_addr = 0xFC9E
        # cartridge INIT: find "AB" header at $4000 and jump to INIT like the BIOS does
        if rom[:2] == b'AB': m.pc = rom[2] | rom[3] << 8
        self.irq_count = 0

    # ---- time ----
    def cur(self):
        left = self.m.ticks_to_stop
        return self.t0 + (self.limit - left if left else self.limit)

    def in_vblank(self):
        return (self.cur() % FRAME_T) < 4000

    # ---- ports ----
    def _in(self, port):
        p = port & 0xFF
        if p == 0x99: return self.vdp.read_status()
        if p == 0x98: return self.vdp.read_data()
        return 0xFF

    def _out(self, port, v):
        p = port & 0xFF
        if p == 0x99: self.vdp.write_ctrl(v)
        elif p == 0x98: self.vdp.write_data(v)
        elif p == 0x9A: self.vdp.write_pal(v)
        elif p == 0x9B: self.vdp.write_indirect(v)

    # ---- BIOS ----
    def bios(self, name):
        m = self.m
        if name == 'CHGMOD':
            v = self.vdp
            assert m.a == 5, 'only SCREEN 5 is simulated'
            for reg, val in ((0, 0x06), (1, 0x60), (2, 0x1F), (5, 0xEF), (6, 0x0F), (8, 0x08), (9, 0x80), (11, 0)):
                v.r[reg] = val
            v.vram[0:0x8000] = bytes(0x8000)
        elif name == 'WRTVDP':
            self.vdp.write_reg(m.c & 0x3F, m.b)
        elif name == 'GTSTCK':
            m.a = {0: self.input.get('cursor', 0), 1: self.input.get('joy1', 0), 2: self.input.get('joy2', 0)}.get(m.a, 0)
        elif name == 'GTTRIG':
            m.a = 0xFF if m.a in self.input.get('trig', ()) else 0
        elif name == 'SNSMAT':
            row = m.a & 15
            v = 0xFF
            for k in self.keys:
                r, b = KEYS[k]
                if r == row: v &= ~(1 << b) & 0xFF
            m.a = v
        elif name == 'CHPUT':
            self.console += chr(m.a)
        # emulate RET
        sp = m.sp
        m.pc = m.memory[sp] | (m.memory[sp + 1] << 8)
        m.sp = (sp + 2) & 0xFFFF

    # ---- run ----
    def run_until(self, t_end, on_vblank=None):
        m = self.m
        while self.now < t_end:
            if m.pc in self.traps:
                self.t0, self.limit = self.now, 0
                self.bios(self.traps[m.pc])
                self.now += 120
                continue
            next_vbl = (self.now // FRAME_T + 1) * FRAME_T
            if m.halted:
                self.now = min(next_vbl, t_end) if not self.vdp.int_line() else self.now
            budget = min(next_vbl, t_end) - self.now
            if self.vdp.int_line() and m.iff1 and not m.int_disabled:
                m.on_handle_active_int()
                self.irq_count += 1
            if budget <= 0 and self.now >= next_vbl:
                pass
            sl = min(max(budget, 1), 20000 if not self.vdp.int_line() else 40)
            if m.halted:
                sl = 0
            if sl:
                self.t0, self.limit = self.now, sl
                m.ticks_to_stop = sl
                m.run()
                used = sl - m.ticks_to_stop if m.ticks_to_stop else sl
                self.now += max(used, 1)
            if self.now >= next_vbl:       # VBlank starts
                self.vdp.s0 |= 0x80
                self.frame = self.now // FRAME_T
                if on_vblank: on_vblank(self.frame)
            if m.halted and not (self.vdp.int_line() and m.iff1):
                self.now = max(self.now, min(next_vbl, t_end))
                if self.now >= next_vbl and not (self.vdp.s0 & 0x80):
                    self.vdp.s0 |= 0x80
                    self.frame = self.now // FRAME_T
                    if on_vblank: on_vblank(self.frame)

    def run_frames(self, n, on_vblank=None):
        target = (self.now // FRAME_T + n) * FRAME_T
        self.run_until(target, on_vblank)

    # ---- helpers ----
    def peek(self, a): return self.m.memory[a]
    def peekw(self, a): return self.m.memory[a] | self.m.memory[a + 1] << 8
    def screenshot(self, path, scale=3):
        img = self.vdp.render()
        img = img.resize((img.width * scale, img.height * scale))
        img.save(path)
