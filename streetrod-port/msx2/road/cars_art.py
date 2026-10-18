"""Carros desenhados na garagem (LIB2): corpo lateral de cada carro (item BODY[figura-1]) + 2 rodas (itens WHEEL[tipo de pneu]).
As imagens usam a paleta EGA propria da cena: indice 0 = transparente, 6 = cor da pintura, 7 = reflexo da pintura, 1 = preto (contorno).
Largura reduzida a 4/5 (descarta 1 coluna em 5, como a cena da garagem 320 -> 256). Posicao das rodas: centro dos vaos (detectados nas arcadas)."""
import os, sys
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
import srlib

BODY = [39, 40, 47, 48, 53, 54, 60, 61, 112, 113, 118, 119, 126, 127, 131, 135, 145, 149, 156, 157, 164, 165, 171, 172, 179, 201]   # figura 1..24, 25 = 179, 26 = 201 (Corvette)
WHEEL = [28, 65, 106]                       # pneu de fabrica (faixa branca), cromado, slick
MANUAL = {61: (70, 192, 40), 135: (50, 186, 47), 126: (34, 158, 51)}    # item: (centro x da roda dianteira, traseira, centro y) onde a deteccao automatica falha
WH_H = 28

def _img(it):
    p = np.frombuffer(it.decode(), np.uint8)
    p = p[:4 * it.h * ((it.w + 7) // 8)].reshape(4, it.h, (it.w + 7) // 8)
    b = np.unpackbits(p, axis=2)[:, :, :it.w]
    return (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)

def _scale(a):
    return a[:, [x for x in range(a.shape[1]) if x % 5 != 4]]

def _wells(a, minrun=16):
    h = a.shape[0]
    for r in range(h - 16, h - 5):
        row = a[r] != 0
        xs = np.nonzero(row)[0]
        if len(xs) == 0: continue
        runs = []; s = None
        for x in range(xs[0], xs[-1] + 1):
            if not row[x] and s is None: s = x
            if row[x] and s is not None:
                if x - s >= minrun: runs.append((s, x - 1))
                s = None
        if len(runs) == 2: return r, runs
    return None

def load(lib2):
    items = srlib.load(lib2)
    cars = []
    for k in BODY:
        a = _scale(_img(items[k])); h, w = a.shape
        if k in MANUAL: cx1, cx2, cy = MANUAL[k]
        else:
            r, runs = _wells(a) or _wells(a, 12) or (None, None)
            assert r is not None, k
            cx1, cx2, cy = (runs[0][0] + runs[0][1]) // 2, (runs[1][0] + runs[1][1]) // 2, r
        cars.append(dict(item=k, body=a, w=w, h=h, wx=(cx1, cx2), wy=cy))
    wheels = [_scale(_img(items[k])) for k in WHEEL]
    return cars, wheels

def compose(c, wheel, pal):
    """imagem RGB do carro com as rodas (para conferencia visual)"""
    a = c['body']; h, w = a.shape; cv = np.zeros((h + 10, w), np.uint8)
    wh = wheel; wh_h, wh_w = wh.shape
    for cx in c['wx']:
        for y in range(wh_h):
            for x in range(wh_w):
                Y = c['wy'] - wh_h // 2 + y; X = cx - wh_w // 2 + x
                if wh[y, x] and 0 <= Y < cv.shape[0] and 0 <= X < w: cv[Y, X] = wh[y, x]
    cv[:h][a != 0] = a[a != 0]
    return np.array(pal, np.uint8)[cv]
