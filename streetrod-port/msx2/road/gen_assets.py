#!/usr/bin/env python3
"""Gera os assets graficos (cockpit etc.) a partir de LIB2 -> assets.bin (vai em 0x8000 da ROM) + assets.h.
Uso: LIB2=<caminho> ./gen_assets.py   (sem LIB2, gera assets vazios e o jogo roda sem cockpit).
Formato LZ4-like por faixas de 4096 bytes: token(lit<<4|mlen-4) [ext lit] literais off16 [ext mlen]; off16==0 encerra."""
import os, struct, sys
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))

def lz_encode(d):
    out = bytearray(); n = len(d); i = 0; lit0 = 0
    tab = {}
    def emit(lit_start, lit_end, off, mlen):
        ll = lit_end - lit_start
        tok = (min(ll, 15) << 4) | (min(mlen - 4, 15) if off else 0)
        out.append(tok)
        if ll >= 15:
            r = ll - 15
            while r >= 255: out.append(255); r -= 255
            out.append(r)
        out.extend(d[lit_start:lit_end])
        out.extend(bytes([off & 255, off >> 8]))
        if off and mlen - 4 >= 15:
            r = mlen - 4 - 15
            while r >= 255: out.append(255); r -= 255
            out.append(r)
    while i < n:
        best = (0, 0)
        if i + 4 <= n:
            for j in reversed(tab.get(bytes(d[i:i+4]), [])[-48:]):
                l = 0
                while i + l < n and d[j + l] == d[i + l]: l += 1
                if l > best[0]: best = (l, i - j)
                if l > 255: break
        if best[0] >= 4:
            emit(lit0, i, best[1], best[0])
            for k in range(i, i + best[0]):
                if k + 4 <= n: tab.setdefault(bytes(d[k:k+4]), []).append(k)
            i += best[0]; lit0 = i
        else:
            if i + 4 <= n: tab.setdefault(bytes(d[i:i+4]), []).append(i)
            i += 1
    emit(lit0, n, 0, 0)
    return bytes(out)

def lz_decode(s):
    out = bytearray(); i = 0
    while True:
        t = s[i]; i += 1; ll = t >> 4
        if ll == 15:
            while True:
                b = s[i]; i += 1; ll += b
                if b != 255: break
        out += s[i:i+ll]; i += ll
        off = s[i] | s[i+1] << 8; i += 2
        if off == 0: return bytes(out)
        ml = (t & 15) + 4
        if (t & 15) == 15:
            while True:
                b = s[i]; i += 1; ml += b
                if b != 255: break
        for _ in range(ml): out.append(out[-off])

# paleta do jogo (tem de bater com gen_tables.py): usada para mapear as cores EGA do original
EGA = [(0,0,0),(0,0,0),(85,85,85),(170,170,170),(255,255,255),(170,170,170),(0,0,0),(85,255,255),
       (0,170,170),(0,170,0),(170,0,0),(255,85,85),(170,0,170),(85,255,85),(170,85,0),(255,255,85)]
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette_def
def msx_rgb(c): return tuple(round(v * 255 / 7) for v in c)
MSX = [msx_rgb(c) for c in palette_def.PAL]
def nearest(rgb):
    return min(range(16), key=lambda k: sum((a - b) ** 2 for a, b in zip(rgb, MSX[k])))
MAP = [nearest(c) for c in EGA]

def cockpit_pixels(lib2):
    import srlib
    it = srlib.load(lib2)[14]
    p = np.frombuffer(it.decode(), np.uint8).reshape(4, 190, 40)
    b = np.unpackbits(p, axis=2)[:, :, :320]
    idx = (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)
    idx = idx[:, [x for x in range(320) if x % 5 != 4]]      # 320 -> 256 colunas (descarta 1 em 5: preserva o dither)
    return np.array(MAP, np.uint8)[idx]

STRIP = 4096
WIN_Y, SKY_TOP = 24, 22
VIEW_BOT = int(os.environ.get('VIEW_BOT', '95'))       # altura da janela da estrada (igual ao -DVIEW_BOT do build)
STRIP_TOL = 1
ARC_X0 = 66; ARC_XL = 54; DIAG_SLOPE = 0.67      # ARC_XL: a diagonal do capo e prolongada ate a base da coluna A
# a partir daqui o recorte segue o capo (antes ficam a coluna A e a porta)
GLASS_MODE = os.environ.get('GLASS_MODE', 'dash')    # flat: vidro termina em y=GLASS_BOT; dash: segue o topo do painel (1o pixel claro por coluna)
GLASS_BOT = WIN_Y + VIEW_BOT              # y da tela onde o vidro termina (a estrada nao e desenhada abaixo)     # janela da estrada (tem de bater com build.sh: VIEW_BOT)
OV_SY = 702                                # sobreposicao do cockpit (so as partes opacas) na pagina 2, abaixo do cockpit limpo
SPR_W = [8, 11, 15, 22, 30, 42, 64]      # larguras das versoes reduzidas do carro (visto por tras)
TREE_Y, SIGA_Y, SIGB_Y, SIGC_Y = 212, 468, 704, 734            # cenario da corrida: arvores/predios (pagina 0, linhas 212-255), placas A (pagina 1, 468-511), B e C (pagina 2, 704-767)
RACE_X0 = 0                                                   # a estrada (por quadro) comeca nesta coluna; a esquerda e estatica
BG_Y, BG_H = 768, 15                                         # colinas + grama do horizonte (item 267 da LIB2) na pagina 3 (corrida), copiadas por cima do ceu a cada quadro
SPR_Y0 = 808; SPR_STRIDE = 40; REAR_ITEMS = (11, 249, 57)                                     # a folha de sprites fica na pagina 3 da VRAM

