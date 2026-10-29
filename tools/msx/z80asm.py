#!/usr/bin/env python3
"""Small Z80 assembler (sjasmplus-compatible subset), table-generated from z80dis.

Usage: z80asm.py SRC.asm OUT.bin [--sym OUT.sym]

Supported: labels (`name:` or column 0), `.local` labels, EQU, ORG, DB/DEFB, DW/DEFW, DS/DEFS,
INCLUDE, ALIGN, END; expressions with + - * / % << >> & | ^ ~ ( ), `$`, 0x/$/h/%/b literals and
'c' characters. The instruction table is built from the decoder, so every documented Z80
instruction (and the common undocumented IX/IY halves) is covered; `selftest` checks the
whole opcode space by round-tripping through the decoder.
"""
import re, sys, os
from z80dis import z80
from z80dis.z80 import OPER_TYPE as T

REGS = {'F','A','B','C','D','E','H','L','I','R','AF','BC','DE','HL','SP','IX','IY','IXH','IXL','IYH','IYL'}
CONDS = {'NZ':'NOT_Z','Z':'Z','NC':'NOT_C','C':'C','PO':'NOT_P','PE':'P','P':'NOT_S','M':'S'}
EMBEDDED = {'BIT', 'SET', 'RES', 'IM', 'RST'}


class AsmError(Exception):
    pass


def _kind(o, op):
    ty, v = o
    if ty == T.REG: return ('REG', 'AF2' if v.name == 'AF_' else v.name)
    if ty == T.REG_DEREF: return ('REGD', v.name)
    if ty == T.COND: return ('COND', v.name)
    if ty == T.MEM_DISPL_IX: return ('IXD', 'IX')
    if ty == T.MEM_DISPL_IY: return ('IXD', 'IY')
    if ty == T.ADDR_DEREF: return ('ADDRD', None)
    if ty == T.ADDR: return ('REL' if op in ('JR', 'DJNZ') else 'ABS', None)
    if ty == T.IMM: return ('K', v) if op in EMBEDDED else ('IMM', None)
    raise AsmError(f'unhandled operand {o}')


def _build_table():
    """template key (op, kinds) -> (fixed_bytes, layout) where layout = list of ('b',byte)|('d',)|('n',size)|('r',)"""
    table = {}
    seqs = []
    for b in range(256):
        seqs.append(([b, 0x34, 0x12, 0, 0], 0, 'plain'))
        seqs.append(([0xCB, b, 0, 0, 0], 0, 'cb'))
        seqs.append(([0xED, b, 0x34, 0x12, 0], 0, 'ed'))
        seqs.append(([0xDD, b, 5, 0x34, 0x12], 0, 'xy'))
        seqs.append(([0xFD, b, 5, 0x34, 0x12], 0, 'xy'))
        seqs.append(([0xDD, 0xCB, 5, b, 0], 0, 'xycb'))
        seqs.append(([0xFD, 0xCB, 5, b, 0], 0, 'xycb'))
    for raw, _, fam in seqs:
        if fam == 'plain' and raw[0] in (0xCB, 0xED, 0xDD, 0xFD): continue
        if fam == 'xy' and raw[1] in (0xCB, 0xDD, 0xFD, 0xED): continue
        try:
            i = z80.decode(bytes(raw), 0x100)
        except Exception:
            continue
        if i.len == 0 or i.op.name in ('NONE', 'INVALID') or 'INVALID' in i.op.name: continue
        # skip undocumented duplicates / bad decodes: re-disassemble must be sane
        txt = z80.disasm(i)
        if not txt.strip() or txt.startswith(('XNOP', 'XRETN', 'XIM', 'XNEG', 'XLD', 'XPREFIX')) or '?' in txt: continue
        if fam == 'xycb' and (txt.upper().startswith('LD ') or re.search(r'\),[A-Z]$', txt) and not txt.startswith(('BIT', 'RES', 'SET'))) or (fam == 'xycb' and re.match(r'^(RES|SET) \d,\(I[XY]\+5\),', txt)): continue
        try:
            kinds = tuple(_kind(o, i.op.name) for o in i.operands if o[0] != T.NONE)
        except AsmError:
            continue
        # JP (IX)/(IY) decodes as an IXD operand with displacement 0
        if i.op.name == 'JP' and kinds and kinds[0][0] == 'IXD': kinds = (('IXI', kinds[0][1]),)
        L = i.len
        has_disp = any(k[0] == 'IXD' for k in kinds)
        if fam == 'xycb':
            fixed = [(raw[0],), (raw[1],), ('d',), (raw[3],)]
            layout = [('b', raw[0]), ('b', raw[1]), ('d',), ('b', raw[3])]
        else:
            pre = {'plain': 1, 'cb': 2, 'ed': 2, 'xy': 2}[fam]
            layout = [('b', x) for x in raw[:pre]]
            rest = L - pre
            if has_disp: layout.append(('d',)); rest -= 1
            if rest:
                k0 = [k for k in kinds if k[0] in ('IMM', 'ADDRD', 'ABS', 'REL')]
                kind0 = k0[0][0] if k0 else 'IMM'
                layout.append(('r',) if kind0 == 'REL' else ('n', rest))
        key = (i.op.name, kinds)
        table.setdefault(key, (layout, txt))
    return table


