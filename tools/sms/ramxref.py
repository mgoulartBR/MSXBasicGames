#!/usr/bin/env python3
"""Static cross-reference of RAM ($C000-$DFFF) absolute accesses in a disassembled ROM.

Usage: ramxref.py ROM.sms OUT.txt trace.json [...]
Lists, per RAM address, which instruction addresses read/write it (absolute operands,
`LD (nn),r`, `LD r,(nn)`, `INC/DEC (nn)` ...), plus the 16-bit immediate loads (`LD HL,nn`,
`LD IX,nn`) that point at RAM structures.
"""
import sys, os, json, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from disasm import Dis, vaddr
from z80dis import z80
from z80dis.z80 import OPER_TYPE


def main(rom_path, out, traces):
    rom = open(rom_path, 'rb').read()
    entries = [0, 0x38, 0x66]
    for t in traces: entries += [tuple(e) for e in json.load(open(t))]
    d = Dis(rom).run(entries)
    refs = collections.defaultdict(lambda: {'r': [], 'w': [], 'imm': []})
    for p, i in sorted(d.insn.items()):
        txt = z80.disasm(i)
        for k, (ty, v) in enumerate(i.operands):
            if not isinstance(v, int) or not (0xC000 <= v < 0xE000): continue
            if ty == OPER_TYPE.ADDR_DEREF:
                kind = 'w' if k == 0 else 'r'
                refs[v][kind].append(vaddr(p))
            elif ty == OPER_TYPE.IMM and k == 1 and txt.startswith('LD') and i.len >= 3:
                refs[v]['imm'].append(vaddr(p))
    with open(out, 'w') as f:
        for a in sorted(refs):
            r = refs[a]
            f.write(f"{a:04X}  R:{' '.join(f'{x:04X}' for x in sorted(set(r['r']))[:12]):<60} W:{' '.join(f'{x:04X}' for x in sorted(set(r['w']))[:12]):<60} IMM:{' '.join(f'{x:04X}' for x in sorted(set(r['imm']))[:8])}\n")
    print(len(refs), "RAM addresses referenced")


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3:])
