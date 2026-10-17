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
pal = [(0,0,0), (2,5,7), (0,3,0), (1,5,0), (2,2,2), (3,3,3), (6,0,0), (7,7,7),
       (7,7,0), (6,1,1), (2,0,0), (1,1,1), (7,7,7), (0,6,0), (0,0,0), (4,4,4)]
flat = []
for c in pal: flat += list(rgb(*c))
out.append('const uint8_t palette[32] = {%s};' % ','.join(map(str, flat)))
open('tables.h', 'w').write('\n'.join(out) + '\n')
print('tables.h ok; curve range', min(curve), max(curve), 'hill range', min(hill), max(hill))
