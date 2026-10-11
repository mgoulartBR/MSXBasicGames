"""TAB e texto do E1M1 no harness Z80 (SIMULATED, nao mede FPS). Requer a ROM privada compilada."""
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(__file__))
from e1m1_sim import make, symbols

held = False
def keyboard(row):
    return 0xF7 if row == 7 and held else 0xFF

M = make(keyboard)
sym = symbols()
mem = M.m.memory
header = open(os.path.join(os.path.dirname(__file__), '..', 'private', 'e1m1_assets.h')).read()
fontchars = re.search(r'#define FONT_CHARS "([^"]+)"', header).group(1)
fontrow = int(re.search(r'#define FONT_GRAY (\d+)', header).group(1))
black = int(re.search(r'#define C_BLACK (\d+)', header).group(1))

def frames(n):
    target = len(M.frames) + n
    M.run_frames(target)
    assert len(M.frames) == target, 'ROM nao produziu os quadros esperados'

def rd16(name):
    v = int.from_bytes(mem[sym[name]:sym[name]+2], 'little')
    return v - 65536 if v >= 32768 else v

def wr16(name, value):
    mem[sym[name]:sym[name]+2] = (value & 65535).to_bytes(2, 'little')

def panel(machine=M):
    page = (machine.vreg[2] >> 5) & 3
    return machine.vram.reshape(1024, 256)[page*256+12:page*256+23, 2:150].copy()

def check_text():
    angle = (90 - int(mem[sym['yaw']])*45//32) % 360
    line = f"X{rd16('px')} Y{rd16('py')} A{angle}"
    expected = np.full((11, 148), black, np.uint8)
    atlas = M.vram.reshape(1024, 256)
    for i, ch in enumerate(line):
        gx = fontchars.index(ch)*6
        glyph = atlas[fontrow:fontrow+7, gx:gx+5]
        expected[2:9, 2+6*i:7+6*i] = np.where(glyph == 0, black, glyph)
    assert np.array_equal(panel(), expected), f'texto incorreto: {line}'

frames(3)
assert mem[sym['dbg']] == 0
control = make()
held = True
frames(2)
assert mem[sym['dbg']] == 1
check_text()
M.frames[-1].resize((768, 636)).save(os.path.join(os.path.dirname(__file__), '..', 'private', 'tab_simulated.png'))
frames(5)
assert mem[sym['dbg']] == 1, 'TAB segurado alternou mais de uma vez'
check_text()
held = False
frames(2)
held = True
frames(2)
for _ in range(3):
    frames(1)
    assert mem[sym['dbg']] == 0
    control.run_frames(len(M.frames))
    assert np.array_equal(panel(), panel(control)), 'overlay deixou vestigios em uma pagina'
held = False
frames(2)
held = True
frames(2)
assert mem[sym['dbg']] == 1
held = False
frames(2)
print('PASS TAB: borda, tecla segurada, reapertar e limpeza das duas paginas')

# Congela a logica; o texto continua sendo executado pela ROM. O desenho 3D
# nao e necessario para conferir todos os graus e limites dos inteiros.
mem[sym['over']] = 1
M.run = lambda ctrl: None
for yaw in range(256):
    mem[sym['yaw']] = yaw
    frames(2)
    check_text()
print('PASS angulo Doom: todos os 256 valores de yaw, incluindo wrap e pontos cardeais')
for x, y in ((0, 0), (32767, -32768), (-32768, 32767), (-1, 1), (1, -1)):
    wr16('px', x)
    wr16('py', y)
    frames(2)
    check_text()
print('PASS coordenadas: zero, sinais e extremos de s16')
held = True
frames(2)
assert mem[sym['dbg']] == 0
held = False
frames(2)
held = True
frames(2)
assert mem[sym['dbg']] == 1
check_text()
print('PASS TAB no fim de jogo')
