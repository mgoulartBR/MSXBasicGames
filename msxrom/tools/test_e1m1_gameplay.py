"""HUD, animacoes e barris no harness Z80 (SIMULATED, nao openMSX/hardware)."""
import os
import re
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, os.path.dirname(__file__))
from e1m1_sim import make, symbols
import e1m1_build as B
import e1m1_pvs as P

root = Path(__file__).resolve().parent.parent
output = root / 'private' / 'gameplay-simulated'
output.mkdir(exist_ok=True)
header = (root / 'private' / 'e1m1_assets.h').read_text()
defines = {n: int(v) for n, v in re.findall(r'#define (\w+) (-?\d+)', header)}
sym = symbols()
held = False
M = make(lambda row: 0xFE if held and row == 8 else 0xFF)
mem = M.m.memory

def frames(n=1):
    target = len(M.frames) + n
    for _ in range(2000000):
        M.m.ticks_to_stop = 32
        M.m.run()
        if len(M.frames) >= target:
            break
    assert len(M.frames) == target

def put(name, value, width=2):
    mem[sym[name]:sym[name]+width] = (int(value) & ((1 << (width*8))-1)).to_bytes(width, 'little')

def get(name, width=2):
    return int.from_bytes(mem[sym[name]:sym[name]+width], 'little', signed=width == 2)

fields = {'x': (0, 2), 'y': (2, 2), 'hp': (4, 2), 'kind': (6, 1),
          'st': (7, 1), 't': (8, 1), 'on': (9, 1), 'vis': (10, 1)}

def thing(i, **values):
    base = sym['th'] + 11*i
    for key, value in values.items():
        off, size = fields[key]
        mem[base+off:base+off+size] = (int(value) & ((1 << (size*8))-1)).to_bytes(size, 'little')
    return {key: int.from_bytes(mem[base+off:base+off+size], 'little', signed=size == 2)
            for key, (off, size) in fields.items()}

def reset():
    global held
    held = False
    for i in range(defines['NTHING']):
        thing(i, on=0)
    put('px', 1056); put('py', -3616); put('eye', 41)
    put('hp', 100); put('armor', 0); put('ammo', 50)
    for name in ('over', 'yaw', 'cool', 'fire_t', 'kills'):
        put(name, 0, 1)
    frames(2)

def actor(i=0, **values):
    defaults = dict(x=1056, y=-3536, hp=60, kind=0, st=0, t=0, on=1, vis=1)
    defaults.update(values)
    return thing(i, **defaults)

def sprite_uvs():
    return [M.gt[k] for k in range(M.gb[0x59])
            if M.gf[k][1] == (-1204, 2353, -2202)]

L = P.load(os.environ['E1M1_WAD'])
atlas, sprite_info = B.build_sprite_cells(L.w, L.w.playpal())
rects = [r for r, _ in sprite_info]

def uv(name):
    x, y, w, h = rects[B.SLOT[name]]
    return (x, y, x+w-1, y, x+w-1, y+h-1, x, y+h-1)

frames(2)
assert all(thing(i)['hp'] == 20 for i in range(defines['NTHING']) if thing(i)['kind'] == 14)
print('PASS barris iniciam com 20 HP')

# Testa todos os glifos, a mudanca de largura e a ausencia de vestigios nas duas paginas.
reset()
vram = M.vram.reshape(1024, 256)
digit_table = np.array(re.findall(r'\{(\d+),(\d+),(\d+),(\d+)\}',
                                re.search(r'digit_tab\[12\]\[4\]=\{(.+?)\};', header).group(1)), int)
face_table = np.array(re.findall(r'\{(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\}',
                               re.search(r'face_tab\[6\]\[6\]=\{(.+?)\};', header).group(1)), int)

