#!/usr/bin/env python3
"""Extrai dados de jogo de HOT_DATA + SRSE.EXE do Street Rod SE para CSV/JSON.
uso: hotdata.py SRSE.EXE HOT_DATA saida_dir
Os arquivos do jogo NAO ficam no repositorio; as saidas tambem nao (ver .gitignore)."""
import csv, json, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from unexepack import unpack

DS = 0x3E960                       # base do segmento de dados na imagem desempacotada
SIZES = (396, 260, 260, 312, 520, 156, 1850, 2, 200, 18, 18, 24, 48)   # blocos de HOT_DATA (ordem do arquivo)
CAT = {0: 'engine', 1: 'transmission', 2: 'carburetor', 3: 'manifold', 4: 'tires'}
MAKER = {1: 'GM', 2: 'Ford', 4: 'Chrysler', 7: 'all'}

def cstr(buf, off):
    return buf[off:buf.index(b'\0', off)].decode('latin-1')

def main(exe, hot, out):
    img = unpack(open(exe, 'rb').read())[1]
    h = open(hot, 'rb').read()
    blocks, o = [], 0
    for s in SIZES:
        blocks.append(h[o:o + s]); o += s
    pool = blocks[6]                 # texto: carros, descricoes, nomes dos pilotos (DS:0x2C02)
    os.makedirs(out, exist_ok=True)

    # --- carros: 25 registros de 10 bytes (bloco 1): preco, desempenho, indice, flags, offset do nome
    cars = []
    for i in range(25):
        price, perf, idx, flags, name_off = struct.unpack('<HBBHH', blocks[1][10 * i:10 * i + 8])
        name = cstr(pool, name_off)
        desc = cstr(pool, name_off + len(name) + 1)
        cars.append(dict(id=i, name=name, desc=desc, price=price, perf=perf, pic_index=idx, flags=flags,
                         price_in_text=int(desc.rsplit('$', 1)[1]) if '$' in desc else None))
    # --- pilotos oponentes: 22 registros de 18 bytes (bloco 0)
    opps = []
    for i in range(22):
        r = blocks[0][18 * i:18 * i + 18]
        a, b = r[1], r[2]
        name_off = struct.unpack('<H', r[6:8])[0]
        opps.append(dict(id=i, name=cstr(pool, name_off), param_a=a, param_b=b, raw=r.hex()))
    # --- pecas: catalogo de 43 entradas de 8 bytes em DS:0x4808 (+ texto na continuacao do pool, DS:0x2C02+ptr)
    parts = []
    ptrs = []
    # o preco de cada peca fica no +6 da entrada ANTERIOR (a struct de 8 bytes comeca em 0x4808+6-8): price[k] = word em 0x4806+8k
    price_of = lambda k: struct.unpack('<H', img[DS + 0x4806 + 8 * k: DS + 0x4806 + 8 * k + 2])[0]
    for i in range(43):
        e = img[DS + 0x4808 + 8 * i: DS + 0x4808 + 8 * i + 8]
        ptrs.append(struct.unpack('<H', e[4:6])[0])
    order = sorted(set(ptrs))
    for i in range(43):
        e = img[DS + 0x4808 + 8 * i: DS + 0x4808 + 8 * i + 8]
        variant, makers, cat = e[0], e[1], e[2]
        ptr = struct.unpack('<H', e[4:6])[0]
        price = price_of(i)
        base = DS + 0x2C02 + ptr
        nxt = [p for p in order if p > ptr]
        end = DS + 0x2C02 + nxt[0] if nxt else base + 80
        text = ' / '.join(t for t in img[base:end].decode('latin-1').split('\0') if t and not t.strip().startswith('---') and t.strip('- USEDCAR') != '')
        text = text.split(' / ----')[0]
        parts.append(dict(id=i, category=CAT.get(cat, cat), variant=variant, makers=MAKER.get(makers, makers),
                          price=price, text=text))
    # --- tabelas pequenas
    tabs = {'t9_words': list(struct.unpack('<9H', blocks[9])), 't10_words': list(struct.unpack('<9H', blocks[10])),
            't11_words': list(struct.unpack('<12H', blocks[11]))}
    for name, rows in (('cars', cars), ('opponents', opps), ('parts', parts)):
        with open(os.path.join(out, name + '.csv'), 'w', newline='') as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys())); w.writeheader(); w.writerows(rows)
    json.dump(dict(cars=cars, opponents=opps, parts=parts, **tabs), open(os.path.join(out, 'hotdata.json'), 'w'), indent=1)
    import re
    bad = [c['name'] for c in cars if c['price_in_text'] != c['price']]
    badp = []
    for p in parts:
        m = re.findall(r'\$(\d+)', p['text'])
        if m and int(m[-1]) != p['price']: badp.append((p['id'], p['price'], m[-1]))
    print('carros', len(cars), 'pilotos', len(opps), 'pecas', len(parts))
    print('divergencia de preco carros:', bad or 'nenhuma', '| pecas:', badp or 'nenhuma')

if __name__ == '__main__':
    main(*sys.argv[1:4])
