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
FRONT_BACK = 4
REAR_FWD = 4

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

def _arch(a, cx, cy, thr=4):
    """largura da arcada: por coluna, o 'teto' = primeiro pixel do corpo subindo a partir da base da roda; a arcada e' o trecho de colunas cujo teto fica acima do centro da roda
    (+thr). Devolve o centro x do trecho que contem cx (ou None se a medida nao for plausivel: largura fora de 22..52 ou deslocamento > 10 px)."""
    h, w = a.shape; base = min(h - 1, cy + 14)
    ok = np.zeros(w, bool)
    for x in range(max(0, cx - 34), min(w, cx + 35)):
        for y in range(base, -1, -1):
            if a[y, x] != 0: ok[x] = y <= cy + thr; break
        else: ok[x] = True
    if not ok[cx]:
        c = [xx for xx in range(max(0, cx - 12), min(w, cx + 13)) if ok[xx]]
        if not c: return cx
        x = min(c, key=lambda v: abs(v - cx))
    else: x = cx
    l = r = x
    while l - 1 >= 0 and ok[l - 1]: l -= 1
    while r + 1 < w and ok[r + 1]: r += 1
    m = (l + r) // 2
    return m if 22 <= r - l + 1 <= 52 and abs(m - cx) <= 10 else cx

def arch_centers(a):
    """centro x das cavas das rodas pelo contorno inferior do corpo: as colunas onde a ultima linha opaca sobe mais de 7 px acima da soleira; centro = media das colunas do topo da cava
    (ate 4 px abaixo do apice). Devolve [(x, apice)] ordenado da traseira (esquerda) para a dianteira (direita)"""
    h, w = a.shape; yl = np.full(w, -1)
    for x in range(w):
        nz = np.nonzero(a[:, x])[0]
        if len(nz): yl[x] = nz.max()
    valid = yl >= 0
    thr = np.percentile(yl[valid], 85) - 7
    ivs, s0 = [], None
    for x in range(w):
        up = valid[x] and yl[x] <= thr
        if up and s0 is None: s0 = x
        if not up and s0 is not None: ivs.append((s0, x - 1)); s0 = None
    if s0 is not None: ivs.append((s0, w - 1))
    m = []
    for iv in ivs:
        if m and iv[0] - m[-1][1] <= 3: m[-1] = (m[-1][0], iv[1])
        else: m.append(iv)
    out = []
    for lo, hi in m:
        if hi - lo < 14: continue
        seg = yl[lo:hi + 1]; ap = int(seg.min()); xs = np.nonzero(seg <= ap + 4)[0] + lo
        out.append((int(round(xs.mean())), ap))
    return out


def top_row(a):
    """primeira linha larga (>= 12 pixels opacos): ignora antenas"""
    nz = (a != 0).sum(axis=1)
    return int(np.nonzero(nz >= 12)[0][0])


def driver_window(a, wx0, wx1):
    """janela do motorista: o transparente cercado pelo corpo (vidros) na metade de cima, escolhendo a maior janela cujo centro cai entre 30% e 80% do entre-eixos;
    devolve (x, y) do centro (a cabeca fica la), ou None"""
    h, w = a.shape; out = np.zeros((h, w), bool); st = []
    for x in range(w): st += [(0, x), (h - 1, x)]
    for y in range(h): st += [(y, 0), (y, w - 1)]
    while st:
        y, x = st.pop()
        if 0 <= y < h and 0 <= x < w and not out[y, x] and a[y, x] == 0:
            out[y, x] = True; st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    enc = (a == 0) & ~out; seen = np.zeros_like(enc); best = None
    for y0 in range(h):
        for x0 in range(w):
            if enc[y0, x0] and not seen[y0, x0]:
                pts = []; st = [(y0, x0)]
                while st:
                    y, x = st.pop()
                    if not (0 <= y < h and 0 <= x < w) or seen[y, x] or not enc[y, x]: continue
                    seen[y, x] = True; pts.append((y, x)); st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
                ys = [p[0] for p in pts]; xs = [p[1] for p in pts]
                cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
                if len(pts) >= 30 and cy < h * 0.6 and wx0 + 0.30 * (wx1 - wx0) <= cx <= wx0 + 0.80 * (wx1 - wx0):
                    if best is None or len(pts) > best[0]: best = (len(pts), int(round(cx)), int(round(cy)))
    return None if best is None else best[1:]


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
            cx1, cx2 = _arch(a, cx1, cy), _arch(a, cx2, cy) - FRONT_BACK       # a dianteira (lado direito) um pouco para tras do centro medido (ajuste visual)
        wx0, wx1 = cx1 + REAR_FWD, cx2
        ar = arch_centers(a)                                   # centros das cavas medidos no contorno: tem prioridade sobre a deteccao antiga
        if len(ar) >= 2: wx0, wx1 = ar[0][0], ar[-1][0]
        elif len(ar) == 1 and ar[0][0] > w // 2: wx1 = ar[0][0]; wx0 = wx1 - (cx2 - cx1 - REAR_FWD)       # so a dianteira aparece (saia no traseiro): mantem o entre-eixos antigo
        if len(ar) >= 1 and k not in MANUAL: cy = ar[-1][1] + 13                                          # topo da roda logo abaixo do apice da cava dianteira
        cars.append(dict(item=k, body=a, w=w, h=h, wx=(wx0, wx1), wy=cy, head=driver_window(a, wx0, wx1) or (int(wx0 + 0.48 * (wx1 - wx0)), top_row(a) + 10)))          # traseira: REAR_FWD px para a frente (todos os carros, inclusive os manuais)
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

def fillwin(a):
    """vidros: o transparente (0) cercado pelo corpo (nao ligado a borda da imagem: as cavas das rodas ficam abertas embaixo) vira preto opaco (1);
    senao o cenario (Bob's: lanchonete, poste) aparecia pelas janelas do carro"""
    h, w = a.shape; out = np.zeros((h, w), bool); st = []
    for x in range(w): st += [(0, x), (h - 1, x)]
    for y in range(h): st += [(y, 0), (y, w - 1)]
    while st:
        y, x = st.pop()
        if 0 <= y < h and 0 <= x < w and not out[y, x] and a[y, x] == 0:
            out[y, x] = True; st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    b = a.copy(); b[(a == 0) & ~out] = 1
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
