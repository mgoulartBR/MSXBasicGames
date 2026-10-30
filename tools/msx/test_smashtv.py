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
    return s.vdp.px(x, 768 + y)


def front(s):
    return s.peek(A('front_page'))


def expected_screen(s):
    """clean page + the enemies recorded as drawn in the page being displayed"""
    v = s.vdp
    img = {}
    fp = front(s)
    ox_off, oy_off = (0x50, 0x60) if fp == 0 else (-0x20, -0x10)
    for y in range(24, 184):
        for x in range(16, 240): img[(x, y)] = v.px(x, 512 + y)
    for i in range(16):
        ox = s.peek(EN + ox_off + i)
        if s.peek(EN + i) == 1 and ox:
            oy, fr = s.peek(EN + oy_off + i), s.peek(EN + 0x70 + i) & 1
            kind = s.peek(EN - 0x30 + i)
            for j in range(16):
                for k in range(16):
                    c = sheet_px(s, (kind * 2 + fr) * 16 + k, 8 + j)
                    if c: img[(ox + k, oy + j)] = c
    return img


def screen_diff(s):
    img = expected_screen(s)
    v = s.vdp
    fy = 256 * front(s)
    return sum(1 for (x, y), c in img.items() if v.px(x, fy + y) != c)


def hud_ok(s):
    v = s.vdp
    digits = []
    for b in (2, 1, 0):
        byte = s.peek(A('score') + b)
        digits += [byte >> 4, byte & 15]
    rb = s.peek(A('room_bcd'))
    wu = s.peek(A('p_wunits'))
    digits += [rb >> 4, rb & 15, s.peek(A('p_weapon')) + 1, wu >> 4, wu & 15, s.peek(A('lives')) & 15]
    xs = [8 + 8 * i for i in range(6)] + [120, 128, 152, 168, 176, 232]
    bad = 0
    for d, x0 in zip(digits, xs):
        for j in range(8):
            for k in range(8):
                g = sheet_px(s, d * 8 + k, j)
                want = g if g else 14
                if v.px(x0 + k, 256 * front(s) + 4 + j) != want: bad += 1
    return bad


def quiet(s, hook=None):
    """advance to just after the next page flip: the one moment where the screen and the game state agree
    (between the logic of a tick and the flip that shows it, the state is ahead of the screen)"""
    start = front(s)
    for _ in range(12):
        s.run_frames(1, hook)
        if front(s) != start:
            return


def tick_frames(s, n_ticks):
    s.run_frames(3 * n_ticks, calm_hook(s) if getattr(s, 'calm', False) else None)


def calm_hook(s):
    """keeps enemies away and the player invulnerable, for tests that are not about combat"""
    def hook(frame):
        s.m.memory[A('scr_wait')] = 255          # the room script stays asleep
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
check(v.px(100, 356) == 2 and v.px(100, 612) == 2, 'second buffer (page 1) and clean copy (page 2) hold the arena too')
check(s.peek(0xFC9E) in range(18, 22) and s.irq_count == s.peek(0xFC9E), f'one interrupt per frame (JIFFY={s.peek(0xFC9E)}, irqs={s.irq_count})')
check(not v.violations, f'no VDP misuse: {v.violations[:3]}')
check(hud_ok(s) == 0, 'HUD shows 000000, room 01, weapon 1 (00 s) and 3 lives')
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
    if s.peek(A('exit_open')):                  # head for the gap in the top wall
        s.input['cursor'] = 1 if 114 <= px <= 126 else (3 if px < 114 else 7)
        return
    if en:
        i, ex, ey = min(en, key=lambda e: abs(e[1] - px) + abs(e[2] - py))
        dx, dy = ex - px, ey - py
        if abs(dx) > 6 or abs(dy) > 6:
            if abs(dx) > abs(dy) * 2: s.keys.add('D' if dx > 0 else 'A')
            elif abs(dy) > abs(dx) * 2: s.keys.add('S' if dy > 0 else 'W')
            else: s.keys |= {('D' if dx > 0 else 'A'), ('S' if dy > 0 else 'W')}
    # keep away from the walls, drift towards the centre - or go and take a floor item
    s.input['cursor'] = 3 if px < 100 else 7 if px > 140 else 0
    floor = [(s.peek(A('IT_BASE') + 0x10 + i), s.peek(A('IT_BASE') + 0x20 + i)) for i in range(6) if s.peek(A('IT_BASE') + i)]
    if floor:
        ix_, iy_ = min(floor, key=lambda t: abs(t[0] - px) + abs(t[1] - py))
        dx, dy = ix_ - px, iy_ - py
        h = 3 if dx > 4 else 7 if dx < -4 else 0
        v = 5 if dy > 4 else 1 if dy < -4 else 0
        s.input['cursor'] = {(3, 0): 3, (7, 0): 7, (0, 5): 5, (0, 1): 1, (3, 1): 2, (3, 5): 4, (7, 5): 6, (7, 1): 8}.get((h, v), 0)
