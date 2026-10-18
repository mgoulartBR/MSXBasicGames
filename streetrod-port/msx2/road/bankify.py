#!/usr/bin/env python3
"""bankify.py main.asm main.c out.asm
Distribui as funcoes/tabelas de main.asm em areas BANKn conforme os `#pragma codeseg` do fonte (por linha do .c) e confere que
nenhuma referencia cruza bancos de codigo sem passar por uma funcao __banked (o nucleo = area _CODE, na pagina 1, e visivel de todos)."""
import re, sys
asm_p, c_p, out_p = sys.argv[1:4]
regs = []
for i, l in enumerate(open(c_p, errors='replace').read().split('\n'), 1):
    m = re.match(r'#pragma codeseg (\w+)', l)
    if m: regs.append((i, m.group(1)))
def region(n):
    r = 'CODE'
    for ln, name in regs:
        if ln <= n: r = name
    return r
clines = open(c_p, errors='replace').read().split('\n')
def decl_line(name):
    pat = re.compile(r'^(static |const |extern )*[\w ]+[ *]\*?%s\s*[\[(=;]' % re.escape(name[1:]))
    for i, l in enumerate(clines, 1):
        if pat.match(l) and not l.startswith(('return', 'else')) and not (re.search(r'\)\s*(__banked|BK)?\s*;\s*$', l) and '(' in l): return i
    return None
lines = open(asm_p).read().split('\n')
banked = set(re.findall(r'^\s*b_(\w+)\s*=', '\n'.join(lines), re.M))
out = []; area = None; cur = 'CODE'; srcline = 1; incode = False
sym_bank = {}; label_of_line = {}; fn = None; refs = []
lab = re.compile(r'^(_\w+|___str_\d+):{1,2}\s*$')
for l in lines:
    m = re.match(r';main\.c:(\d+):', l)
    if m: srcline = int(m.group(1))
    a = re.match(r'\t\.area (\w+)', l)
    if a:
        incode = (a.group(1) == '_CODE'); area = a.group(1)
        if incode: continue          # o proximo rotulo decide a area
        out.append(l); continue
    if incode:
        m = lab.match(l)
        if m:
            name = m.group(1).rstrip(':')
            cur = region(decl_line(name) or srcline) if not name.startswith('___str') else region(srcline); sym_bank[name] = cur; fn = name
            out.append('\t.area %s' % ('_CODE' if cur == 'CODE' else cur))
        elif fn and not l.startswith(('\t.globl', ';')):
            for t in re.findall(r'(?<![\w$])(_\w+|___str_\d+)\b', l):
                if t != fn: refs.append((fn, t, srcline))
    out.append(l)
open(out_p, 'w').write('\n'.join(out))
bad = 0
for fn, t, ln in refs:
    if t in sym_bank and sym_bank[t] != 'CODE' and sym_bank[t] != sym_bank[fn] and t[1:] not in banked:
        print('ERRO: %s (%s) usa %s (%s) perto de main.c:%d' % (fn, sym_bank[fn], t, sym_bank[t], ln)); bad += 1
    if t in sym_bank and sym_bank[fn] != 'CODE' and sym_bank[t] == 'CODE': pass
print('bankify: %d referencias entre bancos sem __banked' % bad)
sys.exit(1 if bad else 0)
