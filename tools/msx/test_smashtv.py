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
    digits += [rb >> 4, rb & 15, s.peek(A('lives')) & 15]
    xs = [8 + 8 * i for i in range(6)] + [120, 128, 232]
    bad = 0
    for d, x0 in zip(digits, xs):
        for j in range(8):
            for k in range(8):
                g = sheet_px(s, d * 8 + k, j)
                want = g if g else 14
                if v.px(x0 + k, 256 * front(s) + 4 + j) != want: bad += 1
    return bad


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
check(hud_ok(s) == 0, 'HUD shows 000000, room 01 and 3 lives')
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

print('double buffering: the displayed page never changes while it is shown')
s = mk(); s.run_frames(10)
samples = []
def sample(frame):
    page = (s.vdp.r[2] >> 5) & 3
    top = page * 0x8000
    samples.append((frame, page, hash(bytes(s.vdp.vram[top:top + 212 * 128])), hash(bytes(s.vdp.vram[0x7600:0x7680]))))
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
        s.m.memory[A('p_inv')] = 251
        s.m.memory[A('lives')] = 3
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
    s.run_frames(3, hook)
    for _, k, _, _ in alive(s): spawned_kinds.add(k)
    r = s.peek(A('room_bcd'))
    if r != rooms_seen[-1]:
        rooms_seen.append(r)
        run_ticks(s, 2)
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