for t in range(900):
    bot(0)
    quiet(s)
    max_enemies = max(max_enemies, len(enemies(s)))
    sc = score(s)
    if sc > last_score: kills += (sc - last_score) // 10
    last_score = sc
    if t % 10 == 5:      # right after a flip: all commands have completed
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

print('double buffering: the displayed page never changes while it is shown')
s = mk(); s.run_frames(10)
samples = []
def sample(frame):
    page = (s.vdp.r[2] >> 5) & 3
    top = page * 0x8000
    samples.append((frame, page, hash(bytes(s.vdp.vram[top:top + 212 * 128])), hash(bytes(s.vdp.vram[0x7400:0x7680]))))
random.seed(11)
def bot_hook(frame):
    bot(0); sample(frame)
n0 = len(s.r2_log)
for t in range(300):
    bot(0)
    s.run_frames(3, sample)
torn = [(a, b) for a, b in zip(samples, samples[1:]) if a[1] == b[1] and a[2] != b[2]]
flips = [(a, b) for a, b in zip(samples, samples[1:]) if a[1] != b[1]]
spr_early = [(a, b) for a, b in zip(samples, samples[1:]) if a[3] != b[3] and a[1] == b[1]]
check(len(torn) == 0, f'no tearing: displayed page unchanged between flips ({len(samples)} frames sampled, {len(torn)} violations)')
check(250 <= len(flips) <= 310, f'one page flip per tick ({len(flips)} flips in 300 ticks)')
check(len(spr_early) == 0, f'sprite attributes only change together with a page flip ({len(spr_early)} violations)')
r2 = s.r2_log[n0:]
late = [t % FRAME_T for t, _ in r2 if t % FRAME_T > 6000]
check(len(r2) >= 250 and not late, f'R#2 is written right after the VBlank starts (worst {max(t % FRAME_T for t, _ in r2)} T into the frame)')
check(set(v & 0x60 for _, v in r2) == {0x00, 0x20}, 'only pages 0 and 1 are ever displayed')


# ---------------------------------------------------------------------------
print('script machine (hand-made scripts injected into RAM)')
OPC = {n[3:]: SYM[n] for n in SYM if n.startswith('OP_') and n != 'OP_COUNT'}
SCR = 0xD100


def safe_hook(s):
    def hook(frame):
        s.m.memory[A('p_inv')] = 251      # invulnerable, so lives never change behind the HUD's back
    return hook


def run_ticks(s, n):
    done = 0
    last = s.peek(A('last_jiffy'))
    hook = safe_hook(s)
    while done < n:
        s.run_frames(1, hook)
        cur = s.peek(A('last_jiffy'))
        if cur != last:
            done += 1
            last = cur


def alive(s):
    return [(i, s.peek(EN - 0x30 + i), s.peek(EN + 0x20 + i), s.peek(EN + 0x30 + i)) for i in range(16) if s.peek(EN + i) == 1]


def load_script(s, code, at=SCR, more=()):
    m = s.m.memory
    for i, b in enumerate(code): m[at + i] = b
    for addr, blob in more:
        for i, b in enumerate(blob): m[addr + i] = b
    for i in range(16): m[EN + i] = 0; m[BUL + i] = 0
    m[A('scr_pc')], m[A('scr_pc') + 1] = at & 255, at >> 8
    for n in ('scr_wait', 'scr_done', 'scr_depth', 'lp_depth', 'exit_open'): m[A(n)] = 0


def fresh():
    s = mk()
    s.run_frames(8)
    return s