def hud_expected(health, armor, ammo):
    image = np.zeros((34, 256), np.uint8)
    def copy(x, y, w, h, dx, dy):
        source = vram[y:y+h, x:x+w]
        dest = image[dy:dy+h, dx:dx+w]
        dest[:] = np.where(source == 0, dest, source)
    copy(defines['BAR_X'], defines['BAR_Y'], defines['BAR_W'], defines['BAR_H'], 0, 0)
    copy(0, defines['ARMS_Y'], defines['ARMS_W'], defines['ARMS_H'], 83, 0)
    for value, right, percent in ((health, 72, True), (armor, 177, True), (ammo, 35, False)):
        if percent:
            copy(*digit_table[10], right, 3)
        for digit in reversed(str(min(999, max(0, value)))):
            right -= digit_table[0][2]
            copy(*digit_table[int(digit)], right, 3)
    f = 5 if health <= 0 else min(4, max(0, (100-health)//20))
    copy(defines['BAR_X']+114, defines['BAR_Y'], 26, defines['BAR_H'], 114, 0)
    x, y, w, h, dx, dy = face_table[f]
    copy(x, y, w, h, dx, dy)
    return image

for value in (111, 222, 333, 444, 555, 666, 777, 888, 999, 100, 20, 10, 1, 0):
    put('hp', max(1, value)); put('armor', value); put('ammo', value)
    frames(2)
    expected = hud_expected(max(1, value), value, value)
    for page in (0, 1):
        assert np.array_equal(vram[page*256+178:page*256+212], expected), ('HUD', value, page)
    if value == 111:
        M.frames[-1].save(output / 'hud-111.png')
put('hp', 200); put('armor', 100); put('ammo', 111); frames(2)
put('ammo', 1); frames(2)
for page in (0, 1):
    assert np.array_equal(vram[page*256+178:page*256+212], hud_expected(200, 100, 1))
M.frames[-1].save(output / 'hud-final.png')
print('PASS HUD: glifos individuais, offsets do rosto, alinhamento e limpeza nas duas paginas')

for kind, prefix, death in ((0, 'POSS', 'HIJKL'), (1, 'SPOS', 'HIJKL'), (2, 'TROO', 'IJKLM')):
    reset(); actor(kind=kind, st=4, hp=0)
    seen = []
    for f in range(12):
        frames()
        tex = sprite_uvs()
        assert len(tex) == 1
        name = next(n for n in B.MON_FRAMES[kind][-5:] if uv(n) == tex[0])
        if not seen or seen[-1] != name:
            seen.append(name)
            M.frames[-1].save(output / (name+'.png'))
    assert seen == [prefix+d+'0' for d in death], seen
    assert thing(0)['st'] == 5 and thing(0)['on'] == 1
    assert get('kills', 1) == 0  # a animacao nao contabiliza novamente uma morte
    actor(kind=kind, st=2, t=8)
    seen = set()
    for _ in range(8):
        frames()
        if thing(0)['st'] == 2:
            seen.add(sprite_uvs()[0])
    expected = B.MON_FRAMES[kind][4:7 if kind == 2 else 6]
    assert seen == {uv(n) for n in expected}
    assert thing(0)['st'] == 1
    actor(kind=kind, st=3, t=6)
    frames(6)
    assert thing(0)['st'] == 1
    put('hp', 1000)
    actor(kind=kind, st=1, t=0)
    seen = set()
    for _ in range(40):
        frames()
        if thing(0)['st'] == 1:
            seen.update(sprite_uvs())
    assert {uv(n) for n in B.MON_FRAMES[kind][:4]} <= seen
    print('PASS', prefix, 'caminhada/queda completas, cadaver persistente e saida de ataque/dor')

reset(); put('armor', 90)
actor(kind=14, hp=20)
actor(1, kind=14, hp=20, x=1106)
actor(2, kind=0, hp=20, y=-3506, st=3, t=250)
actor(3, kind=0, hp=60, y=-3396, st=3, t=250)
held = True
frames()
assert 0 < thing(0)['hp'] < 20 and thing(2)['hp'] == 20
seen = []
for _ in range(70):
    frames()
    state = thing(0)
    if state['st'] >= 4:
        held = False
    if state['st'] == 4:
        name = 'BEXP'+'ABCDE'[state['t']//2]+'0'
        if not seen or seen[-1] != name:
            seen.append(name)
            assert uv(name) in sprite_uvs()
            M.frames[-1].save(output / (name+'.png'))
    if not thing(0)['on'] and not thing(1)['on']:
        break
held = False
assert seen == ['BEXP'+f+'0' for f in 'ABCDE'], seen
assert thing(0)['on'] == thing(1)['on'] == 0
assert thing(2)['st'] in (4, 5) and get('kills', 1) == 1
assert thing(3)['hp'] == 60
assert get('hp') == 36 and get('armor') == 58
print('PASS tiro acerta barril mais proximo, cinco quadros, dano/armadura, alcance e reacao em cadeia')

reset()
R = L.R
pair = None
for cy, cx in zip(*np.nonzero(~R.open)):
    if cx < 5 or cy < 5 or cx >= R.nx-5 or cy >= R.ny-5:
        continue
    for dx, dy in ((4, 0), (0, 4)):
        if R.open[cy-dy, cx-dx] and R.open[cy+dy, cx+dx]:
            a = (R.x0+(cx-dx)*8+4, R.y0+(cy-dy)*8+4)
            b = (R.x0+(cx+dx)*8+4, R.y0+(cy+dy)*8+4)
            if L.m.sectors[int(R.sec[cy-dy, cx-dx])]['special'] not in (4, 5, 7, 16):
                pair = a, b
                break
    if pair:
        break
assert pair, 'nenhuma parede fina encontrada no WAD'
a, b = pair
put('px', a[0]); put('py', a[1]); put('hp', 200)
actor(x=b[0], y=b[1], kind=14, st=4, t=3, hp=0)
actor(1, x=a[0], y=a[1], kind=0, hp=60, st=3, t=250)
frames()
assert get('hp') == 200 and thing(1)['hp'] == 60, ('explosao atravessou parede', pair)
print('PASS parede bloqueia dano da explosao ao jogador e inimigo')

reset(); actor(kind=14, st=4, t=3, hp=0)
put('hp', 1)
frames(12)
assert get('over', 1) == 1 and thing(0)['on'] == 0
held = True; frames(2); held = False; frames(2)
assert get('over', 1) == 0 and get('hp') == 100
assert all(thing(i)['hp'] == 20 for i in range(defines['NTHING']) if thing(i)['kind'] == 14)
print('PASS explosao termina mesmo apos morte do jogador; reinicio restaura barris')
print('ALL GAMEPLAY TESTS PASSED (SIMULATED)')
