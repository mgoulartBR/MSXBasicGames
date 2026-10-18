#!/usr/bin/env python3
"""Gera tables.h (tabelas const em ROM) para o protótipo da estrada MSX2.
Modelo de projeção baseado em SRSE.EXE (img 0x27BBF/0x27C01): x = cx + ((off - P)*k)/z, y = HZ + (camH - h)*k/z."""
import math

HALF_ROAD = 900          # metade da largura da pista (unidades do jogo original: 0x5DC..0xCE4 / 2)
CAM_K = 1000             # constante vertical: y = HZ + CAM_K/z
ZMAX = 128

def recip(z): return min(65535, round(8192 / z)) if z else 0

out = []
out.append('/* gerado por gen_tables.py - nao editar */')
out.append('#include <stdint.h>')
out.append('const uint16_t recip_tab[%d] = {%s};' % (ZMAX, ','.join(str(recip(z)) for z in range(ZMAX))))
out.append('const uint16_t hw_tab[%d] = {%s};' % (ZMAX, ','.join(str((HALF_ROAD * recip(z)) >> 12) for z in range(ZMAX))))
out.append('const uint8_t rc8_tab[%d] = {%s};' % (ZMAX, ','.join(str(min(255, round(1024 / z)) if z else 255) for z in range(ZMAX))))
hwy = [max(0, round((y - 36) * 2 * HALF_ROAD / CAM_K)) for y in range(256)]
out.append('const uint16_t hwy_tab[256] = {%s};' % ','.join(map(str, hwy)))
out.append('const uint8_t kwy_tab[256] = {%s};' % ','.join(str(max(1, h // 9)) for h in hwy))
out.append('const uint8_t ytab[%d] = {%s};' % (ZMAX, ','.join(str(min(255, round(CAM_K / z)) if z else 255) for z in range(ZMAX))))
out.append('const uint8_t kw_tab[%d] = {%s};' % (ZMAX, ','.join(str(max(1, ((HALF_ROAD * recip(z)) >> 12) // 9)) for z in range(ZMAX))))

out.append('const uint16_t rdy_tab[256] = {%s};' % ','.join(str(min(65535, 65536 // d) if d else 65535) for d in range(256)))

# pista sintética de 256 segmentos: reta, curva esq., reta, curva dir., S, etc. (curvatura i8)
segs = [(18, 0), (22, -14), (14, 0), (26, 20), (12, 0), (10, -24), (10, 24), (16, 0), (20, -10), (14, 0), (24, 16), (14, 0), (16, -18), (10, 0), (10, 12)]
curve = []
for n, c in segs: curve += [c] * n
while len(curve) < 256: curve.append(0)
curve = curve[:256]
# suaviza entradas/saídas de curva (rampa de 4 segmentos)
sm = curve[:]
for i in range(256):
    w = [curve[(i + k) % 256] for k in range(-2, 3)]
    sm[i] = int(sum(w) / 5)
curve = sm
hill = [int(45 * math.sin(i * 2 * math.pi / 64) + 25 * math.sin(i * 2 * math.pi / 23)) for i in range(256)]
out.append('const int8_t curve_tab[256] = {%s};' % ','.join(map(str, curve)))
out.append('const int8_t hill_tab[256] = {%s};' % ','.join(map(str, hill)))

# limiares de mph da estrada (SRSE.EXE img 0xD544): passo 1..16 subunidades por tick
thr = [13, 27, 41, 55, 69, 83, 97, 101, 105, 109, 113, 117, 121, 125, 129]
out.append('const uint8_t step_thr[15] = {%s};' % ','.join(map(str, thr)))

def rgb(r, g, b): return ((r << 4) | b, g)   # MSX2: byte0 = R<<4|B, byte1 = G
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from palette_def import PAL as pal
flat = []
for c in pal: flat += list(rgb(*c))
out.append('const uint8_t palette[32] = {%s};' % ','.join(map(str, flat)))
# --- powertrain dos dois carros (jogador CAR e oponente OPP_CAR) a partir do modelo extraido de SRSE.EXE
import os, struct, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
CAR = int(os.environ.get('CAR', '17'))
OPP_CAR = int(os.environ.get('OPP_CAR', '12'))
OPP = int(os.environ.get('OPP', '0'))          # indice do piloto oponente (0 = Mike ... 21 = The King)
def default_pt(n_gears, redline):
    mk = lambda r, a: dict(ratio=r, segs=[dict(lo=0, hi=2900, accel=a[0]), dict(lo=2900, hi=redline, accel=a[1]), dict(lo=redline, hi=5700, accel=a[2])])
    return dict(n_gears=n_gears, redline=redline, trans=3, gears=[mk(28500, (573, 573, 57)), mk(153, (573, 435, 43)), mk(91, (72, 72, 7)), mk(57, (26, 26, 2)), mk(45, (17, 17, 1))])
pts, opp_ab = [], (20, 10)
try:
    from powertrain import Model, stock_variants
    from hotdata import SIZES
    exe, hot = os.environ['SRSE_EXE'], os.environ['HOT_DATA']
    hd = open(hot, 'rb').read()
    b1 = hd[SIZES[0]:SIZES[0] + SIZES[1]]
    M = Model(exe)
    for cid in (CAR, OPP_CAR):
        price, perf, idx, flags, noff = struct.unpack('<HBBHH', b1[10 * cid:10 * cid + 8])
        v = stock_variants(flags)
        p = M.build(perf, v['engine'], v['trans'], v['manifold'], v['carb'], v['tires']); p['trans'] = v['trans']
        pts.append(p)
    rec = hd[:SIZES[0]][18 * OPP:18 * OPP + 18]
    opp_ab = (rec[1], rec[2])
    src = 'carros %d/%d, piloto %d' % (CAR, OPP_CAR, OPP)
except Exception as e:                       # sem os arquivos do jogo: curvas genericas
    sys.stderr.write('powertrain padrao (%s)\n' % e)
    pts = [default_pt(4, 5200), default_pt(4, 4500)]
    src = 'padrao'
out.append('/* powertrain: %s */' % src)
out.append('const uint16_t pt_ratio[10] = {%s};' % ','.join(str(x['ratio']) for p in pts for x in p['gears']))
out.append('const uint16_t pt_hi0[10] = {%s};' % ','.join(str(x['segs'][0]['hi']) for p in pts for x in p['gears']))
out.append('const uint16_t pt_hi1[10] = {%s};' % ','.join(str(x['segs'][1]['hi']) for p in pts for x in p['gears']))
out.append('const uint16_t pt_acc[30] = {%s};' % ','.join(str(s['accel']) for p in pts for x in p['gears'] for s in x['segs']))
out.append('const uint8_t pt_ngears[2] = {%s};' % ','.join(str(p['n_gears']) for p in pts))
out.append('const uint16_t pt_redline[2] = {%s};' % ','.join(str(p['redline']) for p in pts))
out.append('const uint8_t pt_auto[2] = {%s};' % ','.join('1' if p['trans'] == 0 else '0' for p in pts))
out.append('#define OPP_A %d' % opp_ab[0])
out.append('#define OPP_B %d' % opp_ab[1])
open('tables.h', 'w').write('\n'.join(out) + '\n')
print('tables.h ok; curve range', min(curve), max(curve), 'hill range', min(hill), max(hill))