def w16(a): return [a & 255, a >> 8]


# WAIT n: the next opcode runs exactly n ticks later
s = fresh()
load_script(s, [OPC['SPAWN'], 0, OPC['WAIT'], 5, OPC['SPAWN'], 1, OPC['END']])
seen = []
for t in range(14):
    run_ticks(s, 1)
    if len(alive(s)) > len(seen): seen.append(t)
check(len(seen) == 2 and seen[1] - seen[0] == 5, f'WAIT 5 resumes 5 ticks later (spawns at ticks {seen})')
check(s.peek(A('scr_done')) == 1, 'END stops the script')

# LOOP / nested LOOP
s = fresh()
load_script(s, [OPC['LOOP'], 5, OPC['SPAWN'], 0, OPC['ENDLOOP'], OPC['END']])
run_ticks(s, 3)
check(len(alive(s)) == 5 and s.peek(A('lp_depth')) == 0, f'LOOP 5 spawns 5 enemies ({len(alive(s))}) and leaves the loop stack empty')
s = fresh()
load_script(s, [OPC['LOOP'], 2, OPC['LOOP'], 3, OPC['SPAWN'], 1, OPC['ENDLOOP'], OPC['ENDLOOP'], OPC['END']])
run_ticks(s, 3)
check(len(alive(s)) == 6 and all(k == 1 for _, k, _, _ in alive(s)), 'nested loops: 2 x 3 = 6 enemies of the requested kind')

# CALL / RET
s = fresh()
sub = SCR + 0x20
load_script(s, [OPC['CALL']] + w16(sub) + [OPC['CALL']] + w16(sub) + [OPC['END']],
            more=[(sub, [OPC['SPAWN'], 0, OPC['SPAWN'], 1, OPC['RET']])])
run_ticks(s, 3)
check(sorted(k for _, k, _, _ in alive(s)) == [0, 0, 1, 1] and s.peek(A('scr_depth')) == 0, 'CALL/RET: the sub-script ran twice and the call stack is empty')

# CALLR picks among the sub-scripts at random
counts = {0: 0, 1: 0, 2: 0}
for trial in range(60):
    if trial == 0:
        s = fresh()
    pick = [SCR + 0x20, SCR + 0x28, SCR + 0x30]
    load_script(s, [OPC['CALLR'], 3] + sum((w16(a) for a in pick), []) + [OPC['END']],
                more=[(a, [OPC['SPAWN'], k, OPC['RET']]) for k, a in enumerate(pick)])
    run_ticks(s, 2)
    for _, k, _, _ in alive(s): counts[k] += 1
check(sum(counts.values()) == 60 and min(counts.values()) >= 8, f'CALLR chooses all three sub-scripts, never none or two ({counts})')

# WAITCLEAR
s = fresh()
load_script(s, [OPC['SPAWN'], 0, OPC['WAITCLEAR'], 0xFF, OPC['SPAWN'], 1, OPC['END']])
run_ticks(s, 12)
check(len(alive(s)) == 1 and s.peek(A('scr_done')) == 0, 'WAITCLEAR $FF blocks while an enemy lives')
s.m.memory[EN + alive(s)[0][0]] = 0
run_ticks(s, 3)
check([k for _, k, _, _ in alive(s)] == [1], 'killing it lets the script go on')
s = fresh()
load_script(s, [OPC['SPAWN'], 0, OPC['SPAWN'], 1, OPC['WAITCLEAR'], 0, OPC['SPAWN'], 2, OPC['END']])
run_ticks(s, 6)
check(sorted(k for _, k, _, _ in alive(s)) == [0, 1], 'WAITCLEAR kind waits for that kind only (third spawn not yet)')
for i, k, _, _ in alive(s):
    if k == 0: s.m.memory[EN + i] = 0
run_ticks(s, 3)
check(sorted(k for _, k, _, _ in alive(s)) == [1, 2], 'with the grunt gone the runner may stay alive and the tank spawns')

# SPAWN with a full pool retries until a slot frees up
s = fresh()
load_script(s, [OPC['SPAWN'], 2, OPC['END']])
for i in range(16):
    s.m.memory[EN + i] = 1; s.m.memory[EN + 0x10 + i] = 9; s.m.memory[EN + 0x20 + i] = 40 + 8 * i; s.m.memory[EN + 0x30 + i] = 60
    s.m.memory[EN - 0x30 + i] = 0
