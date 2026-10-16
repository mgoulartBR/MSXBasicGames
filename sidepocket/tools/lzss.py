"""LZSS (Okumura-style) used for the screen images.

Stream: groups of 8 items led by a flag byte (LSB first); flag bit 1 = literal byte; 0 = 2-byte reference
    b1 = low 8 bits of the ring position, b2 = (high 4 bits of the position << 4) | (length - 3)   (length 3..18)
Ring buffer N = 4096, initial write position N - 18, initially zero.  The decoder (msx/blob.c) mirrors this.
"""
N, F, THRESH = 4096, 18, 3

def compress(data: bytes) -> bytes:
    out = bytearray(); items = []          # items: ('L', byte) | ('M', pos, len)
    head = {}                              # 3-byte hash -> list of candidate output indices
    i = 0; n = len(data)
    while i < n:
        best_len, best_j = 0, 0
        if i + THRESH <= n:
            key = data[i:i + 3]
            for j in reversed(head.get(key, [])[-64:]):
                if i - j > N - F: break
                l = 0
                while l < F and i + l < n and data[j + l] == data[i + l]: l += 1
                if l > best_len: best_len, best_j = l, j
                if l == F: break
        step = best_len if best_len >= THRESH else 1
        if best_len >= THRESH:
            r = (N - F + best_j) % N
            items.append(('M', r, best_len))
        else:
            items.append(('L', data[i]))
        for k in range(i, min(i + step, n - 2)):
            head.setdefault(data[k:k + 3], []).append(k)
        i += step
    for g in range(0, len(items), 8):
        grp = items[g:g + 8]; flag = 0; body = bytearray()
        for b, it in enumerate(grp):
            if it[0] == 'L': flag |= 1 << b; body.append(it[1])
            else: body += bytes((it[1] & 0xFF, ((it[1] >> 4) & 0xF0) | (it[2] - THRESH)))
        out.append(flag); out += body
    return bytes(out)

def decompress(buf: bytes, size: int) -> bytes:
    ring = bytearray(N); r = N - F; out = bytearray(); p = 0
    while len(out) < size:
        flag = buf[p]; p += 1
        for b in range(8):
            if len(out) >= size: break
            if flag & 1:
                c = buf[p]; p += 1; out.append(c); ring[r] = c; r = (r + 1) % N
            else:
                b1, b2 = buf[p], buf[p + 1]; p += 2
                pos = b1 | ((b2 & 0xF0) << 4); ln = (b2 & 0x0F) + THRESH
                for k in range(ln):
                    c = ring[(pos + k) % N]; out.append(c); ring[r] = c; r = (r + 1) % N
            flag >>= 1
    return bytes(out[:size])

if __name__ == '__main__':
    import sys
    d = open(sys.argv[1], 'rb').read(); c = compress(d)
    assert decompress(c, len(d)) == d
    print(len(d), '->', len(c), 'round-trip OK')
