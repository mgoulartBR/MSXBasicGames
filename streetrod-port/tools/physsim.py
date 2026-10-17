#!/usr/bin/env python3
"""Simula a fisica de arrancada (logica de SRSE.EXE img 0xD29D) com o modelo de pecas (powertrain.py).
uso: physsim.py SRSE.EXE HOT_DATA"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from powertrain import Model, stock_variants
from hotdata import SIZES

TICK_HZ = 72.826

def simulate(r, shift_frac=0.97, seconds=30, auto=False):
    """r = saida de Model.build. Devolve lista (t, mph, marcha)."""
    gears = r['gears']
    n = r['n_gears']
    gear, rpm = 0, 1500
    seg = 0
    speed = 0
    out = []
    cap = 5300
    for tick in range(int(seconds * TICK_HZ)):
        g = gears[gear]
        # segmento atual pelo rpm
        s = 0
        while s < 2 and rpm >= g['segs'][s]['hi']: s += 1
        rpm = min(cap, rpm + g['segs'][s]['accel'])
        speed = rpm // g['ratio'] if g['ratio'] else 0
        if gear < n:
            limit = min(cap, r['redline']) * shift_frac if gear else 2900
            if rpm >= limit:
                gear += 1
                rpm = speed * gears[gear]['ratio']
        out.append((tick / TICK_HZ, speed, gear))
    return out

if __name__ == '__main__':
    exe, hot = sys.argv[1:3]
    h = open(hot, 'rb').read()
    b1 = h[SIZES[0]:SIZES[0] + SIZES[1]]
    pool = h[sum(SIZES[:6]): sum(SIZES[:7])]
    M = Model(exe)
    for i in (0, 1, 2, 12, 17, 20):
        price, perf, idx, flags, noff = struct.unpack('<HBBHH', b1[10 * i:10 * i + 8])
        v = stock_variants(flags)
        r = M.build(perf, v['engine'], v['trans'], v['manifold'], v['carb'], v['tires'])
        tr = simulate(r)
        t60 = next((t for t, s, g in tr if s >= 60), None)
        t100 = next((t for t, s, g in tr if s >= 100), None)
        top = max(s for t, s, g in tr)
        print('%-34s 0-60: %s s  0-100: %s s  max %d mph' % (pool[noff:pool.index(b'\0', noff)].decode()[:34],
              '%.2f' % t60 if t60 else '-', '%.2f' % t100 if t100 else '-', top))
