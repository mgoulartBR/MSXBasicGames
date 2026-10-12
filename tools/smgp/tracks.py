#!/usr/bin/env python3
"""Lista as pistas de Super Monaco GP (Mega Drive, USA) a partir da ROM do usuário.

Uso: python3 -I tracks.py <rom.md> [n_registros]

Formato (obtido por engenharia reversa; veja notes/super-monaco-gp-analysis.md):
  tabela de registros em 0xFBDA, 0x48 bytes cada; em +0x20: flag(w) lap(w) ... e,
  em +0x30/+0x34/+0x38, ponteiros (long) para 3 fluxos de segmentos:
    curvatura : [len(w) flag(b) [valor(w) se flag!=0]]...  termina em byte com bit7
    relevo    : altura_inicial(b) + [len(w) flag(b) [inclinação(b) se flag!=0]]...
    zonas     : [len(w) valor(b)]...
  Os três fluxos somam o mesmo nº de posições (= lap / 4).
"""
import struct
import sys

BASE, REC = 0xFBDA, 0x48


def u16(rom, o):
    return struct.unpack(">H", rom[o:o + 2])[0]


def u32(rom, o):
    return struct.unpack(">I", rom[o:o + 4])[0]


def stream(rom, p, kind):
    """Decodifica um fluxo. Retorna lista de (comprimento, flag, valor)."""
    o, segs = p, []
    if kind == "relief":
        o += 1
    while not rom[o] & 0x80:
        n = (rom[o] << 8) | rom[o + 1]
        o += 2
        if kind == "zone":
            segs.append((n, rom[o], 0))
            o += 1
            continue
        fl = rom[o]
        o += 1
        val = 0
        if fl and kind == "curve":
            val = u16(rom, o)
            o += 2
        elif fl and kind == "relief":
            val = rom[o]
            o += 1
        segs.append((n, fl, val))
    return segs


def track(rom, r):
    o = BASE + r * REC + 0x20
    ptrs = [u32(rom, o + 0x10), u32(rom, o + 0x14), u32(rom, o + 0x18)]
    if not all(0 < p < len(rom) for p in ptrs):
        return None
    return {"index": r, "lap": u16(rom, o + 2),
            "curve": stream(rom, ptrs[0], "curve"),
            "relief": stream(rom, ptrs[1], "relief"),
            "zone": stream(rom, ptrs[2], "zone"), "ptrs": ptrs}


def main():
    rom = open(sys.argv[1], "rb").read()
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 20
    print("reg  lap   pos  curva relevo zonas  ponteiros")
    for r in range(n):
        t = track(rom, r)
        if t is None:
            print("%2d  fim da tabela" % r)
            break
        tot = [sum(s[0] for s in t[k]) for k in ("curve", "relief", "zone")]
        ok = "ok" if len(set(tot)) == 1 and tot[0] * 4 == t["lap"] else "INCONSISTENTE"
        print("%2d  %5d %5d %5d %5d %5d  %s  %s" % (
            r, t["lap"], tot[0], len(t["curve"]), len(t["relief"]),
            len(t["zone"]), " ".join("%06X" % p for p in t["ptrs"]), ok))


if __name__ == "__main__":
    main()
