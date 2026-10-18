#!/usr/bin/env python3
"""Gera tables.h (tabelas const em ROM) para o protótipo da estrada MSX2.
Modelo de projeção baseado em SRSE.EXE (img 0x27BBF/0x27C01): x = cx + ((off - P)*k)/z, y = HZ + (camH - h)*k/z."""
import math, os

HZ = 22                  # linha do horizonte na janela da estrada
WIDE = 2.1               # fator visual de largura da pista (so desenho; a fisica continua com HALF_ROAD)
HALF_ROAD = 900          # metade da largura da pista (unidades do jogo original: 0x5DC..0xCE4 / 2)
CAM_K = 1866             # constante vertical: y = HZ + CAM_K/z
ZMAX = 128

def recip(z): return min(65535, round(8192 / z)) if z else 0

out = []
out.append('/* gerado por gen_tables.py - nao editar */')
out.append('#include <stdint.h>')
out.append('#define HZ %d' % HZ)
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'layout.inc'), 'w').write('\t.equ HZ, %d\n' % HZ)
out.append('const uint16_t recip_tab[%d] = {%s};' % (ZMAX, ','.join(str(recip(z)) for z in range(ZMAX))))
out.append('const uint16_t hw_tab[%d] = {%s};' % (ZMAX, ','.join(str(round(WIDE * HALF_ROAD * recip(z)) >> 12) for z in range(ZMAX))))
out.append('const uint8_t rc8_tab[%d] = {%s};' % (ZMAX, ','.join(str(min(255, round(WIDE * 1024 / z)) if z else 255) for z in range(ZMAX))))
hwy = [max(0, round((y - HZ) * 2 * WIDE * HALF_ROAD / CAM_K)) for y in range(256)]
out.append('const uint16_t hwy_tab[256] = {%s};' % ','.join(map(str, hwy)))
out.append('const uint8_t kwy_tab[256] = {%s};' % ','.join(str(max(1, h // 9)) for h in hwy))
out.append('const uint8_t ytab[%d] = {%s};' % (ZMAX, ','.join(str(min(255, round(CAM_K / z)) if z else 255) for z in range(ZMAX))))
out.append('const uint8_t kw_tab[%d] = {%s};' % (ZMAX, ','.join(str(max(1, (round(WIDE * HALF_ROAD * recip(z)) >> 12) // 9)) for z in range(ZMAX))))

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
# --- powertrain de todos os carros (modelo extraido de SRSE.EXE), nomes e pilotos: cars.h. O carro/piloto escolhidos vem de um menu.
import os, struct, sys, re
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
CAR = int(os.environ.get('CAR', '17'))
OPP_CAR = int(os.environ.get('OPP_CAR', '12'))
OPP = int(os.environ.get('OPP', '0'))          # indice do piloto oponente (0 = Mike ... 21 = The King)
NCARS = 26
def default_pt(n_gears, redline):
    mk = lambda r, a: dict(ratio=r, segs=[dict(lo=0, hi=2900, accel=a[0]), dict(lo=2900, hi=redline, accel=a[1]), dict(lo=redline, hi=5700, accel=a[2])])
    return dict(n_gears=n_gears, redline=redline, trans=3, gears=[mk(28500, (573, 573, 57)), mk(153, (573, 435, 43)), mk(91, (72, 72, 7)), mk(57, (26, 26, 2)), mk(45, (17, 17, 1))])
def clean(s, n):
    s = s.upper().replace(' & ', ' ').replace('CONVERTIBLE', 'CONV').replace('CONV.', 'CONV').replace('SEDAN', 'SDN').replace('CHEVROLET', 'CHEVY').replace('OLDSMOBILE', 'OLDS').replace('PLYMOUTH', 'PLYM').replace('CHRYSLER', 'CHRY')
    s = re.sub(r'[^A-Z0-9 .\-]', '', s).strip(' .')
    return s[:n]
pts, names, pilots, prices, parts = [], [], [], [], []
try:
    from powertrain import Model, stock_variants
    from hotdata import SIZES, cstr
    exe, hot = os.environ['SRSE_EXE'], os.environ['HOT_DATA']
    hd = open(hot, 'rb').read()
    b0 = hd[:SIZES[0]]; b1 = hd[SIZES[0]:SIZES[0] + SIZES[1]]; pool = hd[sum(SIZES[:6]):sum(SIZES[:7])]
    M = Model(exe)
    model_tabs = [('pm_ratio', M.t_ratio), ('pm_accel', M.t_accel), ('pm_rpm', M.t_rpm), ('pm_combo', M.t_combo), ('pm_eng', M.t_eng), ('pm_trans', M.t_trans), ('pm_redline', M.t_redline), ('pm_gears', M.t_gears), ('pm_body', M.t_body)]
    carperf, carflags = [], []
    for cid in range(NCARS):
        price, perf, idx, flags, noff = struct.unpack('<HBBHH', b1[10 * cid:10 * cid + 8])
        v = stock_variants(flags)
        p = M.build(perf, v['engine'], v['trans'], v['manifold'], v['carb'], v['tires']); p['trans'] = v['trans']
        pts.append(p); names.append(clean(cstr(pool, noff), 27)); prices.append(price); carperf.append(perf); carflags.append(flags)
    for i in range(22):
        r = b0[18 * i:18 * i + 18]
        pilots.append((clean(cstr(pool, struct.unpack('<H', r[6:8])[0]), 10), r[1], r[2]))
    src = 'jogo original'
    # catalogo de pecas (43): nome curto montado a partir de categoria/variante/montadora
    import csv, io, contextlib
    hot_out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'hotdata_out')
    import hotdata
    with contextlib.redirect_stdout(io.StringIO()): hotdata.main(exe, hot, hot_out)
    MK = {'GM': 'GM', 'Ford': 'FORD', 'Chrysler': 'CHRY', 'all': 'ALL'}
    for r in csv.DictReader(open(os.path.join(hot_out, 'parts.csv'))):
        mk, v, c = MK[r['makers']], int(r['variant']), r['category']
        if c == 'engine':
            cu = re.search(r'(\d{3}) cu', r['text']); name = '%s V-6' % mk if v == 0 else '%s V-8 %s CI' % (mk, cu.group(1))
        elif c == 'transmission': name = ['AUTO TRANS %s', '3-SPD %s', '4-SPD %s', 'RACING 4-SPD %s'][v] % mk
        elif c == 'carburetor': name = ['2-BRL CARB', '4-BRL CARB', 'RACING 4-BRL CARB'][v]
        elif c == 'manifold': name = '%s %s' % (mk, ['2-BRL MANIFOLD', '4-BRL MANIFOLD', '3X2 MANIFOLD', '2X4 MANIFOLD', 'RACING MANIFOLD'][v])
        else: name = ['TIRES', 'BRAND NAME TIRES', 'RACING SLICKS'][v]
        parts.append((name, int(r['price']), ['engine', 'transmission', 'carburetor', 'manifold', 'tires'].index(c), {'GM': 1, 'FORD': 2, 'CHRY': 4, 'ALL': 7}[mk], v))
except Exception as e:                       # sem os arquivos do jogo: curvas genericas
    sys.stderr.write('powertrain padrao (%s)\n' % e)
    pts = [default_pt(4, 5200 - 100 * (i % 5)) for i in range(NCARS)]
    model_tabs = [('pm_ratio', [0,100,60,40,30]*4), ('pm_accel', [50,40,30,20,10]*3), ('pm_rpm', [2900,5200]*3), ('pm_combo', [0]*15), ('pm_eng', [0,5,10]), ('pm_trans', [0,0,5,10]), ('pm_redline', [5200,5300,5400]), ('pm_gears', [3,3,4,4]), ('pm_body', list(range(9)))]
    carperf = [i % 9 for i in range(NCARS)]; carflags = [0x0141] * NCARS
    names = ['DEFAULT CAR %d' % i for i in range(NCARS)]; prices = [500 + 100 * i for i in range(NCARS)]
    parts = [('PART %d' % i, 20 + 5 * i, i % 5, 7, 0) for i in range(43)]
    pilots = [('DRIVER %d' % i, 20 - i // 3, 10 - i // 4) for i in range(22)]
    src = 'padrao'
def spr_set(n):                              # vista traseira do carro: 0 sedan generico, 1 perua/woodie, 2 Corvette
    return 2 if 'CORVETTE' in n else 1 if ('WOODIE' in n or 'WAGON' in n or 'NOMAD' in n) else 0
cars = ['/* gerado por gen_tables.py (%s) */' % src, '#define NCARS %d' % NCARS, '#define NPILOTS %d' % len(pilots),
        '#define DEF_CAR %d' % CAR, '#define DEF_OPP_CAR %d' % OPP_CAR, '#define DEF_OPP %d' % OPP]
def arr(t, name, vals): cars.append('static const %s %s[%d] = {%s};' % (t, name, len(vals), ','.join(map(str, vals))))
for n_, t_ in model_tabs: arr('uint8_t' if max(t_) < 256 and min(t_) >= 0 else 'int16_t', n_, t_)      # tabelas pequenas em bytes (evita padroes 32 00 50 que confundem o detector de mapper)
arr('uint8_t', 'car_perf', carperf); arr('uint16_t', 'car_flags', carflags)
arr('uint8_t', 'car_ngears', [p['n_gears'] for p in pts]); arr('uint16_t', 'car_redline', [p['redline'] for p in pts]); arr('uint8_t', 'car_auto', [1 if p['trans'] == 0 else 0 for p in pts])
arr('uint8_t', 'car_spr', [spr_set(n) for n in names])
arr('uint16_t', 'car_price', prices)
cars.append('#define NPARTS %d' % len(parts))
cars.append('static const char part_name[%d][21] = {%s};' % (len(parts), ','.join('"%s"' % p[0][:20] for p in parts)))
arr('uint16_t', 'part_price_b', [p[1] + 1 for p in parts])        # +1: evita o padrao de bytes 32 00 50 (confunde o detector de mapper); use PART_PRICE(i)
cars.append('#define PART_PRICE(i) (part_price_b[i] - 1)')
arr('uint8_t', 'part_cat', [p[2] for p in parts])
arr('uint8_t', 'part_mk', [p[3] for p in parts]); arr('uint8_t', 'part_var', [p[4] for p in parts])
cars.append('static const char car_name[%d][28] = {%s};' % (NCARS, ','.join('"%s"' % n for n in names)))
cars.append('static const char pil_name[%d][11] = {%s};' % (len(pilots), ','.join('"%s"' % p[0] for p in pilots)))
arr('uint8_t', 'pil_a', [p[1] for p in pilots]); arr('uint8_t', 'pil_b', [p[2] for p in pilots])
open('cars.h', 'w').write('#include <stdint.h>\n' + '\n'.join(cars) + '\n')
open('tables.h', 'w').write('\n'.join(out) + '\n')
print('tables.h ok; curve range', min(curve), max(curve), 'hill range', min(hill), max(hill))
