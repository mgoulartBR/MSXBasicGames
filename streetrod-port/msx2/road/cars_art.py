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

def _refine(a, cx, cy, wh_w=32, wh_h=28):
    """centraliza a roda no vao da arcada: flood-fill dos pixels transparentes a partir do centro (limitado a +-24 px e ate a base da roda) -> topo (apice) e extensao
    horizontal a meia altura da roda; centro x = meio do vao, centro y = apice + raio"""
    h, w = a.shape; r = wh_h // 2
    bottom = min(h - 1, cy + r)
    seen = set(); st = [(cy, cx)]; reg = []
    if a[cy, cx] != 0:                                     # semente dentro do corpo: procura o vazio mais proximo na mesma linha
        for d in range(1, 12):
            for xx in (cx - d, cx + d):
                if 0 <= xx < w and a[cy, xx] == 0: st = [(cy, xx)]; break
            else: continue
            break
    while st:
        y, x = st.pop()
        if (y, x) in seen or not (0 <= y <= bottom and 0 <= x < w) or abs(x - cx) > 24 or a[y, x] != 0: continue
        seen.add((y, x)); reg.append((y, x)); st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    if not reg: return cx, cy
    ytop = min(y for y, x in reg)
    row = [x for y, x in reg if y == min(bottom, ytop + r - 2)]
    if not row: return cx, cy
    return (min(row) + max(row)) // 2, min(bottom, ytop + r + 1) if ytop + r + 1 <= bottom + 3 else cy

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
        if k not in MANUAL:
            ox1, ox2, oy = cx1, cx2, cy
            (cx1, cy1), (cx2, cy2) = _refine(a, cx1, cy), _refine(a, cx2, cy)
            clamp = lambda v, o, d: max(o - d, min(o + d, v))                 # a refinacao so ajusta: no maximo +-8 px em x e -1..+3 em y
            cx1, cx2 = clamp(cx1, ox1, 8), clamp(cx2, ox2, 8)
            cy = oy                                                       # vertical mantido (so o x e' recentralizado)
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


STICKER = [186, 187, 188, 189, 190, 191, 192, 193]      # banana club, explosive, (estrela), N.E.D., veteran, for sale, aguia, USA 1

def chop(a, wy):
    """teto rebaixado: tira k linhas dos montantes (entre a capota e a linha da cintura). Devolve (imagem, wy novo)."""
    h, w = a.shape
    nz = (a != 0).sum(axis=1)
    top = int(np.nonzero(nz)[0][0])
    belt = top + 1
    while belt < h - 1 and nz[belt] < 0.5 * w: belt += 1                     # primeira linha larga: capo/porta-malas
    cap = top + 1
    while cap < belt and nz[cap] > 0.6 * max(nz[top:top + 4]): cap += 1      # fim da capota
    span = belt - cap
    k = max(2, min(7, span // 2))
    cut = cap + (span + k) // 2
    return np.concatenate([a[:cut - k], a[cut:]], axis=0), wy - k

def strip(a):
    """sem para-choques: apaga o cromado (cinzas/branco) nas pontas, na parte baixa"""
    h, w = a.shape; b = a.copy()
    nz = (a != 0).sum(axis=1); top = int(np.nonzero(nz)[0][0])
    y0 = top + int(0.5 * (h - top)); e = max(10, w // 9)
    for x in list(range(0, e)) + list(range(w - e, w)):
        for y in range(y0, h):
            if b[y, x] in (2, 3, 4, 5): b[y, x] = 0
    return b

def variants(c):
    """4 corpos por carro: v = teto_rebaixado + 2*sem_para_choques -> (imagem, wy)"""
    out = []
    for v in range(4):
        a, wy = c['body'], c['wy']
        if v & 2: a = strip(a)
        if v & 1: a, wy = chop(a, wy)
        out.append((a, wy))
    return out

def stickers(lib2):
    items = srlib.load(lib2)
    return [_scale(_img(items[k])) for k in STICKER]