pc0 = s.peekw(A('scr_pc'))
run_ticks(s, 6)
check(s.peekw(A('scr_pc')) == pc0 and s.peek(A('scr_done')) == 0, 'SPAWN with a full pool waits on the opcode')
s.m.memory[EN + 7] = 0
run_ticks(s, 3)
check(s.peek(EN + 7) == 1 and s.peek(EN - 0x30 + 7) == 2 and s.peek(A('scr_done')) == 1, 'a freed slot is used for the pending spawn')

# SPAWNAT
s = fresh()
load_script(s, [OPC['SPAWNAT'], 2, 100, 60, OPC['END']])
run_ticks(s, 1)
a = alive(s)
check(len(a) == 1 and a[0][1] == 2 and abs(a[0][2] - 100) <= 1 and abs(a[0][3] - 60) <= 1, f'SPAWNAT puts a tank at (100, 60): {a}')

# EXIT_OPEN draws the gap on both display pages and the clean page
s = fresh()
load_script(s, [OPC['EXIT_OPEN'], OPC['END']])
run_ticks(s, 3); s.run_frames(2)
v = s.vdp
check(s.peek(A('exit_open')) == 1 and all(v.px(120, y) == 7 for y in (20, 276, 532)), 'EXIT_OPEN: yellow gap in pages 0, 1 and in the clean page')

# a broken script ends instead of running wild
for name, code in (('unknown opcode', [0xEE]), ('RET without CALL', [OPC['RET']]), ('ENDLOOP without LOOP', [OPC['ENDLOOP']]),
                   ('call stack overflow', [OPC['CALL']] + w16(SCR)),
                   ('loop stack overflow', [OPC['LOOP'], 2] * 5), ('CALLR with n = 0', [OPC['CALLR'], 0])):
    s = fresh()
    load_script(s, code)
    j0 = s.peek(0xFC9E)
    run_ticks(s, 4)
    check(s.peek(A('scr_done')) == 1, f'{name}: the script ends')
s = fresh()
load_script(s, [OPC['JUMP']] + w16(SCR))
run_ticks(s, 10)
check(s.peek(A('scr_done')) == 0 and s.peek(A('last_jiffy')) != 0, 'an infinite JUMP loop does not freeze the game (32 opcodes per tick at most)')

# ---------------------------------------------------------------------------
print('rooms: the real scripts, door, next room (bot plays with extra lives)')
s = mk(); s.run_frames(10)
random.seed(21)
rooms_seen = [s.peek(A('room_bcd'))]
checks_done = 0; door_bad = 0; clean_bad = 0; spawned_kinds = set(); hud_room_bad = 0
tick_n = 0
hook = safe_hook(s)
for t in range(9000):
    bot(0)
    quiet(s, hook)
    for _, k, _, _ in alive(s): spawned_kinds.add(k)
    r = s.peek(A('room_bcd'))
    if r != rooms_seen[-1]:
        rooms_seen.append(r)
        quiet(s, hook); quiet(s, hook)
        checks_done += 1
        fy = 256 * front(s)
        door_bad += s.vdp.px(120, fy + 20) != 3                    # the door is closed again
        clean_bad += screen_diff(s) != 0 or len(alive(s)) > 1       # the old room's enemies are gone from the screen
        if s.peek(A('score_dirty')) == 0: hud_room_bad += hud_ok(s)
        if s.peek(A('p_y')) < 100: clean_bad += 1                   # player was moved back from the door
    if len(rooms_seen) >= 6: break
check(rooms_seen[:6] == [1, 2, 3, 4, 1, 2], f'rooms advance 1 -> 2 -> 3 -> 4 and wrap to 1 (seen {rooms_seen})')
check(spawned_kinds == {0, 1, 2}, f'all three enemy kinds appeared in the real scripts ({sorted(spawned_kinds)})')
check(checks_done >= 5 and door_bad == 0, f'the door is closed at the start of every room ({checks_done} rooms)')
check(clean_bad == 0, 'previous enemies are erased and the player restarts at the bottom')
check(hud_room_bad == 0, 'the HUD shows the room number')