TABLE = _build_table()
BY_OP = {}
for (op, kinds), v in TABLE.items():
    BY_OP.setdefault(op, []).append((kinds, v[0]))

# ---------------- expressions ----------------
_num = re.compile(r"^(?:0[xX][0-9a-fA-F]+|\$[0-9a-fA-F]+|[0-9][0-9a-fA-F]*[hH]|%[01]+|[01]+[bB]|[0-9]+)$")


def _tokenize(s):
    return re.findall(r"'(?:\\.|[^'])'|\"(?:\\.|[^\"])\"|0[xX][0-9a-fA-F]+|\$[0-9a-fA-F]+|[0-9][0-9a-fA-F]*[hH]\b|%[01]+|[01]+[bB]\b|[0-9]+|<<|>>|[A-Za-z_.@][A-Za-z0-9_.@']*|\$|[-+*/%&|^~()]|\S", s)


def parse_num(t):
    t = t.strip()
    if t[:2].lower() == '0x': return int(t, 16)
    if t[0] == '$' and len(t) > 1: return int(t[1:], 16)
    if t[-1] in 'hH': return int(t[:-1], 16)
    if t[0] == '%': return int(t[1:], 2)
    if t[-1] in 'bB' and set(t[:-1]) <= {'0', '1'}: return int(t[:-1], 2)
    return int(t)


class Unresolved(Exception):
    pass


def evaluate(expr, syms, pc, strict):
    expr = re.sub(r'\bhigh\s+(\w+|\([^)]*\))', r'((\1>>8)&255)', expr.strip(), flags=re.I)
    expr = re.sub(r'\blow\s+(\w+|\([^)]*\))', r'(\1&255)', expr, flags=re.I)
    toks = _tokenize(expr)
    out = []
    for t in toks:
        if t == '$': out.append(str(pc))
        elif t[0] in "'\"" and len(t) >= 3: out.append(str(ord(bytes(t[1:-1], 'utf8').decode('unicode_escape'))))
        elif _num.match(t) and not re.match(r'^[A-Za-z_]', t): out.append(str(parse_num(t)))
        elif re.match(r'^[A-Za-z_.@]', t):
            if t in syms: out.append(str(syms[t]))
            elif strict: raise AsmError(f'undefined symbol {t!r}')
            else: out.append('0')
        elif t == '/': out.append('//')
        else: out.append(t)
    try:
        return int(eval(''.join(out), {'__builtins__': {}}, {}))
    except AsmError: raise
    except Exception as e:
        raise AsmError(f'bad expression {expr!r}: {e}')


# ---------------- operand classification ----------------
def split_operands(s):
    ops, depth, cur, q = [], 0, '', None
    for ch in s:
        if q:
            cur += ch
            if ch == q: q = None
            continue
        if ch in '"\'': q = ch; cur += ch; continue
        if ch == '(': depth += 1
        if ch == ')': depth -= 1
        if ch == ',' and depth == 0: ops.append(cur.strip()); cur = ''
        else: cur += ch
    if cur.strip(): ops.append(cur.strip())
    return ops


def classify(s, permissive=False):
    """-> list of (kind tuple, expr) candidates"""
    u = s.strip()
    U = u.upper()
    c = []
    if U == "AF'": return [(('REG', 'AF2'), None)]
    if U in REGS: c.append((('REG', U), None))
    if U in CONDS: c.append((('COND', CONDS[U]), None))
    m = re.match(r'^\(\s*(HL|BC|DE|SP|C)\s*\)$', U)
    if m: return [(('REGD', m.group(1)), None)]
    m = re.match(r'^\(\s*(IX|IY)\s*([+-].*)?\)$', U)
    if m:
        reg = m.group(1); d = u[u.index(reg.lower()) + 2:].rstrip(')') if False else None
        inner = u.strip()[1:-1].strip()
        rest = inner[2:].strip()
        return [(('IXD', reg), rest or '0'), (('IXI', reg), None)]
    if u.startswith('(') and u.endswith(')') and _balanced(u[1:-1]):
        c.append((('ADDRD', None), u[1:-1]))
        if permissive:   # `(expr)` as a parenthesised immediate, e.g. ld a,(1+2)
            c += [((k, None), u) for k in ('IMM', 'ABS', 'REL', 'K')]
        return c
    if not c:
        for k in ('IMM', 'ABS', 'REL', 'K'): c.append(((k, None), u))
    return c


