#!/usr/bin/env python3
"""Gera os assets graficos (cockpit etc.) a partir de LIB2 -> assets.bin (vai em 0x8000 da ROM) + assets.h.
Uso: LIB2=<caminho> ./gen_assets.py   (sem LIB2, gera assets vazios e o jogo roda sem cockpit).
Formato LZ4-like por faixas de 4096 bytes: token(lit<<4|mlen-4) [ext lit] literais off16 [ext mlen]; off16==0 encerra."""
import os, sys
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
    glass[108 - y0:, :54] = False             # abaixo da janela lateral o preto e a porta (opaco)
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
    sprh = ''; orects = []; carh = ''; headh = ''
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
        f.write('#define HAVE_COCKPIT %d\n#define TEXT_BANK %d\n#define TEXT_BASE %d\n' % (1 if ents else 0, text_bank, text_base))
        f.write('static const uint8_t mus_bank[5] = {%s};\nstatic const uint16_t mus_rows[5] = {%s};\nstatic const uint16_t mus_ms[5] = {%s};\n' % tuple(','.join(str(m[i]) for m in mus) for i in range(3)))
        def fmnum(n):                                              # OPLL: f = F * 49716 / 2^(19 - bloco); F de 9 bits, bloco 0..7 -> bits (bloco << 9) | F
            fr = 440 * 2 ** ((n - 69) / 12)
            for blk in range(8):
                F = round(fr / (49716 / 2 ** (19 - blk)))
                if F <= 511: return (blk << 9) | F
            return (7 << 9) | 511
        f.write('static const uint16_t mus_ftab[128] = {%s};\n' % ','.join(str(fmnum(n)) if n >= 12 else '0' for n in range(128)))
        f.write('static const uint16_t mus_per[128] = {%s};\n' % ','.join(str(min(4095, round(55930 / (440 * 2 ** ((n - 69) / 12))))) if n >= 12 else '0' for n in range(128)))
        gp = []
        for r, g_, b_ in EGA_PAL: gp += [(round(r * 7 / 255) << 4) | round(b_ * 7 / 255), round(g_ * 7 / 255)]
        f.write('static const uint8_t pal_garage[32] = {%s};\n' % ','.join(map(str, gp)))
        pe = []
        for r, g_, b_ in EGA_PAL: pe += [(round(r * 7 / 255) << 4) | round(b_ * 7 / 255), round(g_ * 7 / 255)]
        f.write('static const uint8_t pal_ega[32] = {%s};\n' % ','.join(map(str, pe)))
        RANK_PAL = [(85,85,85),(0,0,0),(170,85,0),(0,170,170),(255,255,255),(255,85,255),(255,170,0),(0,170,170),
                    (120,120,120),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(0,0,0),(255,255,85)]   # muro de grafite: 0 cinza escuro, 1 preto, 4 branco (texto), 15 amarelo (destaque)
        pr = []
        for r, g_, b_ in RANK_PAL: pr += [(round(r * 7 / 255) << 4) | round(b_ * 7 / 255), round(g_ * 7 / 255)]
        f.write('static const uint8_t pal_rank[32] = {%s};\n' % ','.join(map(str, pr)))
        f.write(sprh); f.write(carh); f.write(headh); f.write('#define CAL_X0 %d\n#define CAL_Y0 %d\n' % (CAL_X0, CAL_Y0))
        f.write('#define OV_N %d\n#define LIGHT_Y %d\n' % (len(orects), LIGHT_Y))
        f.write('static const uint8_t ov_x[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[0] // 2) for r in orects) or '0'))
        f.write('static const uint8_t ov_y[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[1]) for r in orects) or '0'))
        f.write('static const uint8_t ov_w[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[2] // 2) for r in orects) or '0'))
        f.write('static const uint8_t ov_h[%d] = {%s};\n' % (max(1, len(orects)), ','.join(str(r[3]) for r in orects) or '0'))
        f.write('#define WIN_Y %d\n#define SKY_TOP %d\n' % (WIN_Y, SKY_TOP))
        f.write('/* ponteiros dos mostradores do cockpit: (dx,dy) da ponta em relacao ao eixo; tacometro: indice = rpm/50, velocimetro: indice = mph */\n')
        f.write('#define GAUGE_RPM_X 117\n#define GAUGE_RPM_Y 142\n#define GAUGE_SPD_X 157\n#define GAUGE_SPD_Y 142\n')
        f.write('#define NEEDLE_RPM_N 108\n#define NEEDLE_SPD_N 144\n')
        f.write('static const int8_t needle_rpm[NEEDLE_RPM_N * 2] = {%s};\n' % tips(108, lambda i: 225 - (i / 2) / 60 * 270))
        f.write('static const int8_t needle_spd[NEEDLE_SPD_N * 2] = {%s};\n' % tips(144, lambda j: 225 - j * 270 / 130))
if __name__ == '__main__': main()
