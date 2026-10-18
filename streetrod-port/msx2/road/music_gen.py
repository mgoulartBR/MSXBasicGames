#!/usr/bin/env python3
"""Gera as 5 estacoes de radio (composicoes originais, estilos dos anos 50/60) em linhas de sequenciador para o PSG.

Formato de cada musica: N linhas x 4 bytes (canais A=melodia, B=baixo, C=harmonia, e um byte de bateria).
Notas: 0 = continua a nota anterior, 1 = silencio, >= 12 = nota MIDI. Bateria: bits 16 bumbo, 8 caixa, 4 tom, 2 prato, 1 chimbal (como o registrador 0x0E do OPLL).
No FM (MSX-MUSIC) tudo toca; no PSG a bateria vira ruido no canal C (chimbal/prato = curto, caixa = longo).
Cada linha dura `row_ms` milissegundos; as musicas voltam ao comeco quando acabam (loop sem emenda: todas fecham o ciclo harmonico)."""
import random

K_BD, K_HH, K_SD, K_TOM, K_CYM = 1, 2, 3, 4, 8                   # bumbo, chimbal, caixa, tom, prato
DRUM_BIT = {K_BD: 16, K_HH: 1, K_SD: 8, K_TOM: 4, K_CYM: 2}   # bits do registrador de ritmo do OPLL (0x0E)
NAMES = ["ROCK 'N' ROLL", "SLOW BLUES", "SWING JAZZ", "DOO-WOP", "SURF ROCK"]


class Song:
    def __init__(s, bars, rpb, bpm):
        s.rpb, s.bpm = rpb, bpm                    # linhas por tempo e batidas por minuto (compasso 4/4)
        s.rpbar = rpb * 4
        s.n = bars * s.rpbar
        s.ch = [[1] * s.n for _ in range(3)]
        s.dr = [0] * s.n
        s.row_ms = round(60000 / (bpm * rpb))

    def put(s, c, row, pitch, ln, gate=0):
        """nota `pitch` em `row` por `ln` linhas; gate = linhas de silencio no fim (staccato)."""
        if row < 0 or row >= s.n: return
        s.ch[c][row] = pitch
        for k in range(1, ln):
            if row + k < s.n: s.ch[c][row + k] = 0
        for k in range(gate):
            r = row + ln - 1 - k
            if r > row and r < s.n: s.ch[c][r] = 1

    def tremolo(s, c, row, pitch, ln):
        for k in range(ln):
            if row + k < s.n: s.ch[c][row + k] = pitch

    def drum(s, row, kind):
        if 0 <= row < s.n: s.dr[row] |= DRUM_BIT[kind]

    def data(s):
        out = bytearray()
        for r in range(s.n): out += bytes([s.ch[0][r], s.ch[1][r], s.ch[2][r], s.dr[r]])
        return bytes(out)

    def seconds(s): return s.n * s.row_ms / 1000