def _balanced(s):
    d = 0
    for ch in s:
        if ch == '(': d += 1
        if ch == ')':
            d -= 1
            if d < 0: return False
    return d == 0


def kind_match(tk, ck):
    """template kind tk vs candidate kind ck"""
    if tk[0] == 'K': return ck[0] == 'K'
    return tk == ck or (tk[0] == ck[0] and ck[1] is None)


class Assembler:
    def __init__(self):
        self.syms = {}
        self.segs = []          # (addr, bytes)
        self.listing = []

    # one pass over lines
    def run(self, path):
        lines = self._read(path)
        self.syms.clear()
        for strict in (False, True):
            self.segs = []; self.pc = 0; self.last = ''
            self._pass(lines, strict)
        return self

    def _read(self, path, seen=()):
        out = []
        base = os.path.dirname(os.path.abspath(path))
        for n, line in enumerate(open(path, encoding='utf8'), 1):
            m = re.match(r'^\s*include\s+"?([^"\s]+)"?\s*$', line, re.I)
            if m: out += self._read(os.path.join(base, m.group(1)), seen + (path,)); continue
            out.append((path, n, line.rstrip('\n')))
        return out

    def _strip(self, line):
        out, q = '', None
        for ch in line:
            if q:
                out += ch
                if ch == q: q = None
            elif ch in '"\'': q = ch; out += ch
            elif ch == ';': break
            else: out += ch
        return out.rstrip()

    def _label(self, name):
        if name.startswith('.'): name = self.last + name
        else: self.last = name
        return name

    def _lname(self, name):
        return self.last + name if name.startswith('.') else name

    def _pass(self, lines, strict):
        for path, n, raw in lines:
            line = self._strip(raw)
            if not line.strip(): continue
            try:
                self._line(line, strict)
            except AsmError as e:
                raise AsmError(f'{os.path.basename(path)}:{n}: {e}\n    {raw.strip()}')

    def _syms_view(self):
        # locals resolve relative to the current global label
        class V(dict):
            pass
        v = V(self.syms)
        for k in list(self.syms):
            if k.startswith(self.last + '.'): v[k[len(self.last):]] = self.syms[k]
        return v

    def ev(self, e, strict):
        return evaluate(e, self._syms_view(), self.pc, strict)

    def emit(self, bs):
        if self.segs and self.segs[-1][0] + len(self.segs[-1][1]) == self.pc:
            self.segs[-1] = (self.segs[-1][0], self.segs[-1][1] + bytes(bs))
        else: self.segs.append((self.pc, bytes(bs)))
        self.pc += len(bs)

    def _line(self, line, strict):
        m = re.match(r'^([A-Za-z_.@][A-Za-z0-9_.@]*)\s*:?\s*(.*)$', line) if not line[0].isspace() else None
        rest = line.strip()
        if m:
            name, after = m.group(1), m.group(2)
            low = name.lower()
            is_colon = bool(re.match(r'^[A-Za-z_.@][A-Za-z0-9_.@]*\s*:', line))
            if is_colon or low not in DIRECTIVES and low.upper() not in BY_OP:
                # label (with EQU handled below)
                em = re.match(r'^(equ|=)\s+(.*)$', after, re.I)
                if em:
                    self.syms[self._label(name)] = self.ev(em.group(2), strict); return
                self.syms[self._label(name)] = self.pc
                rest = after.strip()
                if not rest: return
        elif rest:
            pass
        em = re.match(r'^(\S+)\s+equ\s+(.*)$', rest, re.I)
        if em:
            self.syms[self._label(em.group(1).rstrip(':'))] = self.ev(em.group(2), strict); return
        parts = rest.split(None, 1)
        mn = parts[0].lower(); arg = parts[1] if len(parts) > 1 else ''
        if mn in ('org',): self.pc = self.ev(arg, True); return
        if mn in ('end',): return
        if mn in ('align',):
            a = self.ev(arg, True)
            while self.pc % a: self.emit([0xFF])
            return
        if mn in ('db', 'defb', 'byte', 'dm', 'defm'):
            for o in split_operands(arg):
                if o[0] in '"\'' and o[-1] == o[0] and len(o) != 3 or (o[0] == '"'):
                    self.emit(bytes(o[1:-1], 'latin1').decode('unicode_escape').encode('latin1'))
                else:
                    self.emit([self.ev(o, strict) & 0xFF])
            return
        if mn in ('dw', 'defw', 'word'):
            for o in split_operands(arg):
                v = self.ev(o, strict) & 0xFFFF; self.emit([v & 255, v >> 8])
            return
        if mn in ('ds', 'defs', 'block'):
            ops = split_operands(arg)
            cnt = self.ev(ops[0], True); fill = self.ev(ops[1], strict) & 0xFF if len(ops) > 1 else 0
            self.emit([fill] * cnt); return
        self._insn(mn.upper(), split_operands(arg), strict)

    def _insn(self, op, ops, strict):
        if op not in BY_OP: raise AsmError(f'unknown instruction {op}')
        for permissive in (False, True):
            if self._try(op, ops, strict, permissive): return
        raise AsmError(f'cannot assemble {op} {", ".join(ops)}')

    def _try(self, op, ops, strict, permissive):
        cands = [classify(o, permissive) for o in ops]
        for kinds, layout in BY_OP[op]:
            if len(kinds) != len(ops): continue
            picks = []
            ok = True
            for tk, cl in zip(kinds, cands):
                hit = None
                for ck, expr in cl:
                    if kind_match(tk, ck):
                        hit = (ck, expr); break
                if not hit: ok = False; break
                picks.append(hit)
            if not ok: continue
            # embedded constants must match their value
            vals = {}
            for tk, (ck, expr) in zip(kinds, picks):
                if tk[0] == 'K':
                    if self.ev(expr, strict) != tk[1]:
                        ok = False; break
            if not ok: continue
            self._encode(op, kinds, picks, layout, strict)
            return True
        return False

    def _encode(self, op, kinds, picks, layout, strict):
        disp = 0; imm = None; rel = None
        for tk, (ck, expr) in zip(kinds, picks):
            if tk[0] == 'IXD': disp = self.ev(expr, strict)
            elif tk[0] in ('IMM', 'ADDRD', 'ABS'): imm = self.ev(expr, strict)
            elif tk[0] == 'REL': rel = self.ev(expr, strict)
        size = sum(1 for l in layout if l[0] == 'b') + sum(1 for l in layout if l[0] in ('d', 'r')) + sum(l[1] for l in layout if l[0] == 'n')
        out = []
        for l in layout:
            if l[0] == 'b': out.append(l[1])
            elif l[0] == 'd':
                if strict and not -128 <= disp <= 127: raise AsmError(f'displacement {disp} out of range')
                out.append(disp & 0xFF)
            elif l[0] == 'r':
                off = rel - (self.pc + size)
                if strict and not -128 <= off <= 127: raise AsmError(f'relative jump out of range ({off})')
                out.append(off & 0xFF)
            elif l[0] == 'n':
                v = imm if imm is not None else 0
                if strict and l[1] == 1 and not -128 <= v <= 255: raise AsmError(f'8-bit value {v} out of range')
                out += [v & 0xFF] if l[1] == 1 else [v & 0xFF, (v >> 8) & 0xFF]
        self.emit(out)

    def image(self, fill=0xFF):
        if not self.segs: return 0, b''
        lo = min(a for a, _ in self.segs); hi = max(a + len(b) for a, b in self.segs)
        img = bytearray([fill]) * (hi - lo)
        for a, b in self.segs: img[a - lo:a - lo + len(b)] = b
        return lo, bytes(img)


