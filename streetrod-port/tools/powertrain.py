#!/usr/bin/env python3
"""Modelo de desempenho do Street Rod SE (porte da rotina em SRSE.EXE img 0xDCBE, que usa ponto flutuante).
Entrada: variantes das pecas + carro; saida: razoes de marcha e curvas de torque por marcha.
Tabelas e constantes sao lidas do EXE desempacotado (DS = img+0x3E960). Transcricao do codigo; nao validada contra
execucao do original (ver NOTES.md).
uso: powertrain.py SRSE.EXE HOT_DATA"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from unexepack import unpack

DS = 0x3E960

class Model:
    def __init__(self, exe):
        self.img = unpack(open(exe, 'rb').read())[1]
        w = lambda a, n: list(struct.unpack('<%dh' % n, self.img[DS + a: DS + a + 2 * n]))
        self.t_wear_tire = w(0x54B2, 15)   # risco do pneu por desgaste (5 por variante)
        self.t_ratio = w(0x54DC, 20)       # fator de relacao por marcha (5 por variante de cambio)
        self.t_accel = w(0x5504, 15)       # aceleracao base por marcha (5 por variante de motor)
        self.t_rpm = w(0x5522, 6)          # limites de rpm das curvas (2 por variante de motor)
        self.t_combo = w(0x552E, 15)       # compatibilidade carburador x coletor (5 por carburador)
        self.t_eng = w(0x554C, 3)          # bonus do motor (%)
        self.t_trans = w(0x5552, 4)        # bonus do cambio (%)
        self.t_redline = w(0x555A, 3)
        self.t_gears = w(0x5560, 4)
        self.t_tire_drag = w(0x5568, 3)
        self.t_body = w(0x556E, 9)         # classe da carroceria (desempenho 0..8)
        self.k = struct.unpack('<20d', self.img[DS + 0x6988: DS + 0x6988 + 160])

    def build(self, perf, engine, trans, manifold, carb, tires, engine_wear=0, trim=0, damage=(0, 0, 0)):
        """engine/trans/manifold/carb/tires = variantes do catalogo (byte 0 da entrada); damage = (f1,f2,f3) 0/1."""
        acc = damage[0] * 0.09 + damage[1] * 0.05 + damage[2] * 0.01
        acc += self.t_body[perf] * 0.1 / 88.0
        acc += self.t_combo[5 * carb + manifold] * 0.15 / 100.0
        acc += self.t_eng[engine] * 0.25 / 100.0
        acc += self.t_trans[trans] * 0.15 / 100.0
        acc += (1.0 - abs(trim / 8.0)) * 0.2
        acc *= (10000.0 - engine_wear) / 10000.0
        m = 0.8 if trans == 0 else 1.0
        B = m * acc * 0.3 + 0.7
        A = min(130.0, B * 130.0)
        gears = []
        w1, w2 = self.t_rpm[2 * engine], self.t_rpm[2 * engine + 1]
        for g in range(5):
            t = self.t_ratio[5 * trans + g]
            x = t * A / 100.0
            if x == 0: x = 0.2
            ratio = int(5700.0 / x)
            v = int(self.t_accel[5 * engine + g] * B)
            q = int(v / (1 if g == 0 else 5))
            a0 = q * (2 if (g == 1 and tires != 2) else 1) * (3 if (g == 1 and tires == 2) else 1)
            third = int(v / (5 - g)) if g else v
            segs = [dict(lo=0, hi=w1, accel=a0, aux=third),
                    dict(lo=w1, hi=w2, accel=v, aux=third),
                    dict(lo=w2, hi=5700, accel=int(v / (10 if g else 1)), aux=third)]
            gears.append(dict(ratio=ratio, segs=segs))
        return dict(power=A, quality=B, n_gears=self.t_gears[trans], redline=self.t_redline[engine],
                    drag=int((130 - perf) * 5.0 / 50.0 + self.t_tire_drag[tires]), gears=gears)

def stock_variants(flags):
    return dict(maker=(flags & 0xF) >> 1, tires=(flags >> 4) & 3, trans=(flags >> 6) & 3,
                engine=(flags >> 8) & 7, manifold=(flags >> 11) & 7, carb=(flags >> 14) & 3)

if __name__ == '__main__':
    exe, hot = sys.argv[1:3]
    from hotdata import SIZES
    h = open(hot, 'rb').read()
    b1 = h[SIZES[0]:SIZES[0] + SIZES[1]]
    pool = h[sum(SIZES[:6]): sum(SIZES[:7])]
    M = Model(exe)
    for i in range(25):
        price, perf, idx, flags, noff = struct.unpack('<HBBHH', b1[10 * i:10 * i + 8])
        v = stock_variants(flags)
        r = M.build(perf, v['engine'], v['trans'], v['manifold'], v['carb'], v['tires'])
        name = pool[noff:pool.index(b'\0', noff)].decode()
        g = r['gears']
        print('%-40s $%4d perf%d eng%d tr%d mf%d cb%d ti%d | pot %5.1f gears %d | ratios %s | accel(g1..) %s' % (
            name[:40], price, perf, v['engine'], v['trans'], v['manifold'], v['carb'], v['tires'], r['power'], r['n_gears'],
            [x['ratio'] for x in g], [x['segs'][0]['accel'] for x in g]))