# ---------------------------------------------------------------------------------------------- 1. ROCK 'N' ROLL (blues de 12 compassos em La, boogie)
def rock():
    bars = 12 * 8
    S = Song(bars, 4, 160)
    roots = [0, 0, 0, 0, 5, 5, 0, 0, 7, 5, 0, 7]                 # A A A A D D A A E D A E
    A0 = 69                                                       # La4
    lick = [
        [(0, 2, 0), (2, 2, 4), (4, 2, 7), (6, 2, 9), (8, 2, 10), (10, 2, 9), (12, 2, 7), (14, 2, 4)],
        [(0, 3, 12), (3, 1, 10), (4, 2, 9), (6, 2, 7), (8, 4, 4), (12, 2, 7), (14, 2, 9)],
        [(0, 2, 7), (3, 2, 10), (6, 2, 7), (8, 2, 5), (10, 2, 4), (12, 4, 0)],
        [(0, 4, 12), (4, 2, 12), (6, 2, 10), (8, 4, 9), (12, 2, 7), (14, 2, 4)],
        [(0, 2, 9), (2, 2, 9), (4, 2, 12), (6, 2, 9), (8, 2, 7), (10, 2, 4), (12, 4, 7)],
        [(0, 2, 12), (2, 2, 9), (4, 2, 7), (6, 2, 9), (8, 2, 12), (10, 2, 9), (12, 4, 7)],
    ]
    walk = [[0, 4, 7, 9, 10, 9, 7, 4], [0, 0, 7, 7, 9, 9, 10, 7]]  # baixo em colcheias (boogie)
    rnd = random.Random(1958)
    for cho in range(8):
        for b in range(12):
            bar = cho * 12 + b; r0 = bar * 16; rt = roots[b]
            bass = walk[1 if (b % 2 and cho % 2) else 0]
            for i, off in enumerate(bass): S.put(1, r0 + 2 * i, 33 + rt + off, 2, gate=1)
            # lead: quatro tipos de compasso por coro
            if cho == 0 and b < 4: pass                                              # introducao: so baixo e bateria
            else:
                idx = (b + cho) % len(lick) if cho else [0, 0, 0, 0, 1, 1, 2, 2, 3, 1, 2, 4][b]
                if cho % 4 == 3: idx = [5, 5, 4, 4, 3, 1, 2, 5, 3, 1, 2, 4][b]
                for (r, ln, off) in lick[idx]: S.put(0, r0 + r, A0 + rt + off, ln, gate=1 if ln > 2 else 0)
            # bateria / quebra
            brk = (b == 11 and cho % 2 == 1)
            for q in range(8):
                row = r0 + 2 * q
                if q in (2, 6): S.drum(row, K_SD)
                elif not (cho == 0 and b < 4 and q % 2): S.drum(row, K_HH)
                if q in (0, 4): S.drum(row, K_BD)
                if q == 0 and b == 0 and cho > 0: S.drum(row, K_CYM)
            if brk:                                                                  # quebra de coro: acordes no canal C
                for k, off in enumerate([0, 4, 7, 12, 7, 4, 0, 4]): S.put(2, r0 + 2 * k, 57 + rt + off, 2, gate=1)
    return S


# ---------------------------------------------------------------------------------------------- 2. SLOW BLUES (12 compassos em Do, shuffle)
def blues():
    S = Song(12 * 4, 3, 80)
    roots = [0, 0, 0, 0, 5, 5, 0, 0, 7, 5, 0, 7]
    C4 = 60
    licks = [                                                     # (linha, duracao, semitons sobre a fundamental), 2 compassos = 24 linhas
        [(0, 2, 7), (3, 2, 10), (6, 3, 7), (9, 2, 5), (12, 3, 3), (15, 3, 0), (18, 6, 0)],
        [(0, 3, 12), (3, 3, 10), (6, 2, 7), (8, 1, 6), (9, 3, 7), (12, 12, 0)],
        [(2, 1, 3), (3, 3, 4), (6, 3, 7), (9, 2, 10), (11, 1, 9), (12, 3, 7), (15, 9, 4)],
        [(0, 6, 10), (6, 3, 9), (9, 3, 7), (12, 3, 10), (15, 3, 7), (18, 6, 3)],
        [(0, 2, 12), (2, 1, 10), (3, 3, 7), (6, 3, 10), (9, 3, 7), (12, 3, 5), (15, 3, 3), (18, 6, 0)],
        [(0, 12, 7), (12, 3, 10), (15, 3, 9), (18, 3, 7), (21, 3, 4)],
    ]
    third = {0: 4, 5: 4, 7: 4}
    bass_pat = {0: [0, 4, 7, 9], 5: [0, 4, 7, 9], 7: [0, 4, 7, 9]}
    alt = [0, 4, 7, 10]
    for cho in range(4):
        for b in range(12):
            bar = cho * 12 + b; r0 = bar * 12; rt = roots[b]
            pat = bass_pat[rt] if b % 2 == 0 else alt
            if b == 11: pat = [7, 6, 5, 4]                       # virada final: G F# F E (sobe de volta ao Do)
            for i, off in enumerate(pat):
                p = 36 + rt + off
                if b == 11: p = [43, 42, 41, 40][i]
                S.put(1, r0 + 3 * i, p, 3, gate=1)
            # harmonia no canal C: terca e quinta em compasso de shuffle, caixa no 2 e 4
            for beat in range(4):
                row = r0 + 3 * beat
                S.drum(row, K_HH); S.drum(row + 2, K_HH)                                   # prato em shuffle
                if beat in (1, 3): S.drum(row, K_SD)
                else: S.drum(row, K_BD); S.put(2, row, C4 + rt + (4 if beat == 0 else 7), 2, gate=0)
                if beat in (0, 2): S.put(2, row + 2, C4 + rt + (7 if beat % 2 == 0 else 10), 1)
        # melodia (3 frases por coro: AAB)
        base = cho * 12 * 12
        sel = [(0, 0, 4), (1, 1, 5), (2, 2, 3), (0, 3, 4)][cho]
        phr = [(0, 0, sel[0]), (2, 0, sel[0]), (4, 5, sel[1]), (6, 5, sel[1]), (8, 7, sel[2]), (10, 0, sel[2])]
        for (bar_i, tr, li) in phr:
            r0 = base + bar_i * 12
            for (r, ln, off) in licks[li]:
                if cho == 0 and li == sel[0] and bar_i == 2: off = off                     # repeticao
                S.put(0, r0 + r, C4 + tr + off, ln, gate=1 if ln > 3 else 0)
        # fim de coro: nota longa na tonica
        S.put(0, base + 11 * 12, C4 + 7 + 7, 6, gate=1)
    return S


