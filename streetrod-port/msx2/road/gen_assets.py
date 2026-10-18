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
WIN_Y, SKY_TOP, VIEW_BOT = 24, 22, 88     # janela da estrada (tem de bater com build.sh: VIEW_BOT)
OV_SY = 702                                # sobreposicao do cockpit (so as partes opacas) na pagina 2, abaixo do cockpit limpo
SPR_W = [6, 8, 11, 15, 20, 27, 36, 48, 64]      # larguras das versoes reduzidas do carro (visto por tras)
SPR_Y0 = 768                                      # a folha de sprites fica na pagina 3 da VRAM

def overlay(a):
    """Parte do cockpit que fica POR CIMA da estrada: vidro (preto ligado ao para-brisa e a janela lateral) vira transparente (0);
    pretos de outras partes viram cor 11."""
    y0, y1 = WIN_Y + SKY_TOP, WIN_Y + VIEW_BOT
    sub = a[y0:y1]
    h, w = sub.shape
    glass = np.zeros((h, w), bool)
    st = [(60 - y0, 160), (70 - y0, 25), (y1 - 1 - y0, 160)]
    while st:
        y, x = st.pop()
        if 0 <= y < h and 0 <= x < w and not glass[y, x] and sub[y, x] == 0:
            glass[y, x] = True; st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    glass[106 - y0:] = False                   # abaixo da base das janelas (y >= 106) o preto e interior do carro, nao vidro
    out = np.where(sub == 0, 11, sub).astype(np.uint8); out[glass] = 0
    # regioes com moldura: coluna esquerda (x < aw) e, a partir da linha by, o painel em toda a largura
    opaque = out != 0
    by = next(r for r in range(h) if opaque[r, 60:].sum() > 8)
    aw = int(np.nonzero(opaque[:by].any(axis=0) & (np.arange(w) < 120))[0].max()) + 2
    return out, by, (aw + 1) & ~1

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
    bg = np.zeros((h, w), bool); st = [(y, x) for y in range(h) for x in (0, w - 1)] + [(y, x) for x in range(w) for y in (0, h - 1)]
    while st:
        y, x = st.pop()
        if 0 <= y < h and 0 <= x < w and not bg[y, x] and col[y, x] == 0:
            bg[y, x] = True; st += [(y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)]
    col = np.where(col == 0, 11, col).astype(np.uint8); col[bg] = 0
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
    blob = bytearray(); ents = []       # ents: (offset_rom, vram_addr, paginas_256, copias)
    lib2 = os.environ.get('LIB2')
    def add(raw, vaddr, copies):
        for s in range(0, len(raw), STRIP):
            chunk = bytes(raw[s:s + STRIP]); chunk += bytes(-len(chunk) % 256)
            enc = lz_encode(chunk); assert lz_decode(enc) == chunk
            ents.append((0x8000 + len(blob), vaddr + s, len(chunk) // 256, copies)); blob.extend(enc)
    sprh = ''; OV_AW = OV_BY = 0
    if lib2:
        a = cockpit_pixels(lib2)
        np.save(os.path.join(here, 'cockpit_px.npy'), a)
        ov, OV_BY, OV_AW = overlay(a); np.save(os.path.join(here, 'overlay_px.npy'), ov)
        add((a[:, 0::2] << 4 | a[:, 1::2]).astype(np.uint8).tobytes(), 0, 3)    # copias: paginas 0, 1 e 2 (limpa, para restaurar)
        add((ov[:, 0::2] << 4 | ov[:, 1::2]).astype(np.uint8).tobytes(), OV_SY * 128, 1)
        col = rear_sprite(lib2, int(os.environ.get('OPP_SPRITE', '248')))
        sheet = np.zeros((SPR_W[-1] * col.shape[0] // col.shape[1] + 2, 256), np.uint8)
        xs = []; x = 0
        for w in SPR_W:
            sp = scale_sprite(col, w); sheet[:sp.shape[0], x:x + w] = sp; xs.append((x, w, sp.shape[0])); x += w
        np.save(os.path.join(here, 'sheet_px.npy'), sheet)
        add((sheet[:, 0::2] << 4 | sheet[:, 1::2]).astype(np.uint8).tobytes(), 0x18000, 1)
        sel = [255 if w < SPR_W[0] else max(i for i, s in enumerate(SPR_W) if s <= w) for w in range(128)]
        sprh = ('#define SPR_N %d\n#define SPR_Y0 %d\nstatic const uint8_t spr_x[SPR_N] = {%s};\nstatic const uint8_t spr_w[SPR_N] = {%s};\n'
                'static const uint8_t spr_h[SPR_N] = {%s};\nstatic const uint8_t spr_sel[128] = {%s};\n') % (
            len(SPR_W), SPR_Y0, ','.join(str(t[0]) for t in xs), ','.join(str(t[1]) for t in xs), ','.join(str(t[2]) for t in xs), ','.join(map(str, sel)))
    print('assets: %d faixas, %d bytes comprimidos' % (len(ents), len(blob)), file=sys.stderr)
    assert len(blob) <= 0x4000, 'assets > 16 KB'
    open(os.path.join(here, 'assets.bin'), 'wb').write(blob)
    with open(os.path.join(here, 'assets.h'), 'w') as f:
        import math
        L = 14
        def tips(n, ang):
            return ','.join('%d,%d' % (round(L * math.cos(math.radians(ang(i)))), -round(L * math.sin(math.radians(ang(i))))) for i in range(n))
        f.write('#define HAVE_COCKPIT %d\n#define ASSET_N %d\n' % (1 if ents else 0, len(ents)))
        n = max(1, len(ents))
        f.write('static const uint16_t asset_src[%d] = {%s};\n' % (n, ','.join(str(e[0]) for e in ents) or '0'))
        f.write('static const uint16_t asset_vlo[%d] = {%s};\n' % (n, ','.join(str(e[1] & 0xFFFF) for e in ents) or '0'))
        f.write('static const uint8_t asset_vhi[%d] = {%s};\n' % (n, ','.join(str(e[1] >> 16) for e in ents) or '0'))
        f.write('static const uint8_t asset_pg[%d] = {%s};\n' % (n, ','.join(str(e[2]) for e in ents) or '0'))
        f.write('static const uint8_t asset_cp[%d] = {%s};\n' % (n, ','.join(str(e[3]) for e in ents) or '0'))
        f.write(sprh)
        f.write('#define WIN_Y %d\n#define SKY_TOP %d\n#define OV_SY %d\n#define OV_AW %d\n#define OV_BY %d\n' % (WIN_Y, SKY_TOP, OV_SY, OV_AW, OV_BY))
        f.write('/* ponteiros dos mostradores do cockpit: (dx,dy) da ponta em relacao ao eixo; tacometro: indice = rpm/50, velocimetro: indice = mph */\n')
        f.write('#define GAUGE_RPM_X 117\n#define GAUGE_RPM_Y 142\n#define GAUGE_SPD_X 157\n#define GAUGE_SPD_Y 142\n')
        f.write('#define NEEDLE_RPM_N 108\n#define NEEDLE_SPD_N 144\n')
        f.write('static const int8_t needle_rpm[NEEDLE_RPM_N * 2] = {%s};\n' % tips(108, lambda i: 225 - (i / 2) / 60 * 270))
        f.write('static const int8_t needle_spd[NEEDLE_SPD_N * 2] = {%s};\n' % tips(144, lambda j: 225 - j * 270 / 130))
if __name__ == '__main__': main()