# ---------------------------------------------------------------------------
print('weapons')
BUL = A('BUL_BASE')


def put_enemy(s, slot, kind, x, y, hp=None):
    m = s.m.memory
    hp = [1, 1, 3][kind] if hp is None else hp
    m[EN + slot] = 1; m[EN - 0x30 + slot] = kind; m[EN + 0x10 + slot] = hp
    m[EN + 0x20 + slot] = x; m[EN + 0x30 + slot] = y
    m[EN + 0x50 + slot] = 0; m[EN - 0x20 + slot] = 0; m[EN + 0x70 + slot] = 0


def put_bullet(s, slot, x, y, d, btype=1, dmg=1):
    m = s.m.memory
    m[BUL + slot] = btype; m[BUL + 0x10 + slot] = x; m[BUL + 0x20 + slot] = y
    m[BUL + 0x30 + slot] = d; m[BUL + 0x40 + slot] = dmg


def sleepy(s):
    """script asleep, player invulnerable"""
    s.m.memory[A('scr_wait')] = 255


def tick_calm(s, n, god=True):
    """script asleep; god = the player cannot be hurt"""
    h = safe_hook(s)
    def hook(f):
        if god: h(f)
        s.m.memory[A('scr_wait')] = 255
    done = 0; last = s.peek(A('last_jiffy'))
    while done < n:
        s.run_frames(1, hook)
        cur = s.peek(A('last_jiffy'))
        if cur != last: done += 1; last = cur


def set_weapon(s, w, units=0x99):
    s.m.memory[A('p_weapon')] = w; s.m.memory[A('p_wunits')] = units; s.m.memory[A('p_wsub')] = 20


shots = {}
for w in range(4):
    s = fresh()
    load_script(s, [OPC['END']])
    set_weapon(s, w)
    s.m.memory[A('p_x')] = 100; s.m.memory[A('p_y')] = 100
    s.keys = {'D'}
    tick_calm(s, 6)
    shots[w] = bullets(s)
exp = {0: (2, 3), 1: (5, 7), 2: (5, 7), 3: (2, 3)}
for w, name in enumerate(['pistol', 'spread', 'rapid', 'piercing']):
    n = len(shots[w])
    check(exp[w][0] <= n <= exp[w][1], f'{name}: {n} shots in 6 ticks (expected {exp[w][0]}..{exp[w][1]})')
check(set(d for _, _, _, d in shots[1]) == {8, 9, 10}, f'spread aimed right fires right + up-right + down-right ({sorted(set(d for _, _, _, d in shots[1]))})')
check(all(s_.__class__ for s_ in shots[3]) and all(s.peek(BUL + i) in (0, 2) for i in range(16)), 'piercing shots are type 2')

# spread around other directions, and a non-direction aim fires straight only
for aim, want in (({'W', 'D'}, {9, 1, 8}), ({'S', 'A'}, {6, 2, 4}), ({'W', 'S'}, {3})):
    s = fresh(); load_script(s, [OPC['END']]); set_weapon(s, 1)
    s.m.memory[A('p_x')] = 100; s.m.memory[A('p_y')] = 100
    s.keys = aim; tick_calm(s, 2)
    got = set(d for _, _, _, d in bullets(s))
    check(got == want, f'spread aim {sorted(aim)}: directions {sorted(got)}')

