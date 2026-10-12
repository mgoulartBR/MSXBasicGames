"""SIMULATED: projectile/door logic on the compiled Z80 ROM, plus asset checks."""
import os
import re
import struct
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from e1m1_sim import make, symbols
import e1m1_pvs as P

root = Path(__file__).resolve().parent.parent
out = root / 'private' / 'interactions-simulated'
out.mkdir(exist_ok=True)
defs = {k: int(v) for k, v in re.findall(r'#define (\w+) (-?\d+)',
                                       (root / 'private/e1m1_assets.h').read_text())}
held = {}
M = make(lambda row: 255 ^ held.get(row, 0))
sym = symbols()
mem = M.m.memory


def frames(n=1):
    target = len(M.frames) + n
    for _ in range(3000000):
        M.m.ticks_to_stop = 32
        M.m.run()
        if len(M.frames) >= target:
            break
    assert len(M.frames) == target, ('frame timeout', target, len(M.frames))


def put(name, value, size=2, offset=0):
    at = sym[name] + offset
    mem[at:at+size] = (int(value) & ((1 << (size*8))-1)).to_bytes(size, 'little')


def get(name, size=2, offset=0):
    at = sym[name] + offset
    return int.from_bytes(mem[at:at+size], 'little', signed=size == 2)


def clear():
    held.clear()
    for i in range(defs['NTHING']):
        put('th', 0, 1, 11*i+9)
    for i in range(4):
        put('shots', 0, 1, 16*i+12)
    for name in ('door_pos', 'door_state', 'door_hold'):
        for i in range(defs['NDOOR']):
            put(name, 0, 1, i)
    for name in ('over', 'yaw', 'cool', 'fire_t', 'kills'):
        put(name, 0, 1)
    put('px', 1056); put('py', -3616); put('eye', 41)
    put('hp', 100); put('armor', 0); put('ammo', 50)
    frames(2)


def imp(x=1056, y=-3400):
    for offset, size, value in ((0, 2, x), (2, 2, y), (4, 2, 60),
                                (6, 1, 2), (7, 1, 1), (8, 1, 0),
                                (9, 1, 1), (10, 1, 1)):
        put('th', value, size, offset)


def shot(i=0):
    return struct.unpack_from('<hhhhhhBBBB', mem, sym['shots']+16*i)


def await_shot():
    imp()
    for _ in range(80):
        frames()
        if any(shot(i)[6] for i in range(4)):
            assert get('hp') == 100, 'ranged attack must not cause instant damage'
            put('th', 0, 1, 9)
            return next(i for i in range(4) if shot(i)[6])
    raise AssertionError('Imp never launched')


def capture(name):
    M.frames[-1].resize((768, 636)).save(out / (name + '.png'))


L = P.load(os.environ['E1M1_WAD'])
frames(2)
assert bytes(mem[0xD200:0xF000]) == M.rom[0x2000:0x3E00], 'boot-loaded code differs'
assert defs['NDOOR'] == 4 and defs['NSPR'] >= 63
clear()
i = await_shot()
start = shot(i)
capture('imp-flight')
frames(3)
assert shot(i)[:3] != start[:3] and get('hp') == 100
for _ in range(40):
    frames()
    if shot(i)[9]:
        break
assert shot(i)[9] and 76 <= get('hp') <= 91
impact_hp = get('hp')
capture('imp-impact')
frames(8)
assert not shot(i)[6] and get('hp') == impact_hp, 'impact must damage once, then expire'
print('PASS Imp: animated flight, no instant hit, impact damage once, explosion expires')

clear()
i = await_shot()
put('yaw', 64, 1); held[8] = 0x20
frames(20)
held.clear()
assert get('px') > 1100 and get('hp') == 100, 'keyboard movement must dodge the projectile'
put('shots', 1, 1, 16*i+14)
frames()
assert not shot(i)[6], 'lifetime must expire'
print('PASS Imp: keyboard dodge and bounded lifetime')

clear()
imp(y=-3556)
for _ in range(32):
    frames()
    if get('hp') < 100:
        break
assert get('hp') < 100 and not any(shot(i)[6] for i in range(4))
print('PASS Imp melee remains immediate; no projectile at melee range')

records = [struct.unpack_from('<hhhhBBBBB', mem, sym['doors']+13*i)
           for i in range(defs['NDOOR'])]