# ---------------------------------------------------------------------------------------------- 3. SWING JAZZ (mudancas "rhythm", 32 compassos AABA em Do)
CH = {'C': (0, [0, 4, 7, 11]), 'Am7': (9, [0, 3, 7, 10]), 'A7': (9, [0, 4, 7, 10]), 'Dm7': (2, [0, 3, 7, 10]), 'G7': (7, [0, 4, 7, 10]),
      'Em7': (4, [0, 3, 7, 10]), 'Gm7': (7, [0, 3, 7, 10]), 'C7': (0, [0, 4, 7, 10]), 'F': (5, [0, 4, 7, 11]), 'F#o': (6, [0, 3, 6, 9]),
      'E7': (4, [0, 4, 7, 10]), 'D7': (2, [0, 4, 7, 10]), 'Cmaj7': (0, [0, 4, 7, 11])}
SCALE = {  # semitons da escala sobre a fundamental
    'maj': [0, 2, 4, 5, 7, 9, 11], 'dor': [0, 2, 3, 5, 7, 9, 10], 'mix': [0, 2, 4, 5, 7, 9, 10], 'dim': [0, 2, 3, 5, 6, 8, 9, 11]}
KIND = {'C': 'maj', 'F': 'maj', 'Cmaj7': 'maj', 'Am7': 'dor', 'Dm7': 'dor', 'Em7': 'dor', 'Gm7': 'dor', 'A7': 'mix', 'G7': 'mix', 'C7': 'mix', 'E7': 'mix',
        'D7': 'mix', 'F#o': 'dim'}
A_PART = ['C', 'A7', 'Dm7', 'G7', 'Em7', 'A7', 'Dm7', 'G7', 'Gm7', 'C7', 'F', 'F#o', 'C', 'G7', 'C', 'G7']   # 8 compassos em 2 acordes por compasso (pares)
A_BARS = [['C', 'A7'], ['Dm7', 'G7'], ['Em7', 'A7'], ['Dm7', 'G7'], ['Gm7', 'C7'], ['F', 'F#o'], ['C', 'G7'], ['C', 'G7']]
B_BARS = [['E7', 'E7'], ['E7', 'E7'], ['A7', 'A7'], ['A7', 'A7'], ['D7', 'D7'], ['D7', 'D7'], ['G7', 'G7'], ['G7', 'G7']]


