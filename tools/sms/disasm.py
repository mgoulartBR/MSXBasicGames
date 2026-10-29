#!/usr/bin/env python3
"""Recursive, bank-aware Z80 disassembler for Sega Master System ROMs (Sega mapper).

Usage: disasm.py ROM.sms OUTDIR [trace.json ...]

Memory model: $0000-$3FFF bank 0, $4000-$7FFF bank 1, $8000-$BFFF slot 2 (switched
by writes to $FFFF). The slot-2 bank is tracked through `LD A,n / LD ($FFFF),A`
and `LD A,n / CALL <bank-switch helper>` sequences. Code that is never reached is
dumped as `.db` so nothing is lost; check coverage.txt for what to hand-annotate.
"""
import sys, os
from z80dis import z80
from z80dis.z80 import OP, OPER_TYPE

BANK = 0x4000


def phys(addr, bank2):
    if addr < 0x4000: return addr
    if addr < 0x8000: return addr
    if addr < 0xC000 and bank2 is not None: return bank2 * BANK + (addr - 0x8000)
    return None


def vaddr(p):
    b, o = divmod(p, BANK)
    return o if b == 0 else (0x4000 + o if b == 1 else 0x8000 + o)


def label(p):
    b = p // BANK
    return f"B{b:02X}_{vaddr(p):04X}"


class Dis:
    def __init__(self, rom):
        self.rom = rom
        self.insn = {}        # phys -> decoded
        self.labels = set()
        self.unresolved = []
        self.seen = set()
        self.helpers = set()  # routines that do LD ($FFFF),A; RET

    def find_helpers(self):
        r = self.rom
        for i in range(len(r) - 4):
            if r[i:i+3] == b'\x32\xff\xff' and r[i+3] == 0xC9 and i < 0x8000:
                self.helpers.add(i)

    def run(self, entries):
        self.find_helpers()
        work = [(a, 2, None, ()) if isinstance(a, int) else (a[1], a[0], None, ()) for a in entries]
        while work:
            pc, b2, a, stk = work.pop()
            while True:
                p = phys(pc, b2)
                if p is None:
                    self.unresolved.append((pc, b2)); break
                key = (p, b2)
                if key in self.seen: break
                self.seen.add(key)
                if p + 1 >= len(self.rom): break
                i = z80.decode(self.rom[p:p+4], pc)
                if i.op == OP.NOP and self.rom[p] != 0 and False: break
                self.insn[p] = i
                op, o, n = i.op, i.operands, i.len
                nxt = pc + n

                def tgt(): return o[-1][1] if o and o[-1][0] == OPER_TYPE.ADDR else None
                def tgt_label(t):
                    q = phys(t, b2)
                    if q is not None: self.labels.add(q)
                    return q
                # track A and the slot-2 bank
                txt = z80.disasm(i)
                if txt.startswith('LD A,') and o[1][0] == OPER_TYPE.IMM: a = o[1][1] & 0xFF
                elif txt == 'XOR A' or txt == 'SUB A': a = 0
                elif txt == 'LD A,(0xffff)': a = b2
                elif txt.startswith('LD A,') or (txt.startswith(('INC A', 'DEC A', 'ADD A', 'AND', 'OR', 'XOR', 'POP AF'))) or txt.startswith('LD ' + 'A'):
                    if txt == 'POP AF' and stk: a, stk = stk[-1], stk[:-1]
                    else: a = None
                if txt == 'PUSH AF': stk = stk + (a,)
                if txt == 'LD (0xffff),A' and a is not None: b2 = a & 0x1F
                elif txt == 'LD (0xffff),A': b2 = None

                if op in (OP.RET, OP.RETI, OP.RETN) and i.cond_is_none() if hasattr(i, 'cond_is_none') else False: break
                cond = any(x[0] == OPER_TYPE.COND for x in o) if o else False
                if op in (OP.RET, OP.RETI, OP.RETN):
                    if not cond: break
                elif op == OP.JP:
                    if o and o[0][0] == OPER_TYPE.REG_DEREF or (o and o[0][0] == OPER_TYPE.REG):
                        break  # JP (HL)/(IX)
                    t = tgt()
                    if t is not None and t < 0xC000:
                        tgt_label(t); work.append((t, b2, a, stk))
                    if not cond: break
                elif op == OP.JR:
                    t = tgt()
                    tgt_label(t); work.append((t, b2, a, stk))
                    if not cond: break
                elif op == OP.DJNZ:
                    t = tgt(); tgt_label(t); work.append((t, b2, a, stk))
                elif op == OP.CALL:
                    t = tgt()
                    if t is not None and t < 0xC000:
                        q = tgt_label(t)
                        if q in self.helpers and not cond:
                            if a is not None: b2 = a & 0x1F
                            else: b2 = None
                        work.append((t, b2, None, ()))
                elif op == OP.RST:
                    t = o[0][1]
                    tgt_label(t); work.append((t, b2, None, ()))
                pc = nxt
        return self

    def write(self, outdir):
        os.makedirs(outdir, exist_ok=True)
        rom = self.rom
        covered = bytearray(len(rom))
        for p, i in self.insn.items():
            for k in range(i.len): covered[p + k] = 1
        for bank in range(len(rom) // BANK):
            lines = [f"; ---- bank {bank:02X} (file ${bank*BANK:05X}) ----"]
            p, end = bank * BANK, (bank + 1) * BANK
            while p < end:
                if p in self.labels or p % BANK == 0:
                    lines.append(f"{label(p)}:")
                if p in self.insn:
                    i = self.insn[p]
                    lines.append(f"    {z80.disasm(i):<28}; {vaddr(p):04X}")
                    p += i.len
                else:
                    q = p
                    while q < end and (q == p or (q not in self.insn and q not in self.labels)) and q - p < 16: q += 1
                    chunk = ','.join(f"${x:02X}" for x in rom[p:q])
                    lines.append(f"    .db {chunk:<40}; {vaddr(p):04X}")
                    p = q
            with open(os.path.join(outdir, f"bank{bank:02X}.asm"), 'w') as f:
                f.write('\n'.join(lines) + '\n')
        with open(os.path.join(outdir, "coverage.txt"), 'w') as f:
            tot = len(rom)
            f.write(f"code bytes: {sum(covered)} / {tot} ({100*sum(covered)/tot:.1f}%)\n")
            for bank in range(tot // BANK):
                c = sum(covered[bank*BANK:(bank+1)*BANK])
                f.write(f"bank {bank:02X}: {c:5d}/{BANK} code ({100*c/BANK:5.1f}%)\n")
            f.write(f"\nbank-switch helpers: {[hex(h) for h in sorted(self.helpers)]}\n")
            f.write(f"unresolved targets (slot-2 bank unknown): {len(set(self.unresolved))}\n")
            for pc, b in sorted(set(self.unresolved))[:200]: f.write(f"  {pc:04X}\n")


if __name__ == '__main__':
    import json
    rom = open(sys.argv[1], 'rb').read()
    entries = [0x0000, 0x0038, 0x0066]
    for tf in sys.argv[3:]:     # optional trace files from trace.py: [[bank, pc], ...]
        entries += [tuple(e) for e in json.load(open(tf))]
    d = Dis(rom).run(entries)
    d.write(sys.argv[2])
    print(open(os.path.join(sys.argv[2], 'coverage.txt')).read()[:1500])