DIRECTIVES = {'org', 'end', 'align', 'db', 'defb', 'byte', 'dm', 'defm', 'dw', 'defw', 'word', 'ds', 'defs', 'block', 'equ', 'include'}


def selftest():
    """Round trip every table entry through the decoder."""
    import itertools
    bad = 0; n = 0
    asm = Assembler()
    for (op, kinds), (layout, txt) in sorted(TABLE.items(), key=lambda kv: kv[1][1]):
        n += 1
        # re-assemble the decoder's own text (replace dummy operands by themselves)
        t = txt
        asm.segs = []; asm.pc = 0x100; asm.syms = {}; asm.last = ''
        try:
            line = re.sub(r"\bAF_\b", "AF'", t)
            asm._line('        ' + line, True)
        except AsmError as e:
            bad += 1; print('FAIL', txt, e); continue
        got = b''.join(b for _, b in asm.segs)
        i = z80.decode(got + b'\0\0\0', 0x100)
        if i.len != len(got) or z80.disasm(i) != txt:
            bad += 1; print('MISMATCH', txt, got.hex(), z80.disasm(i))
    print(f'{n} instruction forms, {bad} problems')
    return bad == 0


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == 'selftest':
        sys.exit(0 if selftest() else 1)
    src, out = sys.argv[1], sys.argv[2]
    a = Assembler().run(src)
    lo, img = a.image()
    open(out, 'wb').write(img)
    if '--sym' in sys.argv:
        with open(sys.argv[sys.argv.index('--sym') + 1], 'w') as f:
            for k, v in sorted(a.syms.items(), key=lambda kv: kv[1]): f.write(f'{v:04X} {k}\n')
    print(f'{src}: {len(img)} bytes at ${lo:04X}-${lo+len(img)-1:04X}, {len(a.syms)} symbols')


if __name__ == '__main__':
    try: main()
    except AsmError as e: print('error:', e); sys.exit(1)