# damage and piercing
s = fresh(); load_script(s, [OPC['END']])
s.m.memory[A('p_x')] = 220; s.m.memory[A('p_y')] = 100
put_enemy(s, 0, 0, 100, 100); put_enemy(s, 1, 0, 130, 100)
put_bullet(s, 0, 70, 104, 8, btype=1)           # ordinary shot
tick_calm(s, 14)
check(s.peek(EN + 0) != 1 and s.peek(EN + 1) == 1, 'an ordinary shot kills the first grunt and is used up; the second one survives')
s = fresh(); load_script(s, [OPC['END']])
s.m.memory[A('p_x')] = 220; s.m.memory[A('p_y')] = 100
put_enemy(s, 0, 0, 100, 100); put_enemy(s, 1, 0, 130, 100)
put_bullet(s, 0, 70, 104, 8, btype=2, dmg=2)    # piercing shot
tick_calm(s, 14)
check(s.peek(EN + 0) != 1 and s.peek(EN + 1) != 1 and s.peek(BUL + 0) == 2, 'a piercing shot kills both grunts and keeps flying')
for btype, dmg, hp0, want_hp, want_dead in ((1, 1, 3, 2, False), (2, 2, 3, 1, False), (2, 2, 2, 0, True), (1, 1, 1, 0, True)):
    s = fresh(); load_script(s, [OPC['END']])
    s.m.memory[A('p_x')] = 20; s.m.memory[A('p_y')] = 170
    put_enemy(s, 0, 2, 100, 100, hp0)
    put_bullet(s, 0, 100, 104, 8, btype=btype, dmg=dmg)
    tick_calm(s, 1)
    hp = s.peek(EN + 0x10)
    dead = s.peek(EN) != 1
    check(dead == want_dead and (dead or hp == want_hp), f'tank hp {hp0} hit by damage {dmg}: {"dead" if dead else "hp " + str(hp)}')

# weapon timer
s = fresh(); load_script(s, [OPC['END']])
set_weapon(s, 2, 0x02)
s.m.memory[A('p_wsub')] = 3
tick_calm(s, 3)
check(s.peek(A('p_wunits')) == 0x01 and s.peek(A('p_wsub')) == 20 and s.peek(A('p_weapon')) == 2, 'the weapon timer counts seconds down in BCD (20 ticks each)')
tick_calm(s, 21)
check(s.peek(A('p_weapon')) == 0 and s.peek(A('p_wunits')) == 0, 'when the time is up the pistol is back')
s.run_frames(9)
check(hud_ok(s) == 0, 'the HUD follows the weapon and its time')

# ---------------------------------------------------------------------------
print('items')


def drop_and_take(item, setup=None, ticks=3):
    s = fresh()
    load_script(s, [OPC['DROP'], item, 120, 100, OPC['END']])
    s.m.memory[A('p_x')] = 120; s.m.memory[A('p_y')] = 100
    if setup: setup(s)
    tick_calm(s, ticks, god=False)       # no enemies in the room, so nothing can hurt the player
    return s


s = drop_and_take(ITEM := SYM['ITEM_SPREAD'])
check(s.peek(A('p_weapon')) == 1 and s.peek(A('p_wunits')) in (0x15, 0x14) and s.peek(A('IT_BASE')) == 0, 'weapon item: spread for 15 s, and the item is gone')
s = drop_and_take(SYM['ITEM_PIERCE'])
check(s.peek(A('p_weapon')) == 3, 'weapon item: piercing')
s = drop_and_take(SYM['ITEM_LIFE'], lambda s: s.m.memory.__setitem__(A('lives'), 2))
s2 = s
check(s.peek(A('lives')) == 3, 'life item: +1 life')
s = drop_and_take(SYM['ITEM_LIFE'], lambda s: s.m.memory.__setitem__(A('lives'), 9))
check(s.peek(A('lives')) == 9, 'life item: capped at 9')
s = drop_and_take(SYM['ITEM_SHIELD'], ticks=2)
check(s.peek(A('p_inv')) >= 90, f'shield item: about 5 s of invulnerability (p_inv = {s.peek(A("p_inv"))})')
s = drop_and_take(SYM['ITEM_PRIZE'])
check(score(s) == 100, f'prize item: +100 points (score {score(s)})')

# bomb: every enemy dies, scores, drops nothing, the floor flashes
def bomb_setup(s):
    put_enemy(s, 0, 0, 40, 40); put_enemy(s, 1, 1, 200, 40); put_enemy(s, 2, 2, 40, 150); put_enemy(s, 3, 2, 200, 150)
s = fresh()
load_script(s, [OPC['DROP'], SYM['ITEM_BOMB'], 120, 100, OPC['END']])
s.m.memory[A('p_x')] = 120; s.m.memory[A('p_y')] = 100
tick_calm(s, 1)
bomb_setup(s)
flash_seen = []
h = safe_hook(s)
def hook(f):
    h(f); s.m.memory[A('scr_wait')] = 255
    flash_seen.append(s.vdp.pal[2])
last = s.peek(A('last_jiffy')); done = 0
while done < 6:
    s.run_frames(1, hook)
    if s.peek(A('last_jiffy')) != last: done += 1; last = s.peek(A('last_jiffy'))
