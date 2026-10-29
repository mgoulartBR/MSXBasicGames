#!/usr/bin/env python3
"""Behavioural tests for the MSX2 skeleton, run on the simulator in msxsim.py.

Usage: test_smashtv.py ROM SYM [OUTDIR]     (SYM = symbol file written by z80asm.py --sym)
Exit status 0 = all passed. Screenshots go to OUTDIR when given.
"""
import sys, os, random
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from msxsim import Sim, FRAME_T

rom = open(sys.argv[1], 'rb').read()
SYM = {}
for line in open(sys.argv[2]):
    a, n = line.split()
    SYM[n] = int(a, 16)
OUT = sys.argv[3] if len(sys.argv) > 3 else None
if OUT: os.makedirs(OUT, exist_ok=True)

A = lambda n: SYM[n]
BUL, EN = A('BUL_BASE'), A('EN_BASE')
fails = []


def check(cond, msg):
    print(('  ok   ' if cond else '  FAIL ') + msg)
    if not cond: fails.append(msg)


def mk():
    return Sim(rom)


def shot(s, name):
    if OUT: s.screenshot(os.path.join(OUT, name + '.png'))


def enemies(s):
    return [(i, s.peek(EN + 0x20 + i), s.peek(EN + 0x30 + i)) for i in range(16) if s.peek(EN + i) == 1]


def bullets(s):
    return [(i, s.peek(BUL + 0x10 + i), s.peek(BUL + 0x20 + i), s.peek(BUL + 0x30 + i)) for i in range(16) if s.peek(BUL + i)]


def bcd(b): return (b >> 4) * 10 + (b & 15)
def score(s): return bcd(s.peek(A('score'))) + 100 * bcd(s.peek(A('score') + 1)) + 10000 * bcd(s.peek(A('score') + 2))


# glyphs and sprites as stored in the sheet (page 1)
def sheet_px(s, x, y):
    return s.vdp.px(x, 256 + y)


def expected_screen(s):
    """clean page + enemies at the positions they were last drawn at"""
    v = s.vdp
    img = {}
    for y in range(24, 184):
        for x in range(16, 240): img[(x, y)] = v.px(x, 512 + y)
    for i in range(16):
        if s.peek(EN + i) == 1 and s.peek(EN + 0x50 + i):
            ox, oy, fr = s.peek(EN + 0x50 + i), s.peek(EN + 0x60 + i), s.peek(EN + 0x70 + i) & 1
            for j in range(16):
                for k in range(16):
                    c = sheet_px(s, fr * 16 + k, 8 + j)
                    if c: img[(ox + k, oy + j)] = c
    return img


def screen_diff(s):
    img = expected_screen(s)
    v = s.vdp
    return sum(1 for (x, y), c in img.items() if v.px(x, y) != c)


def hud_ok(s):
    v = s.vdp
    digits = []
    for b in (2, 1, 0):
        byte = s.peek(A('score') + b)
        digits += [byte >> 4, byte & 15]
    digits.append(s.peek(A('lives')) & 15)
    xs = [8 + 8 * i for i in range(6)] + [232]
    bad = 0
    for d, x0 in zip(digits, xs):
        for j in range(8):
            for k in range(8):
                g = sheet_px(s, d * 8 + k, j)
                want = g if g else 14
                if v.px(x0 + k, 4 + j) != want: bad += 1
    return bad


def tick_frames(s, n_ticks):
    s.run_frames(3 * n_ticks, calm_hook(s) if getattr(s, 'calm', False) else None)


def calm_hook(s):
    """keeps enemies away and the player invulnerable, for tests that are not about combat"""
    def hook(frame):
        s.m.memory[A('spawn_timer')] = 255
        s.m.memory[A('p_inv')] = 251      # odd: the tick decrements it to an even (visible) value
    return hook