def overlay(a):
    """Parte do cockpit que fica POR CIMA da estrada: vidro (preto ligado ao para-brisa e a janela lateral) vira transparente (0);
    pretos de outras partes viram cor 11."""
    y0, y1 = WIN_Y + SKY_TOP, WIN_Y + VIEW_BOT
    sub = a[y0:y1]
    h, w = sub.shape
    glass = np.zeros((h, w), bool)
    st = [(60 - y0, 160), (70 - y0, 25)]
    while st:
        y, x = st.pop()
        if 0 <= y < h and 0 <= x < w and not glass[y, x] and sub[y, x] == 0:
            glass[y, x] = True; st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    cuts = None
    # (o preto da janela lateral, inclusive abaixo do retrovisor, e vidro: mostra a estrada; o interior do retrovisor lateral (x 7-33, y 101-125 da tela) continua opaco)
    glass[101 - y0:, 7:34] = False
    if GLASS_MODE == 'flat':
        glass[GLASS_BOT - y0:, 54:] = False         # capo/painel: abaixo do para-brisa (linhas ciano) tudo e cockpit opaco
    else:
        # contorno do capo: a linha ciano do capo (diagonal a esquerda, quase reta a direita) -- a estrada termina logo acima dela
        # e o capo (preto com as linhas ciano) e o painel ficam intactos abaixo
        XA = ARC_X0
        cy = np.full(w, np.nan)
        for x in range(XA, w):
            hit = np.nonzero(sub[88 - y0:, x] == 1)[0]
            if len(hit): cy[x] = 88 - y0 + hit[0]
        ok = np.nonzero(~np.isnan(cy))[0]
        cut = np.interp(np.arange(w), ok, cy[ok]) - 1            # interpola os vaos entre tracos
        sm = cut.copy()
        for x in range(XA, w):
            win = np.sort(cut[max(XA, x - 3):x + 4]); sm[x] = win[len(win) // 2]
        for x in range(ARC_XL, XA): sm[x] = min(h - 1, sm[XA] + DIAG_SLOPE * (XA - x))       # prolonga a diagonal ate a coluna A
        sm = np.rint(sm).astype(int)
        cuts = sm.copy(); x0 = ARC_XL
        while x0 < w:                               # faixas adaptativas: a faixa segue a curva com tolerancia de STRIP_TOL px
            x1 = x0 + 2; lo = hi = sm[x0]
            while x1 < w and max(hi, sm[x1], sm[x1 + 1]) - min(lo, sm[x1], sm[x1 + 1]) <= STRIP_TOL:   # passos de 2 colunas: HMMM exige x e largura pares
                lo = min(lo, sm[x1], sm[x1 + 1]); hi = max(hi, sm[x1], sm[x1 + 1]); x1 += 2
            lo = min(lo, sm[x0 + 1])
            cuts[x0:x1] = lo; x0 = x1
        if os.environ.get('DBG_CUTS'): print(list(zip(range(XA, w), cuts[XA:], sm[XA:]))[50:90], file=sys.stderr)
        for x in range(ARC_XL, w): glass[cuts[x]:, x] = False
    out = np.where(sub == 0, 11, sub).astype(np.uint8); out[glass] = 0
    return out, cuts, (sm if GLASS_MODE != 'flat' else None)

def overlay_rects(ov, band=2, gap=4):
    """Retangulos (x, y, w, h) que cobrem os pixels opacos do overlay, em faixas de `band` linhas (x e w pares, para HMMM)."""
    h, w = ov.shape; rects = []
    for y0 in range(0, h, band):
        m = (ov[y0:y0 + band] != 0).any(axis=0); x = 0; runs = []
        while x < w:
            if m[x]:
                s = x
                while x < w and m[x]: x += 1
                runs.append([s, x])
            else: x += 1
        merged = []
        for r in runs:
            if merged and r[0] - merged[-1][1] <= gap: merged[-1][1] = r[1]
            else: merged.append(r)
        for s, e in merged:
            s &= ~1; e = (e + 1) & ~1
            if rects and rects[-1][1] == y0 and rects[-1][0] == s and rects[-1][2] == e - s: rects[-1][3] += min(band, h - y0)
            else: rects.append([s, y0, e - s, min(band, h - y0)])
    return rects

LIGHT_Y = 940
NEWS_BODY_TOP = [(16, 30), (30, 30), (16, 16), (30, 30), (30, 30)]   # linha (na faixa de 54) onde comeca o corpo do texto de cada coluna, por manchete
CAL_X0 = 60; CAL_Y0 = 20
EGA16 = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),(85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
def lib1_screen(path, k):
    import srlib
    it = srlib.load(path)[k]
    p = np.frombuffer(it.decode(), np.uint8)[:4 * it.h * ((it.w + 7) // 8)].reshape(4, it.h, (it.w + 7) // 8)
    b = np.unpackbits(p, axis=2)[:, :, :it.w]
    a = (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)
    a = a[:, [x for x in range(it.w) if x % 5 != 4]]
    out = np.zeros((212, 256), np.uint8)
    y0 = (212 - a.shape[0]) // 2; x0 = (256 - a.shape[1]) // 2 & ~1
    out[y0:y0 + a.shape[0], x0:x0 + a.shape[1]] = a[:212, :256]
    return out
HEAD_ITEMS = [194, 195, 196, 197, 198, 199] + list(range(202, 218))
def bob_raw(lib2, k):
    import srlib
    it = srlib.load(lib2)[k]
    p = np.frombuffer(it.decode(), np.uint8)[:4 * it.h * ((it.w + 7) // 8)].reshape(4, it.h, (it.w + 7) // 8)
    b = np.unpackbits(p, axis=2)[:, :, :it.w]
    a = (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)
    return a[:, [x for x in range(it.w) if x % 5 != 4]]
CAR_Y0 = 852; WHEEL_Y0 = 920; STK_Y0 = 948; BAND_COPY_Y = 768      # pagina 3 na garagem: copia limpa da faixa do chao, corpo e roda de rascunho
EGA_PAL = [(0,0,0),(0,0,0),(85,85,85),(170,170,170),(255,255,255),(170,170,170),(0,0,0),(85,255,255),
           (0,170,170),(0,170,0),(170,0,0),(255,85,85),(170,0,170),(85,255,85),(170,85,0),(255,255,85)]
def garage_pixels(lib2, k=32):
    import srlib
    it = srlib.load(lib2)[k]
    p = np.frombuffer(it.decode(), np.uint8).reshape(4, it.h, (it.w + 7) // 8)
    b = np.unpackbits(p, axis=2)[:, :, :it.w]
    idx = (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)
    return idx[:, [x for x in range(it.w) if x % 5 != 4]]

def garage_full(lib2, k):
    """Item da LIB2 em colunas originais (320 px), sem remover a 5a coluna."""
    import srlib
    it = srlib.load(lib2)[k]
    p = np.frombuffer(it.decode(), np.uint8).reshape(4, it.h, (it.w + 7) // 8)
    b = np.unpackbits(p, axis=2)[:, :, :it.w]
    return (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)

def garage_pixels_thin(lib2, k):
    """Como garage_pixels, mas a coluna removida (x % 5 == 4) empresta seus pixels a coluna anterior se ela estiver vazia: preserva fios de 1 pixel."""
    f = garage_full(lib2, k).copy()
    for x in range(4, f.shape[1], 5):
        e = (f[:, x - 1] == 0) & (f[:, x] != 0)
        f[e, x - 1] = f[e, x]
    return f[:, [x for x in range(f.shape[1]) if x % 5 != 4]]

def light_sheet(lib2):
    """Semaforo (item 13: duas cabecas de 24x32, lampada vermelha na da direita, verde na da esquerda) em 3 estados: apagado, vermelho, verde."""
    import srlib
    it = srlib.load(lib2)[13]
    p = np.frombuffer(it.decode(), np.uint8).reshape(4, it.h, (it.w + 7) // 8)
    b = np.unpackbits(p, axis=2)[:, :, :it.w]
    idx = (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)
    L, R = idx[:, :24].copy(), idx[:, 24:].copy()
    green = np.isin(L, (9, 13)); red = np.isin(R, (10, 11))
    dark = L.copy(); dark[green] = 1
    redh = dark.copy(); redh[red] = R[red]
    states = [dark, redh, L]
    sheet = np.zeros((32, 256), np.uint8)
    for k, h in enumerate(states):
        m = np.array(MAP, np.uint8)[np.hstack([h, h])]
        sheet[:, k * 48:k * 48 + 48] = m
    return sheet

def rear_sprite(lib2, item):
    """Carro visto por tras (LIB2): fundo (preto ligado a borda) vira transparente (0); pretos internos viram cor 11 (quase preto)."""
    import srlib
    it = srlib.load(lib2)[item]
    bw = (it.w + 7) // 8
    p = np.frombuffer(it.decode(), np.uint8).reshape(4, it.h, bw)
    b = np.unpackbits(p, axis=2)[:, :, :it.w]
    idx = (b[0] | (b[1] << 1) | (b[2] << 2) | (b[3] << 3)).astype(np.uint8)
    col = np.array(MAP, np.uint8)[idx]
    h, w = col.shape
    solid = col != 0
    pad = np.pad(solid, 2)
    thick = np.zeros_like(solid)
    for dy in range(5):
        for dx in range(5): thick |= pad[dy:dy + h, dx:dx + w]       # fecha frestas de ate ~4 px no contorno antes de preencher o fundo
    bg = np.zeros((h, w), bool); st = [(y, x) for y in range(h) for x in (0, w - 1)] + [(y, x) for x in range(w) for y in (0, h - 1)]
    while st:
        y, x = st.pop()
        if 0 <= y < h and 0 <= x < w and not bg[y, x] and not thick[y, x]:
            bg[y, x] = True; st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    col = np.where(col == 0, 11, col).astype(np.uint8); col[bg & ~solid] = 0
    return col

def scale_sprite(col, w):
    h0, w0 = col.shape; h = max(2, round(h0 * w / w0))
    out = np.zeros((h, w), np.uint8)
    for y in range(h):
        for x in range(w):
            blk = col[y * h0 // h:max(y * h0 // h + 1, (y + 1) * h0 // h), x * w0 // w:max(x * w0 // w + 1, (x + 1) * w0 // w)].ravel()
            if (blk == 0).mean() >= 0.5: continue
            v = blk[blk != 0]
            out[y, x] = np.bincount(v, minlength=16).argmax()
    return out

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ABASE = int(os.environ.get('ASSET_BANK0', '6'))     # primeiro banco de assets (antes dele: nucleo + bancos de codigo)
    banks = [bytearray()]; ents = []       # banco de 16 KB por vez (a partir do banco 2 da ROM); ents: (banco, endereco 0x8000+, vaddr, paginas_256, copias)
    lib2 = os.environ.get('LIB2')
    def add(raw, vaddr, copies, group=0):
        for s in range(0, len(raw), STRIP):
            chunk = bytes(raw[s:s + STRIP]); chunk += bytes(-len(chunk) % 256)
            enc = lz_encode(chunk); assert lz_decode(enc) == chunk
            if len(banks[-1]) + len(enc) > 0x4000: banks.append(bytearray())     # faixa nunca cruza dois bancos
            ents.append((len(banks) - 1 + ABASE, group, 0x8000 + len(banks[-1]), vaddr + s, len(chunk) // 256, copies)); banks[-1].extend(enc)
    sprh = ''; orects = []; carh = ''; headh = ''; parth = ''; scnh = ''
    if lib2:
        a = cockpit_pixels(lib2)
        np.save(os.path.join(here, 'cockpit_px.npy'), a)
        ov, cuts, sm = overlay(a); np.save(os.path.join(here, 'overlay_px.npy'), ov)
        if sm is not None:                          # completa o contorno ciano do capo (no original e tracejado): linha continua de 1 px
            a = a.copy()
            for x in range(ARC_XL, 256):
                y = WIN_Y + SKY_TOP + int(sm[x])
                if a[y, x] in (0, 1): a[y, x] = 1          # nao pinta sobre a coluna A
        add((a[:, 0::2] << 4 | a[:, 1::2]).astype(np.uint8).tobytes(), 0, 3)    # copias: paginas 0, 1 e 2 (limpa, para restaurar)
        orects = overlay_rects(ov[:, :ARC_XL] if cuts is not None else ov, band=8, gap=4)    # moldura da esquerda (porta/coluna A): retangulos HMMM justos
        if cuts is not None:                                                             # painel/capo: uma faixa vertical por STRIP_W colunas
            x0 = ARC_XL
            while x0 < 256:
                x1 = x0 + 2
                while x1 < 256 and cuts[x1] == cuts[x0]: x1 += 2
                if cuts[x0] < ov.shape[0]: orects.append([x0, int(cuts[x0]), x1 - x0, ov.shape[0] - int(cuts[x0])])
                x0 = x1
        g = garage_pixels(lib2)                                   # cena da garagem (item 32): grupo 1, so na pagina 0, paleta EGA propria
        add((g[:, 0::2] << 4 | g[:, 1::2]).astype(np.uint8).tobytes(), 0, 3, group=1)
        import cars_art                                            # carros da garagem: corpo (grupo 2+id) e rodas (grupo 28+tipo), na area de rascunho da pagina 3
        carart, wheelart = cars_art.load(lib2)
        def packrows(a, wpx):
            raw = np.zeros((a.shape[0], 256), np.uint8); raw[:, :a.shape[1]] = a
            return (raw[:, 0::2] << 4 | raw[:, 1::2]).astype(np.uint8).tobytes()
        varart = [cars_art.variants(c) for c in carart]              # 4 corpos por carro: v = teto rebaixado + 2 * sem para-choques
        for i, c in enumerate(carart):
            for v in range(4): add(packrows(varart[i][v][0], c['w']), CAR_Y0 * 128, 1, group=2 + i if v == 0 else 100 + i * 3 + v - 1)
        for t, wh in enumerate(wheelart): add(packrows(wh, wh.shape[1]), WHEEL_Y0 * 128, 1, group=28 + t)
        stkart = cars_art.stickers(lib2)
        for s_, st in enumerate(stkart): add(packrows(st, st.shape[1]), STK_Y0 * 128, 1, group=180 + s_)
        def best_rects(arr, nmax=10):                               # animacao do carro: o corpo vira faixas de linhas: retangulos opacos (HMMM no miolo) e faixas "planas" com as colunas ocupadas (LMMM)
            H, Wd = arr.shape
            runs = []; bounds = []
            for row in arr:
                best = (0, 0); cur = 0
                for bx in range((len(row) + 1) // 2):
                    op = bx * 2 + 1 < len(row) and row[bx * 2] != 0 and row[bx * 2 + 1] != 0
                    cur = cur + 1 if op else 0
                    if cur > best[1]: best = (bx - cur + 1, cur)
                runs.append(best)
                nz = np.nonzero(row)[0]
                bounds.append((int(nz[0]), int(nz[-1]) + 1) if len(nz) else None)
            for tol in (6, 10, 16, 24, 40, 80):
                segs = []; r = 0
                while r < H:
                    if runs[r][1] >= 16:
                        s, e = runs[r][0], runs[r][0] + runs[r][1]; r2 = r + 1
                        while r2 < H and runs[r2][1] >= 16:
                            ns, ne = max(s, runs[r2][0]), min(e, runs[r2][0] + runs[r2][1])
                            if ne - ns < 16 or any(runs[k][1] - (ne - ns) > tol for k in range(r, r2 + 1)): break
                            s, e = ns, ne; r2 += 1
                        if r2 - r >= 3: segs.append((r, r2 - r, 1, s * 2, e * 2)); r = r2; continue
                    r2 = r + 1                                       # faixa plana: linhas ate o proximo retangulo
                    while r2 < H and not (runs[r2][1] >= 16 and r2 + 3 <= H and all(runs[k][1] >= 16 for k in range(r2, r2 + 3))): r2 += 1
                    bb = [bounds[k] for k in range(r, r2) if bounds[k]]
                    c1, c2 = (min(x[0] for x in bb) & ~1, (max(x[1] for x in bb) + 1) & ~1) if bb else (0, 0)
                    segs.append((r, r2 - r, 0, c1, c2)); r = r2
                if len(segs) <= nmax: break
            out = []
            for (r, h, flag, c1, c2) in segs:
                if flag or c2 > c1: out.append((r, h | (0x80 if flag else 0), c1, c2))
            out = out[:nmax]
            return out + [(0, 0, 0, 0)] * (nmax - len(out))
        rect_blob = bytes(v for vv in range(4) for i in range(len(carart)) for q in best_rects(varart[i][vv][0]) for v in q)    # 40 bytes por (variante, carro): indice cvar * ncarros + id
        flat = lambda f: ','.join(str(f(i, v)) for v in range(4) for i in range(len(carart)))
        carh = ('#define CAR_Y0 %d\n#define WHEEL_Y0 %d\n#define STK_Y0 %d\n#define BAND_COPY_Y %d\n#define WH_W %d\n#define WH_H %d\n'
                'static const uint8_t car_w[%d] = {%s};\nstatic const uint8_t car_h[%d] = {%s};\nstatic const uint8_t car_wx0[%d] = {%s};\n'
                'static const uint8_t car_wx1[%d] = {%s};\nstatic const uint8_t car_wy[%d] = {%s};\n'
                'static const uint8_t stk_w[8] = {%s};\nstatic const uint8_t stk_h[8] = {%s};\n') % (
            CAR_Y0, WHEEL_Y0, STK_Y0, BAND_COPY_Y, wheelart[0].shape[1], wheelart[0].shape[0],
            len(carart), ','.join(str(c['w']) for c in carart), 4 * len(carart), flat(lambda i, v: varart[i][v][0].shape[0]),
            len(carart), ','.join(str(c['wx'][0]) for c in carart),
            len(carart), ','.join(str(c['wx'][1]) for c in carart), 4 * len(carart), flat(lambda i, v: varart[i][v][1]),
            ','.join(str(st.shape[1]) for st in stkart), ','.join(str(st.shape[0]) for st in stkart))
        lic = garage_pixels(lib2, 16)                            # carteira DMV (item 16): grupo 244, no alto da tela (o teclado do nome fica embaixo)
        canvas = np.zeros((212, 256), np.uint8); lx0 = ((256 - lic.shape[1]) // 2) & ~1
        canvas[:lic.shape[0], lx0:lx0 + lic.shape[1]] = lic
        add((canvas[:, 0::2] << 4 | canvas[:, 1::2]).astype(np.uint8).tobytes(), 0, 3, group=244)
        news = garage_pixels(lib2, 21)                           # jornal "Los Angeles News" (item 21: AUTO PARTS / USED CARS / titulo) + manchete do dia (itens 22-26)
        add((news[:, 0::2] << 4 | news[:, 1::2]).astype(np.uint8).tobytes(), 0, 3, group=190)
        for k_ in range(5):
            hl = garage_pixels(lib2, 22 + k_).copy()
            for c_, top in enumerate(NEWS_BODY_TOP[k_]):          # apaga o "texto" ilegivel (rabiscos) de cada coluna; o codigo reescreve com a fonte da garagem
                hl[top:54, 0 if c_ == 0 else 128:128 if c_ == 0 else 256] = 4
                hl[top:54, 127] = 1                                # filete entre as colunas
            add((hl[:, 0::2] << 4 | hl[:, 1::2]).astype(np.uint8).tobytes(), news.shape[0] * 128, 3, group=191 + k_)
        bobs = garage_pixels(lib2, 93)                           # Bob's Drive-In (item 93); os carros pintados fazem parte do cenario
        add((bobs[:, 0::2] << 4 | bobs[:, 1::2]).astype(np.uint8).tobytes(), 0, 3, group=197)
        headart = [bob_raw(lib2, k_) for k_ in HEAD_ITEMS]        # retratos dos pilotos (LIB2 itens 194-199, 202-217): aparecem na janela do carro (grupos 220+piloto, rascunho STK_Y0)
        for p_, h_ in enumerate(headart): add(packrows(h_, h_.shape[1]), STK_Y0 * 128, 1, group=220 + p_)
        headh = 'static const uint8_t head_w[%d] = {%s};\nstatic const uint8_t head_h[%d] = {%s};\n' % (len(headart), ','.join(str(h_.shape[1]) for h_ in headart), len(headart), ','.join(str(h_.shape[0]) for h_ in headart))
        cal = garage_pixels(lib2, 221)                           # calendario AGO/SET (item 221, 135x150) no centro de uma tela preta (grupo 242)
        calscr = np.zeros((190, 256), np.uint8); calscr[CAL_Y0:CAL_Y0 + cal.shape[0], CAL_X0:CAL_X0 + cal.shape[1]] = cal
        add((calscr[:, 0::2] << 4 | calscr[:, 1::2]).astype(np.uint8).tobytes(), 0, 3, group=242)
        gus = garage_pixels(lib2, 90)                            # posto "Gus' Gas" (item 90, grupo 243)
        add((gus[:, 0::2] << 4 | gus[:, 1::2]).astype(np.uint8).tobytes(), 0, 3, group=243)
        lib1 = os.path.join(os.path.dirname(lib2), 'LIB1')        # telas da LIB1 (mesma paleta do jogo, EGA_PAL): 250 intro, 251 titulo, 252 ranking, 253 vitoria, 254 creditos; 256x200 (320 -> 256) centradas
        for grp_, it_ in ((250, 2), (251, 0), (252, 4), (253, 8), (254, 3)):
            scr = lib1_screen(lib1, it_)
            add((scr[:, 0::2] << 4 | scr[:, 1::2]).astype(np.uint8).tobytes(), 0, 3, group=grp_)
        GEAR_PX, GEAR_SRC_Y = 212, 972                           # painel do cambio no cockpit (itens 227 = 3 marchas, 228 = 4 marchas, 229 = automatico) e botao/alavanca (230, 231, 232):
        mapa = np.array(MAP, np.uint8)                            # grupos 60-62 (painel, na pagina 3 em GEAR_SRC_Y, copiado para as 3 paginas no inicio da corrida) e 63-65 (botao)
        for t_, (pi_, ki_) in enumerate(((229, 232), (227, 230), (228, 231))):
            pn = mapa[garage_pixels(lib2, pi_)]
            cv = np.zeros((pn.shape[0], 256), np.uint8); cv[:, GEAR_PX:GEAR_PX + pn.shape[1]] = pn
            add((cv[:, 0::2] << 4 | cv[:, 1::2]).astype(np.uint8).tobytes(), GEAR_SRC_Y * 128, 1, group=60 + t_)
            kraw = garage_pixels(lib2, ki_); kn = mapa[kraw]; kn[kraw == 1] = 11; kn[kraw == 0] = 0          # contorno escuro vira cinza (0 = transparente)
            ck = np.zeros((kn.shape[0], 256), np.uint8); ck[:, :kn.shape[1]] = kn
            add((ck[:, 0::2] << 4 | ck[:, 1::2]).astype(np.uint8).tobytes(), GEAR_SRC_Y * 128, 1, group=63 + t_)
        # pecas do motor (LIB2): folhas de arte na pagina 1 (livre nos menus), carregadas por grupo sob demanda: 66 = motores 76-78, coletores 79-83/104/105, carburadores 84-86;
        # 67 = cambios 99-102; 68 = pneus 65-68. Posicoes (x, y, w, h) na folha emitidas como art_x/art_y/art_w/art_h (indices: 0-18 grupo 66 [13-15 = itens 87 (alto = solto) / 88 (meio) / 89 (cabeca = apertado), 16 furo = 138, 17-18 fiacao ligada/solta = 94/95], 19-22 grupo 67, 23-26 grupo 68)
        art_pos = []
        for grp_, items_ in ((66, [76, 77, 78, 79, 80, 81, 82, 83, 104, 105, 84, 85, 86, 87, 88, 89, 138, 94, 95]), (67, [99, 100, 101, 102]), (68, [65, 66, 67, 68])):
            sheet = np.zeros((212, 256), np.uint8); x_ = y_ = rh_ = 0
            for it_ in items_:
                px_ = (garage_pixels_thin if it_ in (94, 95) else garage_pixels)(lib2, it_); h_, w_ = px_.shape
                if x_ + w_ > 256: x_ = 0; y_ += rh_; rh_ = 0
                x_ &= ~1
                assert y_ + h_ <= 212
                sheet[y_:y_ + h_, x_:x_ + w_] = px_; art_pos.append((x_, y_, w_, h_)); x_ += w_ + 2; rh_ = max(rh_, h_)
            add((sheet[:, 0::2] << 4 | sheet[:, 1::2]).astype(np.uint8).tobytes(), 256 * 128, 1, group=grp_)
        # posicoes do jogo original em colunas nativas (320 px): o main.c converte engine-relativo com ax(d) = d - (d + 1) / 5 (a mesma regra da 5a coluna removida)
        # medidas na captura original (320x200; motor 78 em (112,77), coletor 104 em (152,69), carburador 86 em (167,60) e (211,60), parafusos 87/89 em carb+9 e carb+w-13 (y carb+6),
        # nas pontas do coletor (+10 / w-10, y+3), no "rosto" do motor (106,37); fiacao 95 em (67,50)). Motores 76/77: parafuso (83,32) e (94,31), fiacao a (-39,+13) dele (correlacao com o rosto do 78)
        # tela TUNING (item 71 = janela inteira 200x175 -> 160x175): na pagina 1 (grupo 69); a direita da arte, 4 quadros da correia dentada (colunas 71-75, linhas 53-81, periodo de 4 linhas)
        # em 4 fases, com o entalhe vermelho (linhas 70-72) fixo
        t71_ = garage_pixels(lib2, 71); cv71_ = np.zeros((212, 256), np.uint8); cv71_[:175, :160] = t71_
        for s_ in range(4):
            for r_ in range(53, 82):
                if 70 <= r_ <= 72: cv71_[r_ - 53, 164 + 6 * s_:164 + 6 * s_ + 5] = t71_[r_, 71:76]
                else: cv71_[r_ - 53, 164 + 6 * s_:164 + 6 * s_ + 5] = t71_[53 + (r_ - 53 - s_) % 4, 71:76]
        add((cv71_[:, 0::2] << 4 | cv71_[:, 1::2]).astype(np.uint8).tobytes(), 256 * 128, 1, group=69)
        eng_r, eng_lf, pw_f, pc_f, mp_n, cw_f = [], [], [], [], [], []
        for i_ in (76, 77, 78):
            ea = garage_pixels(lib2, i_)
            for r_ in range(5, 25):
                if np.count_nonzero((ea[r_] == 4) | (ea[r_] == 5)) >= 0.5 * ea.shape[1]: eng_r.append(r_); break
            eng_lf.append(int(np.nonzero(garage_full(lib2, i_)[0])[0][0]))
        for i_ in (79, 80, 81, 82, 83, 104, 105):             # coletor: largura e centros das almofadas (corridas da fileira 0) em colunas nativas
            pa = garage_full(lib2, i_)[0]; runs_ = []; x_ = 0
            while x_ < len(pa):
                if pa[x_] > 1:
                    s_ = x_
                    while x_ < len(pa) and pa[x_] > 1: x_ += 1
                    if x_ - s_ >= 10: runs_.append((s_ + x_) // 2)
                else: x_ += 1
            mp_n.append(len(runs_)); pw_f.append(len(pa)); pc_f.append((runs_ + [0, 0, 0])[:3])
        cw_f = [garage_full(lib2, i_).shape[1] for i_ in (84, 85, 86)]
        eng_bf = [(83, 32), (94, 31), (106, 37)]; eng_wf = [(44, 45), (55, 44), (67, 50)]
        parth = ''.join('static const uint8_t art_%s[%d] = {%s};\n' % (c_, len(art_pos), ','.join(str(p_[k_]) for p_ in art_pos)) for k_, c_ in enumerate('xywh'))
        parth += 'static const uint8_t eng_r[3] = {%s}, eng_lf[3] = {%s}, eng_bxf[3] = {%s}, eng_byf[3] = {%s}, eng_wxf[3] = {%s}, eng_wyf[3] = {%s}, cw_f[3] = {%s}, pw_f[7] = {%s}, mp_n[7] = {%s}, pc_f[7][3] = {%s};\n' % (','.join(map(str, eng_r)), ','.join(map(str, eng_lf)), ','.join(str(p_[0]) for p_ in eng_bf), ','.join(str(p_[1]) for p_ in eng_bf), ','.join(str(p_[0]) for p_ in eng_wf), ','.join(str(p_[1]) for p_ in eng_wf), ','.join(map(str, cw_f)), ','.join(map(str, pw_f)), ','.join(map(str, mp_n)), ','.join('{%s}' % ','.join(map(str, r_)) for r_ in pc_f))
        bs_ = garage_pixels(lib2, 267)[5:5 + BG_H]; bg_ = np.zeros((BG_H, 256), np.uint8)       # 267: colinas (vermelho com dither) + grama (linhas 5-19); copia OPACA (HMMM): o ceu preto e cor 0
        for y_ in range(BG_H):
            for x_ in range(256):
                v_ = int(bs_[y_, x_])
                bg_[y_, x_] = {14: 8 if (x_ + 2 * y_) % 5 == 0 else (6 if (x_ + y_) % 2 == 0 else 10), 10: 10, 9: 13, 13: 3}.get(v_, 0)
        add((bg_[:, 0::2] << 4 | bg_[:, 1::2]).astype(np.uint8).tobytes(), BG_Y * 128, 1)
        # ---- cenario da rua: arvores e predios (procedurais), placas (LIB2 258, 236 e 238, quadros de 7 tamanhos) e tabela de objetos por segmento ----
        # VRAM livre na corrida: linhas 212-255 da pagina 0 e 468-511 da pagina 1 (fora da tela de 212 linhas) e 704-767 da pagina 2
        rng_ = np.random.RandomState(11)
        def make_tree(w, h):
            t_ = np.zeros((h, w), np.uint8); ch_ = max(4, round(h * 0.72)); tw_ = max(1, round(w / 5)); tx_ = (w - tw_) // 2
            cx_, cy_ = (w - 1) / 2, (ch_ - 1) / 2
            lobes_ = [(0, 0, 0.55), (-0.28, 0.15, 0.40), (0.28, 0.15, 0.40), (0, -0.25, 0.38)]       # copa: circulos (dx, dy em frac. da largura/altura, raio em frac. da largura)
            for r_ in range(ch_):
                for c_ in range(w):
                    for dx_, dy_, rr_ in lobes_:
                        if ((c_ - cx_ - dx_ * w) / (rr_ * w)) ** 2 + ((r_ - cy_ - dy_ * ch_) / (rr_ * w * 0.95)) ** 2 <= 1:
                            lit_ = ((c_ - cx_) / w + (r_ - cy_) / ch_) < -0.12 + (rng_.rand() - 0.5) * 0.25
                            t_[r_, c_] = 13 if lit_ else 3
                            break
            for r_ in range(ch_ - 1, h): t_[r_, tx_:tx_ + tw_] = 10
            if tw_ > 1: t_[ch_ - 1:h, tx_] = 6
            return t_
        def make_bld(w, h):
            t_ = np.full((h, w), 15, np.uint8); t_[0, :] = 4; t_[1:2, :] = 11; t_[:, 0] = 11; t_[:, -1] = 11
            ws_ = max(1, w // 8); gx_ = max(2, w // 5)
            for r_ in range(3, h - 2, ws_ + max(2, h // 9)):
                for c_ in range(gx_ // 2 + 1, w - ws_ - 1, gx_ + ws_):
                    t_[r_:r_ + ws_ + (1 if w > 20 else 0), c_:c_ + ws_] = 8 if rng_.rand() < 0.55 else 11
            return t_
        tree_wh = [(7, 10), (11, 16), (16, 24), (23, 34)]; bld_wh = [(12, 14), (18, 21), (26, 31), (40, 44)]
        sheet_ = np.zeros((44, 256), np.uint8); tree_x = []; bld_x = []; x_ = 0
        for w_, h_ in tree_wh:
            sheet_[:h_, x_:x_ + w_] = make_tree(w_, h_); tree_x.append(x_); x_ += w_ + 1
        x_ = 96                                                     # (x par: os predios usam HMMM)
        for w_, h_ in bld_wh:
            sheet_[:h_, x_:x_ + w_] = make_bld(w_, h_); bld_x.append(x_); x_ += w_ + 2
        # placa CITY LIMITS (portico da rua): 3 tamanhos em fonte de pixels propria, na pagina 0 (linhas 212-255), a direita dos predios: grande 40x18 (5x7), media 26x13 (3x5), pequena 14x7
        F57 = {'C': ['.###.', '#...#', '#....', '#....', '#....', '#...#', '.###.'], 'I': ['.###.', '..#..', '..#..', '..#..', '..#..', '..#..', '.###.'], 'T': ['#####', '..#..', '..#..', '..#..', '..#..', '..#..', '..#..'],
               'Y': ['#...#', '#...#', '.#.#.', '..#..', '..#..', '..#..', '..#..'], 'L': ['#....', '#....', '#....', '#....', '#....', '#....', '#####'], 'M': ['#...#', '##.##', '#.#.#', '#.#.#', '#...#', '#...#', '#...#'],
               'S': ['.####', '#....', '#....', '.###.', '....#', '....#', '####.']}
        F35 = {'C': ['###', '#..', '#..', '#..', '###'], 'I': ['###', '.#.', '.#.', '.#.', '###'], 'T': ['###', '.#.', '.#.', '.#.', '.#.'], 'Y': ['#.#', '#.#', '.#.', '.#.', '.#.'],
               'L': ['#..', '#..', '#..', '#..', '###'], 'M': ['#.#', '###', '###', '#.#', '#.#'], 'S': ['###', '#..', '###', '..#', '###']}
        def sign_img(w, h, font, gw, gh):
            im_ = np.full((h, w), 7, np.uint8)
            for li_, txt_ in enumerate(('CITY', 'LIMITS')):
                x0_ = (w - (len(txt_) * (gw + 1) - 1)) // 2; y0_ = 1 + li_ * (gh + 1)
                for ci_, ch_ in enumerate(txt_):
                    for r_, row_ in enumerate(font[ch_]):
                        for c_, v_ in enumerate(row_):
                            if v_ == '#': im_[y0_ + r_, x0_ + ci_ * (gw + 1) + c_] = 0
            return im_
        sg_big = sign_img(40, 18, F57, 5, 7); sg_med = sign_img(26, 13, F35, 3, 5); sg_sml = np.full((7, 14), 7, np.uint8); sg_sml[2, 2:12] = 0; sg_sml[4, 3:11] = 0
        sheet_[0:18, 200:240] = sg_big; sheet_[19:32, 200:226] = sg_med; sheet_[19:26, 230:244] = sg_sml
        add((sheet_[:, 0::2] << 4 | sheet_[:, 1::2]).astype(np.uint8).tobytes(), TREE_Y * 128, 1)
        FR_X = [0, 64, 120, 168, 208, 240, 256]; FR_W = [64, 56, 48, 40, 32, 16, 8]; FR_H = [43, 37, 30, 24, 19, 13, 8]
        def sign_sheet(item, frames, y0, hmax):
            a_ = garage_full(lib2, item); mp_ = np.array(MAP, np.uint8); out_ = np.zeros((hmax, 256), np.uint8); xs_ = []; ws_ = []; hs_ = []; x_ = 0
            for fi_ in frames:
                f_ = a_[:FR_H[fi_], FR_X[fi_]:FR_X[fi_] + FR_W[fi_]]
                f_ = f_[:, [c_ for c_ in range(f_.shape[1]) if c_ % 5 != 4]]
                m_ = mp_[f_]; m_[(f_ == 1) | (f_ == 6)] = 11; m_[f_ == 0] = 0           # preto interno vira cinza escuro (0 = transparente)
                if m_.shape[1] % 2: m_ = np.hstack([m_, m_[:, -1:]])                       # largura par (copia opaca por bytes)
                out_[:m_.shape[0], x_:x_ + m_.shape[1]] = m_; xs_.append(x_); ws_.append(m_.shape[1]); hs_.append(m_.shape[0]); x_ += m_.shape[1] + 2
            add((out_[:, 0::2] << 4 | out_[:, 1::2]).astype(np.uint8).tobytes(), y0 * 128, 1)
            return xs_, ws_, hs_
        sa_ = sign_sheet(258, [1, 2, 3, 4, 5, 6], SIGA_Y, 37)      # COUNTY LINE: 6 tamanhos (45x37 ... 7x8)
        sb_ = sign_sheet(236, [2, 3, 4, 5, 6], SIGB_Y, 30)         # DRIVE-IN: 5 tamanhos
        sc_ = sign_sheet(238, [2, 3, 4, 5, 6], SIGC_Y, 30)         # GUS' GAS
        scn = []                                                    # por segmento: bits 0-2 objeto da esquerda, 3-5 da direita (1 arvore, 2/3/4 placas, 5 poste, 6 predio), bits 6-7 afastamento
        for s_ in range(256):
            e_ = 0
            for side_ in range(2):
                k_ = 0; r_ = rng_.rand()
                if s_ % 4 == (0 if side_ == 0 else 2): k_ = 5
                elif r_ < 0.20: k_ = 1
                elif r_ < 0.26: k_ = 2 + int(rng_.randint(0, 3))
                elif r_ < 0.32: k_ = 6
                e_ |= k_ << (3 * side_)
            e_ |= int(rng_.randint(0, 3)) << 6
            scn.append(e_)
        def arr_(n, v): return 'static const uint8_t %s[%d] = {%s};\n' % (n, len(v), ','.join(map(str, v)))
        scnh = '#define TREE_Y %d\n#define SIGA_Y %d\n#define SIGB_Y %d\n#define SIGC_Y %d\n' % (TREE_Y, SIGA_Y, SIGB_Y, SIGC_Y)
        scnh += arr_('tr_x', tree_x) + arr_('tr_w', [w_ for w_, h_ in tree_wh]) + arr_('tr_h', [h_ for w_, h_ in tree_wh])
        scnh += arr_('bl_x', bld_x) + arr_('bl_w', [w_ for w_, h_ in bld_wh]) + arr_('bl_h', [h_ for w_, h_ in bld_wh])
        for nm_, s3_ in (('sga', sa_), ('sgb', sb_), ('sgc', sc_)): scnh += arr_(nm_ + '_x', s3_[0]) + arr_(nm_ + '_w', s3_[1]) + arr_(nm_ + '_h', s3_[2])
        scnh += arr_('scn_tab', scn)
        ls = light_sheet(lib2)
        add((ls[:, 0::2] << 4 | ls[:, 1::2]).astype(np.uint8).tobytes(), LIGHT_Y * 128, 1)
        hs = []; xs = []
        for k, item in enumerate(REAR_ITEMS):
            col = rear_sprite(lib2, item)
            sheet = np.zeros((SPR_STRIDE, 256), np.uint8); xs = []; x = 0; hh = []
            for w in SPR_W:
                sp = scale_sprite(col, w); sheet[:sp.shape[0], x:x + w] = sp; xs.append(x); hh.append(sp.shape[0]); x += w
            assert max(hh) <= SPR_STRIDE
            add((sheet[:, 0::2] << 4 | sheet[:, 1::2]).astype(np.uint8).tobytes(), (SPR_Y0 + k * SPR_STRIDE) * 128, 1)
            hs.append(hh)
        sel = [255 if w < SPR_W[0] else max(i for i, s in enumerate(SPR_W) if s <= w) for w in range(128)]
        sprh = ('#define SPR_N %d\n#define SPR_Y0 %d\n#define SPR_STRIDE %d\nstatic const uint8_t spr_x[SPR_N] = {%s};\nstatic const uint8_t spr_w[SPR_N] = {%s};\n'
                'static const uint8_t spr_h[3][SPR_N] = {%s};\nstatic const uint8_t spr_sel[128] = {%s};\n') % (
            len(SPR_W), SPR_Y0, SPR_STRIDE, ','.join(map(str, xs)), ','.join(map(str, SPR_W)),
            ','.join('{%s}' % ','.join(map(str, h)) for h in hs), ','.join(map(str, sel)))
    tdat = os.path.join(here, 'textdata.bin')
    text_bank = 0; text_base = 0
    if ents and os.path.exists(tdat):
        td = open(tdat, 'rb').read()
        if len(banks[-1]) + len(td) > 0x4000: banks.append(bytearray())
        text_base = len(banks[-1]); banks[-1].extend(td); text_bank = len(banks) - 1 + ABASE      # nomes (carros, pecas, pilotos) no fim do ultimo banco de assets
    rect_bank = rect_base = 0
    if ents and 'rect_blob' in dir():
        if len(banks[-1]) + len(rect_blob) > 0x4000: banks.append(bytearray())
        rect_base = len(banks[-1]); banks[-1].extend(rect_blob); rect_bank = len(banks) - 1 + ABASE
    mt_bank = mt_base = 0
    if ents:                                                       # tabelas de periodos (PSG) e numeros F (OPLL) das notas: 128 x 2 bytes cada, lidas por nota (mus_tab)
        if len(banks[-1]) + 512 > 0x4000: banks.append(bytearray())
        mt_base = len(banks[-1]); mt_bank = len(banks) - 1 + ABASE
        per_ = [min(4095, round(55930 / (440 * 2 ** ((n - 69) / 12)))) if n >= 12 else 0 for n in range(128)]
        def fmn_(n):
            fr = 440 * 2 ** ((n - 69) / 12)
            for blk in range(8):
                F = round(fr / (49716 / 2 ** (19 - blk)))
                if F <= 511: return (blk << 9) | F
            return (7 << 9) | 511
        banks[-1].extend(b''.join(struct.pack('<H', v) for v in per_ + [fmn_(n) if n >= 12 else 0 for n in range(128)]))
    import music_gen                                              # radio: cada estacao em seu proprio banco (3 bytes por linha de sequenciador)
    mus = []
    if ents:
        for sg in music_gen.all_songs():
            banks.append(bytearray(sg.data())); mus.append((len(banks) - 1 + ABASE, sg.n, sg.row_ms))
    nb = len(banks) if ents else 0
    print('assets: %d faixas em %d banco(s) de 16 KB' % (len(ents), nb), file=sys.stderr)
    open(os.path.join(here, 'assets.bin'), 'wb').write(b''.join(bytes(b).ljust(0x4000, b'\xff') for b in banks) if ents else b'')
    with open(os.path.join(here, 'assets.inc'), 'w') as f:       # tabela lida pelo carregador em assembly (fica na pagina 1, fixa)
        f.write('\t.equ ASSET_N, %d\nasset_tab:\n' % len(ents))
        for b, grp, src, v, pg, cp in ents:
            f.write('\t.db %d, %d\n\t.dw %d\n\t.dw %d\n\t.db %d, %d, %d\n' % (b, grp, src, v & 0xFFFF, v >> 16, pg, cp))
    with open(os.path.join(here, 'assets.h'), 'w') as f:
        import math
        L = 14
        def tips(n, ang):
            return ','.join('%d,%d' % (round(L * math.cos(math.radians(ang(i)))), -round(L * math.sin(math.radians(ang(i))))) for i in range(n))
        f.write('#define HAVE_COCKPIT %d\n#define TEXT_BANK %d\n#define TEXT_BASE %d\n#define RECT_BANK %d\n#define RECT_BASE %d\n#define MT_BANK %d\n#define MT_BASE %d\n' % (1 if ents else 0, text_bank, text_base, rect_bank, rect_base, mt_bank, mt_base))
        f.write('static const uint8_t mus_bank[5] = {%s};\nstatic const uint16_t mus_rows[5] = {%s};\nstatic const uint16_t mus_ms[5] = {%s};\n' % tuple(','.join(str(m[i]) for m in mus) for i in range(3)))
        gp = []
        for r, g_, b_ in EGA_PAL: gp += [(round(r * 7 / 255) << 4) | round(b_ * 7 / 255), round(g_ * 7 / 255)]
        f.write('static const uint8_t pal_garage[32] = {%s};\n' % ','.join(map(str, gp)))
        pe = []
        for r, g_, b_ in EGA_PAL: pe += [(round(r * 7 / 255) << 4) | round(b_ * 7 / 255), round(g_ * 7 / 255)]
        f.write('static const uint8_t pal_ega[32] = {%s};\n' % ','.join(map(str, pe)))
        RANK_PAL = [(85,85,85),(0,0,0),(170,85,0),(0,170,170),(255,255,255),(255,85,255),(255,170,0),(0,170,170),
                    (120,120,120),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(255,255,85)]   # muro de grafite: 0 cinza escuro, 1 preto, 4 branco (texto), 15 amarelo (destaque)
        CRED_PAL = list(EGA_PAL); CRED_PAL[5] = (110, 175, 255)      # placa "AD 1989": fundo azul claro
        pc = []
        for r, g_, b_ in CRED_PAL: pc += [(round(r * 7 / 255) << 4) | round(b_ * 7 / 255), round(g_ * 7 / 255)]
        f.write('static const uint8_t pal_cred[32] = {%s};\n' % ','.join(map(str, pc)))
        pr = []
        for r, g_, b_ in RANK_PAL: pr += [(round(r * 7 / 255) << 4) | round(b_ * 7 / 255), round(g_ * 7 / 255)]
        f.write('static const uint8_t pal_rank[32] = {%s};\n' % ','.join(map(str, pr)))
        f.write(parth)
        f.write(carh); f.write(headh); f.write('#define CAL_X0 %d\n#define CAL_Y0 %d\n' % (CAL_X0, CAL_Y0))
        f.write('#define OV_N %d\n#define LIGHT_Y %d\n#define BG_Y %d\n#define BG_H %d\n' % (len(orects), LIGHT_Y, BG_Y, BG_H))
        f.write('#define RACE_X0 %d\n#define OV_LEFT %d\n' % (RACE_X0, sum(1 for o_ in orects if (o_[0] + o_[2]) * 2 <= RACE_X0)))     # a janela esquerda (x < RACE_X0: vidro lateral e coluna A) e pintada uma vez; a estrada comeca em RACE_X0
        f.write('static const uint8_t ov_x[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[0] // 2) for r in orects) or '0'))
        f.write('static const uint8_t ov_y[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[1]) for r in orects) or '0'))
        f.write('static const uint8_t ov_w[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[2] // 2) for r in orects) or '0'))
        f.write('static const uint8_t ov_h[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[3]) for r in orects) or '0'))
        f.write('#define WIN_Y %d\n#define SKY_TOP %d\n' % (WIN_Y, SKY_TOP))
    open(os.path.join(here, 'assets_scn.h'), 'w').write(scnh)         # cenario da rua: dados do BANK5 (draw_scn)
    with open(os.path.join(here, 'assets_race.h'), 'w') as fr:        # so usado pelo codigo da corrida (banco 1): fica fora do nucleo
        fr.write(sprh)
        import math
        L = 14
        def tips(n, ang):
            return ','.join('%d,%d' % (round(L * math.cos(math.radians(ang(i)))), -round(L * math.sin(math.radians(ang(i))))) for i in range(n))
        fr.write('/* ponteiros dos mostradores do cockpit: (dx,dy) da ponta em relacao ao eixo; tacometro: indice = rpm/50, velocimetro: indice = mph */\n')
        fr.write('#define GAUGE_RPM_X 117\n#define GAUGE_RPM_Y 142\n#define GAUGE_SPD_X 157\n#define GAUGE_SPD_Y 142\n')
        fr.write('#define NEEDLE_RPM_N 108\n#define NEEDLE_SPD_N 144\n')
        fr.write('static const int8_t needle_rpm[NEEDLE_RPM_N * 2] = {%s};\n' % tips(108, lambda i: 225 - (i / 2) / 60 * 270))
        fr.write('static const int8_t needle_spd[NEEDLE_SPD_N * 2] = {%s};\n' % tips(144, lambda j: 225 - j * 270 / 130))

if __name__ == '__main__': main()
