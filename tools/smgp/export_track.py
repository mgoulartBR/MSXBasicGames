#!/usr/bin/env python3
"""Converte UMA pista de Super Monaco GP (ROM do usuário) em dados para o protótipo MSX.

Uso: python3 -I export_track.py <rom.md> <pista 0..15> <saida.inc>

Gera, por posição da pista (1 posição = 4 unidades de volta):
  trkc : curvatura com sinal (int8) = ±(V/L) * CGAIN, suavizada (V = valor, L = comprimento do segmento)
  trkh : altura relativa à largada (int8) = unidades * HSCALE
O arquivo gerado contém dados derivados da ROM: use só localmente e NÃO o versione.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tracks as T  # noqa: E402

CGAIN, HSCALE, SMOOTH = 18.0, 2.0, 3


def expand(rom, r):
    t = T.track(rom, r)
    if t is None:
        sys.exit("pista %d fora da tabela" % r)
    # curvatura por posição (bit 6 do flag = sentido negativo)
    c = []
    for n, fl, v in t["curve"]:
        k = 0.0 if not fl else (v / n) * (-1 if fl & 0x40 else 1)
        c += [k] * n
    # altura por posição: byte inicial (com sinal) + slope/256 por posição (bit 6 = descida)
    h0 = rom[t["ptrs"][1]]
    cur = float(h0 - 256 if h0 > 127 else h0)
    h = []
    for n, fl, v in t["relief"]:
        d = 0.0 if not fl else (v / 256.0) * (-1 if fl & 0x40 else 1)
        for _ in range(n):
            h.append(cur)
            cur += d
    assert len(c) == len(h) == t["lap"] // 4, "fluxos inconsistentes"
    return t, c, h


def clamp8(x, lim=127):
    return max(-lim, min(lim, int(round(x))))


def main():
    rom = open(sys.argv[1], "rb").read()
    r = int(sys.argv[2])
    t, c, h = expand(rom, r)
    n = len(c)
    cs = [sum(c[(i + d) % n] for d in range(-SMOOTH, SMOOTH + 1)) / (2 * SMOOTH + 1) for i in range(n)]
    trkc = [clamp8(x * CGAIN) for x in cs]
    trkh = [clamp8(x * HSCALE, 63) for x in h]

    def db(name, vals, per=24):
        out = [name + ":"]
        for i in range(0, len(vals), per):
            out.append("        defb " + ",".join(str(v & 0xFF) for v in vals[i:i + per]))
        return out

    lines = ["; DERIVADO DA ROM DO USUARIO - nao versionar. pista %d, %d posicoes" % (r, n),
             "TRKLEN  equ %d" % n] + db("trkc", trkc) + db("trkh", trkh)
    open(sys.argv[3], "w").write("\n".join(lines) + "\n")
    print("pista %d: %d posicoes | curvatura max |c|=%d | altura %d..%d (x%g)" % (
        r, n, max(abs(x) for x in trkc), min(trkh), max(trkh), HSCALE))


if __name__ == "__main__":
    main()