# ---------------------------------------------------------------------------
print('boot')
s = mk()
s.run_frames(20)
v = s.vdp
check(v.r[1] == 0x62, f'R#1 = {v.r[1]:02X} (display + VBlank IRQ + 16x16 sprites)')
check(v.pal[4] == (7, 7, 7) and v.pal[8] == (1, 2, 7), 'palette loaded')
check(v.px(100, 100) == 2 and v.px(10, 100) == 3 and v.px(200, 2) == 14, 'arena: floor / wall / HUD bar colours')
check(s.peek(0xFC9E) in range(18, 22) and s.irq_count == s.peek(0xFC9E), f'one interrupt per frame (JIFFY={s.peek(0xFC9E)}, irqs={s.irq_count})')
check(not v.violations, f'no VDP misuse: {v.violations[:3]}')
check(hud_ok(s) == 0, 'HUD shows 000000 and 3 lives')
shot(s, '1_boot')

print('movement and clamping')
s = mk(); s.calm = True; s.run_frames(10)
x0 = s.peek(A('p_x')); y0 = s.peek(A('p_y'))
s.input['cursor'] = 3          # right
tick_frames(s, 10)
x1 = s.peek(A('p_x'))
check(x1 - x0 == 30, f'right: {x0} -> {x1} (3 px per tick)')
tick_frames(s, 60)
check(s.peek(A('p_x')) == 224, f'clamped at the right wall (x={s.peek(A("p_x"))})')
s.input['cursor'] = 7          # left
tick_frames(s, 100)
check(s.peek(A('p_x')) == 16, f'clamped at the left wall (x={s.peek(A("p_x"))})')
s.input['cursor'] = 1          # up
tick_frames(s, 100)
check(s.peek(A('p_y')) == 24, 'clamped at the top wall')
s.input['cursor'] = 5          # down
tick_frames(s, 100)
check(s.peek(A('p_y')) == 168, 'clamped at the bottom wall')
s.input['cursor'] = 0
s.input['joy1'] = 2            # up-right on joystick 1
y = s.peek(A('p_y')); tick_frames(s, 5)
check(s.peek(A('p_y')) == y - 15, 'joystick 1 diagonal moves both axes')

print('shooting')
s = mk(); s.calm = True; s.run_frames(10)
s.keys = {'D'}
tick_frames(s, 4)
b = bullets(s)
check(len(b) >= 2 and all(d == 8 for _, _, _, d in b), f'aim right (D) spawns bullets going right: {b[:3]}')
xs = [x for _, x, _, _ in b]
tick_frames(s, 1)
xs2 = [x for _, x, _, _ in bullets(s)]
check(max(xs2) > max(xs) or len(xs2) < len(xs), 'bullets advance 8 px per tick')
attr = s.vdp.vram[0x7600:0x7600 + 8]
check(attr[0] == s.peek(A('p_y')) - 1 and attr[1] == s.peek(A('p_x')), f'player sprite attributes track position {list(attr[:2])}')
check(s.vdp.vram[0x7600 + 4] != 217, 'first bullet plane visible in the sprite attribute table')
s.keys = set(); s.input['trig'] = {0}   # space only: shoots in the facing direction
s.input['cursor'] = 7; tick_frames(s, 1); s.input['cursor'] = 0
tick_frames(s, 3)
check(any(d == 4 for _, _, _, d in bullets(s)), 'space shoots along the last facing direction')
tick_frames(s, 40)
check(all(16 <= x <= 232 and 24 <= y <= 176 for _, x, y, _ in bullets(s)), 'bullets are removed at the walls')
shot(s, '2_shooting')

print('enemies, bullets, score and the blit pipeline (bot plays 900 ticks)')
s = mk(); s.run_frames(10)
random.seed(3)
worst_diff = 0; diffs_checked = 0; hud_bad = 0; hud_checked = 0
kills = 0; last_score = 0; max_enemies = 0
def bot(frame):
    en = enemies(s)
    px, py = s.peek(A('p_x')), s.peek(A('p_y'))
    s.keys = set()
    if en:
        i, ex, ey = min(en, key=lambda e: abs(e[1] - px) + abs(e[2] - py))
        dx, dy = ex - px, ey - py
        if abs(dx) > 6 or abs(dy) > 6:
            if abs(dx) > abs(dy) * 2: s.keys.add('D' if dx > 0 else 'A')
            elif abs(dy) > abs(dx) * 2: s.keys.add('S' if dy > 0 else 'W')
            else: s.keys |= {('D' if dx > 0 else 'A'), ('S' if dy > 0 else 'W')}
    # keep away from the walls, drift towards the centre
    s.input['cursor'] = 3 if px < 100 else 7 if px > 140 else 0