def jazz():
    S = Song(96, 3, 140)
    rnd = random.Random(1961)
    eighths = [0, 2, 3, 5, 6, 8, 9, 11]                            # colcheias em swing sobre a grade de tercinas (0,2 | 3,5 | ...)

    def scale_notes(chord, lo, hi):
        root, tones = CH[chord]; sc = SCALE[KIND[chord]]
        return [p for p in range(lo, hi + 1) if (p - root) % 12 in sc], [p for p in range(lo, hi + 1) if (p - root) % 12 in tones]

    def melody_bar(chord, nxt, prev, density, rnd):
        """uma frase de colcheias: caminha pela escala, termina perto do proximo acorde. devolve [(linha, duracao, nota)]"""
        sc, ct = scale_notes(chord, 64, 86)
        cur = prev if prev else rnd.choice(ct)
        out = []; direction = rnd.choice([-1, 1]); pos_list = eighths
        for k, row in enumerate(pos_list):
            if rnd.random() > density and k not in (0, 7): continue
            if row == 0 and k == 0 and rnd.random() < 0.6: cur = min(ct, key=lambda p: abs(p - cur))
            else:
                i = min(range(len(sc)), key=lambda j: abs(sc[j] - cur))
                if rnd.random() < 0.25: direction = -direction
                step = direction * (1 if rnd.random() < 0.75 else 2)
                i = max(0, min(len(sc) - 1, i + step)); cur = sc[i]
                if cur > 84 or cur < 66: direction = -direction
            out.append([row, 1, cur])
        # duracoes: ate a proxima nota; ultima nota segura ate o fim do compasso quando acaba cedo
        for i, ev in enumerate(out):
            end = out[i + 1][0] if i + 1 < len(out) else 12
            ev[1] = max(1, end - ev[0])
        # terminacao em nota do acorde seguinte
        _, ct2 = scale_notes(nxt, 64, 86)
        out[-1][2] = min(ct2, key=lambda p: abs(p - out[-1][2]))
        return [tuple(e) for e in out]

    def head(bars_def, seed, density=0.75):
        r = random.Random(seed); notes = []; prev = 72
        for bar, ch2 in enumerate(bars_def):
            nxt = bars_def[(bar + 1) % len(bars_def)][0]
            half = [melody_bar(ch2[0], ch2[1], prev, density, r)]
            m = half[0]; prev = m[-1][2]; notes.append(m)
        return notes

    cho_melody = []
    heads = {}
    for name, seed in (('A', 11), ('B', 29), ('A2', 41), ('X1', 3), ('X2', 8), ('XB', 61)):
        heads[name] = head(A_BARS if name != 'B' and name != 'XB' else B_BARS, seed, 0.78 if name[0] != 'X' else 0.9)
    form = [['A', 'A', 'B', 'A2'], ['X1', 'X1', 'XB', 'X2'], ['A', 'A', 'B', 'A2']]
    for cho in range(3):
        for sec in range(4):
            part = form[cho][sec]; bars_def = B_BARS if part in ('B', 'XB') else A_BARS
            for b in range(8):
                bar = cho * 32 + sec * 8 + b; r0 = bar * 12
                chs = bars_def[b]
                # baixo caminhando (4 tempos): 1 terca 5 e aproximacao cromatica do proximo
                for beat in range(4):
                    chord = chs[0] if beat < 2 else chs[1]; root, tones = CH[chord]
                    nxt_chord = bars_def[(b + 1) % 8][0] if beat == 3 else chord
                    if beat == 0: p = 36 + root
                    elif beat == 3: p = 36 + CH[nxt_chord][0] + (1 if (cho + b) % 2 else -1)
                    else: p = 36 + root + tones[beat] % 12
                    while p > 50: p -= 12
                    while p < 36: p += 12
                    S.put(1, r0 + 3 * beat, p, 3, gate=1)
                # canal C: "pe de chimbal" nos tempos 2 e 4 e acordes curtos (terca / setima) no contratempo
                for beat in range(4):
                    row = r0 + 3 * beat; chord = chs[0] if beat < 2 else chs[1]; root, tones = CH[chord]
                    S.drum(row, K_HH)                                                      # prato de conducao: toda semínima + o "a" do swing
                    if beat in (1, 3): S.drum(row + 2, K_HH)
                    if bar % 32 == 0 and beat == 0: S.drum(row, K_CYM)
                    if beat == 3 and bar % 4 == 3: S.drum(row + 2, K_SD)
                    if beat in (1, 3): pass
                    else: S.put(2, row, 60 + (root + tones[1]) % 12, 1)
                    if beat in (0, 2): S.put(2, row + 2, 60 + (root + tones[3]) % 12, 1)
                # melodia
                for (r, ln, p) in heads[part][b]: S.put(0, r0 + r, p, ln, gate=1 if ln > 2 else 0)
    return S