check(not alive(s) and score(s) == 10 + 20 + 50 + 50, f'bomb kills every enemy and scores them (score {score(s)})')
check(all(s.peek(A('IT_BASE') + i) == 0 for i in range(6)), 'the bomb\'s kills drop no items')
check((7, 7, 7) in flash_seen and s.vdp.pal[2] == (1, 1, 3), 'the floor flashes white and goes back to its colour')

# items expire, blinking at the end
s = fresh(); load_script(s, [OPC['DROP'], SYM['ITEM_RAPID'], 40, 40, OPC['END']])
s.m.memory[A('p_x')] = 200; s.m.memory[A('p_y')] = 160
tick_calm(s, 2)
s.m.memory[A('IT_BASE') + 0x30] = 20          # 20 ticks left
hidden = []
for t in range(22):
    tick_calm(s, 1)
    hidden.append(s.vdp.vram[0x7600 + 17 * 4] == 217)
check(s.peek(A('IT_BASE')) == 0 and not s.peek(A('item_col_dirty')), 'an item nobody picks up disappears')
check(any(hidden) and not all(hidden) and not hidden[0], 'the item blinks before it goes')

# sprite attributes and colours reach the VDP
s = fresh()
load_script(s, [OPC['DROP'], SYM['ITEM_SPREAD'], 150, 60, OPC['DROP'], SYM['ITEM_BOMB'], 60, 60, OPC['END']])
s.m.memory[A('p_x')] = 200; s.m.memory[A('p_y')] = 160
tick_calm(s, 3)
v = s.vdp.vram
check(list(v[0x7600 + 17 * 4:0x7600 + 17 * 4 + 3]) == [59, 150, 8] and list(v[0x7600 + 18 * 4:0x7600 + 18 * 4 + 3]) == [59, 60, 28], 'item planes 17/18: position and pattern numbers')
check(list(v[0x7400 + 17 * 16:0x7400 + 18 * 16]) == [9] * 16 and list(v[0x7400 + 18 * 16:0x7400 + 19 * 16]) == [7] * 3 + [15] * 13, 'item colour table: cyan weapon, grey bomb with a yellow fuse')
check(all(v[0x7600 + p * 4] == 217 for p in range(19, 32)), 'unused planes stay hidden')

# drops from kills
def kill_trials(kind, n):
    s = fresh(); load_script(s, [OPC['END']])
    s.m.memory[A('p_x')] = 20; s.m.memory[A('p_y')] = 170
    drops = []
    for t in range(n):
        for i in range(6): s.m.memory[A('IT_BASE') + i] = 0
        for i in range(16): s.m.memory[EN + i] = 0; s.m.memory[BUL + i] = 0
        put_enemy(s, 0, kind, 100, 100, 1)
        put_bullet(s, 0, 100, 100, 0)
        tick_calm(s, 1)
        got = [s.peek(A('IT_BASE') + i) for i in range(6) if s.peek(A('IT_BASE') + i)]
        drops += got
    return drops
tank = kill_trials(2, 120)
check(len(tank) >= 110, f'tanks nearly always drop something ({len(tank)} drops in 120 kills)')
check(set(tank) == set(range(1, 8)), f'every item type can drop ({sorted(set(tank))})')
grunt = kill_trials(0, 300)
check(15 <= len(grunt) <= 70, f'grunts drop now and then ({len(grunt)} drops in 300 kills, about 12 % expected)')

print('weapons and items in a real run (bot, extra lives)')
s = mk(); s.run_frames(10); random.seed(8)
seen_w = set(); picked = 0; hook = safe_hook(s); hud_bad2 = 0; hudn = 0; tear2 = 0
last_w = 0
for t in range(2500):
    bot(0); quiet(s, hook)
    w = s.peek(A('p_weapon'))
    seen_w.add(w)
    if s.peek(A('score_dirty')) == 0 and t % 7 == 0: hudn += 1; hud_bad2 += hud_ok(s)
check({0, 1} <= seen_w, f'the bot picks up weapons while playing (weapons seen {sorted(seen_w)})')
check(hudn > 0 and hud_bad2 == 0, f'the HUD stays right with weapons and rooms ({hudn} checks)')

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