for t in range(900):
    bot(0)
    s.run_frames(3)
    max_enemies = max(max_enemies, len(enemies(s)))
    sc = score(s)
    if sc > last_score: kills += (sc - last_score) // 10
    last_score = sc
    if s.peek(A('lives')) == 3 and sc < last_score: pass
    if t % 10 == 5:      # between ticks: all commands have completed
        d = screen_diff(s); diffs_checked += 1; worst_diff = max(worst_diff, d)
        if s.peek(A('score_dirty')) == 0:
            hud_checked += 1; hud_bad += hud_ok(s)
    if t == 300: shot(s, '3_play')
    if t == 600: shot(s, '4_play')
check(max_enemies >= 3, f'enemies spawn (max alive = {max_enemies})')
check(kills >= 5, f'bot kills enemies and the score counts them (kills = {kills}, score = {score(s)})')
check(worst_diff == 0, f'screen == clean page + enemies at drawn positions in {diffs_checked} checks (worst diff {worst_diff} px)')
check(hud_checked > 0 and hud_bad == 0, f'HUD digits always match score/lives ({hud_checked} checks, {hud_bad} px off)')
check(not s.vdp.violations, f'no VDP misuse: {sorted(set(s.vdp.violations))[:3]}')
check((s.irq_count - s.peek(0xFC9E)) % 256 <= 1 and abs(s.irq_count - s.frame) <= 2, f'one interrupt per frame, no VBlank storm (irqs={s.irq_count}, frames={s.frame})')

print('player damage, invulnerability and restart')
s = mk(); s.run_frames(10)
lives_seen = []; prev = 3; restarted = False; score_when = None
for t in range(900):
    s.run_frames(3)
    l = s.peek(A('lives'))
    if l != prev:
        lives_seen.append((l, s.peek(A('p_inv'))))
        prev = l
check(len(lives_seen) >= 3 and lives_seen[0][0] == 2, f'standing still: enemies hurt the player, lives {[l for l,_ in lives_seen]}')
check(any(inv == 40 or inv == 39 for l, inv in lives_seen if l in (2, 1)), 'a hit grants about 2 s of invulnerability')
check(3 in [l for l, _ in lives_seen[1:]], 'game over restarts with 3 lives')
check(screen_diff(s) < 400, 'screen is clean after the restart (enemies erased)')

print('profiler is off by default and F1 toggles it')
s = mk(); s.run_frames(10)
check(not s.border_log or all(c == 0 for _, c in s.border_log), 'border stays black by default')
s.keys = {'F1'}; s.run_frames(6); s.keys = set(); s.run_frames(6)
check(s.peek(A('profile_on')) == 1, 'F1 turns the profiler on')
check(any(c in (2, 6) for _, c in s.border_log), 'border colours change while it is on')
s.keys = {'F1'}; s.run_frames(6); s.keys = set(); s.run_frames(6)
check(s.peek(A('profile_on')) == 0 and s.vdp.r[7] == 0, 'F1 again turns it off and the border goes back to black')

print('timing (rough: command speed is an estimate)')
s = mk(); s.run_frames(10); s.m.memory[A('profile_on')] = 1
random.seed(5)
for t in range(300):
    bot(0); s.run_frames(3)
log = s.border_log
seg = {2: 0, 6: 0, 0: 0}
for (t0, c), (t1, _) in zip(log, log[1:]):
    if c in seg and t0 > 10 * FRAME_T: seg[c] += t1 - t0
total = sum(seg.values())
ticks = 300
print(f'  logic  {seg[2]/ticks:9.0f} T/tick   render {seg[6]/ticks:9.0f} T/tick   budget {3*FRAME_T} T/tick')
check(seg[2] + seg[6] < 3 * FRAME_T * ticks * 0.9, f'work fits in the tick budget ({100*(seg[2]+seg[6])/(3*FRAME_T*ticks):.1f} % busy)')

print()
print('ALL PASSED' if not fails else f'{len(fails)} FAILED: ' + '; '.join(fails))
sys.exit(1 if fails else 0)