# ---------------------------------------------------------------------------------------------- 4. DOO-WOP (balada I-vi-IV-V em Do, 12/8)
def doowop():
    S = Song(36, 3, 60)
    prog = {'C': (0, 'maj'), 'Am': (9, 'min'), 'F': (5, 'maj'), 'G': (7, 'maj'), 'Dm': (2, 'min'), 'Em': (4, 'min')}
    A_ch = ['C', 'Am', 'F', 'G', 'C', 'Am', 'F', 'G']
    B_ch = ['F', 'G', 'C', 'Am', 'F', 'G', 'C', 'G']
    intro = ['C', 'Am', 'F', 'G']
    tag = ['C', 'Am', 'F', 'G']
    # melodia da secao A (4 compassos + 4 compassos), notas (linha, duracao, midi)
    A_mel = [
        [(0, 6, 76), (6, 3, 74), (9, 3, 72)],
        [(0, 3, 72), (3, 3, 76), (6, 6, 79)],
        [(0, 6, 77), (6, 3, 76), (9, 3, 74)],
        [(0, 9, 74), (9, 3, 71)],
        [(0, 6, 79), (6, 3, 76), (9, 3, 72)],
        [(0, 6, 76), (6, 6, 72)],
        [(0, 3, 77), (3, 3, 76), (6, 3, 74), (9, 3, 72)],
        [(0, 9, 74), (9, 3, 71)],
    ]
    A_mel_end = A_mel[:7] + [[(0, 12, 72)]]
    B_mel = [
        [(0, 6, 81), (6, 6, 77)],
        [(0, 6, 79), (6, 3, 76), (9, 3, 79)],
        [(0, 9, 76), (9, 3, 72)],
        [(0, 6, 72), (6, 6, 69)],
        [(0, 6, 77), (6, 3, 74), (9, 3, 77)],
        [(0, 6, 79), (6, 3, 76), (9, 3, 74)],
        [(0, 6, 76), (6, 6, 72)],
        [(0, 12, 71)],
    ]
    plan = [('intro', intro, None), ('A', A_ch, A_mel), ('A', A_ch, A_mel), ('B', B_ch, B_mel), ('A', A_ch, A_mel_end), ('tag', tag, None)]
    bar = 0
    for name, chs, mel in plan:
        for i, ch in enumerate(chs):
            if bar >= 36: break
            r0 = bar * 12; root, kind = prog[ch]
            third = 3 if kind == 'min' else 4
            # arpejo em tercinas no canal C (piano): fundamental, quinta, oitava, quinta ...
            arp = [0, 7, 12, 7, third + 12, 7, 12, 7, 0, 7, 12, 7]
            for k in range(12): S.put(2, r0 + k, 48 + root - (12 if root > 6 else 0) + arp[k], 1)
            if name == 'A' or name == 'B':                                        # bateria suave: so a caixa de leve no 2 e no 4 (canal C ja tem o arpejo)
                pass
            if name != 'intro' and name != 'tag': S.drum(r0, K_BD); S.drum(r0 + 3, K_SD); S.drum(r0 + 9, K_SD)
            # baixo: fundamental no 1, quinta no 3 (voz grave "bom-bom-bom")
            S.put(1, r0, 36 + root, 6, gate=1); S.put(1, r0 + 6, 43 + root if root < 5 else 31 + root, 3, gate=0); S.put(1, r0 + 9, 36 + root, 3, gate=1)
            if mel:
                for (r, ln, p) in mel[i]: S.put(0, r0 + r, p, ln, gate=1 if ln > 6 else 0)
            elif name == 'tag':
                S.put(0, r0, [72, 72, 69, 71][i] + 4 * (i == 0), 12, gate=1)
            bar += 1
    return S