for i, (ax, ay, bx, by, sec, *_texture) in enumerate(records):
    cx, cy = (ax+bx)//2, (ay+by)//2
    nx, ny = (1, 0) if ax == bx else (0, 1)
    assert ax == bx or ay == by
    for side in (-1, 1):
        clear()
        put('px', cx+48*side*nx); put('py', cy+48*side*ny)
        put('eye', 41 if i == 0 else 17)
        yaw = (64 if side == -1 else 192) if nx else (0 if side == -1 else 128)
        put('yaw', yaw, 1)
        held[8] = 0x20; frames(8); held.clear()
        distance = (get('px')-cx)*nx+(get('py')-cy)*ny
        assert distance*side > 15 and get('door_pos', 1, i) == 0, ('closed door', i, side)
        if i == 0 and side == -1:
            capture('door-closed')
        held[2] = 0x40; frames(); held.clear()  # E, not Enter
        assert get('door_state', 1, i) == 1 and get('door_pos', 1, i) == 8
        frames(4)
        if i == 0 and side == -1:
            capture('door-opening')
        frames(5)
        assert get('door_state', 1, i) == 2 and get('door_pos', 1, i) >= 56
        held[8] = 0x20; frames(16); held.clear()
        distance = (get('px')-cx)*nx+(get('py')-cy)*ny
        assert distance*side < -15, ('open door passage', i, side, distance)
        if i == 0 and side == -1:
            capture('door-open')
        put('px', 1056); put('py', -3616); frames(92)
        assert get('door_pos', 1, i) == 0 and get('door_state', 1, i) == 0, ('close', i)
    print(f'PASS door {i}: E, movement blocked/allowed, closes; both sides')

clear()
ax, ay, bx, by, *_ = records[0]
cx, cy = (ax+bx)//2, (ay+by)//2
put('px', cx+100); put('py', cy)
for offset, size, value in ((0, 2, cx-30), (2, 2, cy), (4, 2, 40),
                            (6, 2, 10), (8, 2, 0), (10, 2, 0),
                            (12, 1, 1), (13, 1, 0), (14, 1, 100), (15, 1, 0)):
    put('shots', value, size, offset)
frames(3)
assert shot()[9] and get('hp') == 100, 'closed door must stop fireball'
print('PASS projectile collides with closed door, without damaging player behind it')

clear()
put('px', cx+10);put('py', cy)
put('door_pos',64,1,0);put('door_state',3,1,0)
frames(2)
assert get('door_pos',1,0)==72 and get('door_state',1,0)==2 and get('hp')==100
print('PASS closing door reverses before trapping/crushing the player')

masked = [p for p in L.pcs if p['kind'] == 'masked']
assert {298, 299, 300, 301, 302, 303, 464} <= {p['line'] for p in masked}
for texture in {p['tex'] for p in masked}:
    tile = L.tiles[texture]
    mask = L.alpha[tile['y']:tile['y']+tile['h'], tile['x']:tile['x']+tile['w']]
    if texture.startswith('BRNBIG'):
        assert mask.any() and not mask.all(), ('lost transparency', texture)
        stored=M.vram.reshape(1024,256)[512+tile['y']:512+tile['y']+tile['h'],tile['x']:tile['x']+tile['w']]
        assert np.array_equal(stored==0,~mask), ('lost converted VRAM alpha',texture)
assert np.unique(np.frombuffer(M.rom[(defs['VRAM_BANK0']+3)*16384:
                                    (defs['VRAM_BANK0']+4)*16384], np.uint8)
                 .reshape(64, 256)[48:64, :16]).size >= 3, 'NUKAGE3 tile must not be flat'
print('PASS masked linedefs and transparent BRNBIG atlas; multicolour NUKAGE3 tile')
for sector in (13, 55, 57, 61):
    clear()
    rows = np.argwhere(L.R.open & (L.R.sec == sector))
    y, x = rows[len(rows)//2]
    put('px', L.R.x0+int(x)*8+4); put('py', L.R.y0+int(y)*8+4)
    put('eye', L.m.sectors[sector]['floor']+41); put('tick', 31, 1)
    frames()
    assert get('hp') == 95, ('radiation damage', sector)
    capture(f'radiation-{sector}')
    for heading in range(0,256,16):
        put('yaw',heading,1);frames()
        for fi in range(M.gb[0x59]):
            indices,normal,attr=M.gf[fi]
            verts=[M.gv[i] for i in indices]
            if len({v[1] for v in verts})==1 and attr&128:
                uv=M.gt[fi]
                assert all(0<=u<=15 for u in uv[0::2]) and all(240<=v<=255 for v in uv[1::2]), ('floor UV',sector,heading,uv)
                assert all(abs(v[0])<5000 and abs(v[2])<6000 for v in verts), ('clipped floor coordinates',verts)
        assert M.gb[0x42]<=255 and M.gb[0x59]<=255
print('PASS damage in all four radiation sectors')
print('PASS floor clipping/UV and batch limits at 16 headings in every radiation sector')
print('ALL INTERACTION TESTS PASSED (SIMULATED)')