# ---------------------------------------------------------------------------------------------- 5. SURF ROCK (Mi menor, tremolo)
def surf():
    bars = 96
    S = Song(bars, 4, 144)
    E = 64
    ph = [0, 1, 4, 5, 7, 8, 10]                                      # escala "espanhola" (frigio dominante) sobre a fundamental Mi
    def pitch(deg, octv=0): return E + 12 * (deg // 7 + octv) + ph[deg % 7]
    A_chords = ['Em', 'Em', 'C', 'D', 'Em', 'Em', 'C', 'B']
    B_chords = ['Am', 'Am', 'Em', 'Em', 'C', 'D', 'Em', 'B']
    roots = {'Em': 0, 'C': -4, 'D': -2, 'B': -5, 'Am': 5}
    A_mel = [
        [(0, [0, 1, 0, 1, 2, 1, 0, 1, 0, 1, 2, 3, 4, 3, 2, 1])],
        [(0, [0, 1, 2, 1, 0, 1, 2, 3, 2, 1, 0, 1, 2, 1, 0, 1])],
        [(0, [4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 4, 4, 3, 3])],
        [(0, [2, 2, 2, 2, 3, 3, 3, 3, 4, 3, 2, 3, 4, 5, 6, 7])],
        [(0, [7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 4, 3, 2])],
        [(0, [0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 0, 0])],
        [(0, [4, 5, 6, 5, 4, 3, 2, 3, 4, 3, 2, 1, 2, 1, 0, 1])],
        [(0, [1, 0, 1, 0, 1, 0, 1, 0, 6, 6, 6, 6, 5, 5, 5, 5])],
    ]
    B_mel = [
        [(0, [3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 4, 4, 3, 3])],
        [(0, [2, 3, 4, 3, 2, 3, 4, 5, 4, 3, 2, 3, 4, 3, 2, 1])],
        [(0, [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 0, 0])],
        [(0, [0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1, 0, 1])],
        [(0, [2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 3, 3, 2, 2])],
        [(0, [3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 4, 4, 3, 3])],
        [(0, [4, 3, 2, 1, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0, 0])],
        [(0, [1, 1, 1, 1, 0, 0, 0, 0, 6, 6, 6, 6, 5, 5, 5, 5])],
    ]
    for bar in range(bars):
        sec = bar // 8; part = sec % 2; b = bar % 8; r0 = bar * 16
        ch = (A_chords if part == 0 else B_chords)[b]; rt = roots[ch]
        mel = (A_mel if part == 0 else B_mel)[b][0][1]
        octv = 1 if sec >= 6 and sec % 2 == 0 else 0
        for k, deg in enumerate(mel):
            S.ch[0][r0 + k] = pitch(deg, octv)                                  # notas repetidas = tremolo (cada linha reataca)
        # baixo em colcheias saltando oitava (galope)
        for q in range(8):
            p = 40 + rt + (12 if q % 4 == 3 else 0)
            S.put(1, r0 + 2 * q, p, 2, gate=1)
        for q in range(8):
            row = r0 + 2 * q
            if q in (2, 6): S.drum(row, K_SD)
            else: S.drum(row, K_HH)
            if q in (0, 3): S.drum(row, K_BD)
            if q == 0 and b == 0 and sec % 2 == 0: S.drum(row, K_CYM)
        if bar % 8 == 7 and sec % 2 == 1:                                      # virada de bateria no fim da secao B
            for k in range(8): S.drum(r0 + 8 + k, K_TOM if k % 2 == 0 else K_SD)
    return S


def all_songs():
    return [rock(), blues(), jazz(), doowop(), surf()]


if __name__ == '__main__':
    for nm, s in zip(NAMES, all_songs()):
        d = s.data()
        notes = [x for x in d if x >= 12]
        print('%-14s %4d linhas %5d B  %5.1f s  notas %d..%d' % (nm, s.n, len(d), s.seconds(), min(notes), max(notes)))
