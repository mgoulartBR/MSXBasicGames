"""Leitor das bibliotecas LIB1/LIB1C/LIB2/LIB2C do Street Rod (rotina de descompressao em SRSE.EXE:0xE926).

Formato do arquivo (little-endian):
  u16 n_itens
  n_itens x entrada de 30 bytes:
      u16 largura, u16 altura, u16 bytes_descomprimidos, u8 flag,
      u8 tab[16], u8 ?, u32 offset (relativo ao fim do indice), u16 tamanho_comprimido-1
  dados: cada item comeca e termina com 0xBC (o marcador final e compartilhado com o byte inicial do proximo item)

Algoritmo (src = bytes entre os dois 0xBC):
  lut[tab[i]] = i+1
  b == 0x00 ou 0xFF  -> le N; emite N+1 copias de b
  lut[b] != 0        -> emite RUNS[lut[b]-1] (contagem, valor)
  senao              -> byte literal
"""
import struct

# (contagem, valor) das 16 corridas pre-definidas, na ordem do indice 1..16.
# Constantes lidas direto de SRSE.EXE (offset DS:0x501E contagens, DS:0x502E valores).
RUN_COUNTS = [1, 1, 2, 3, 4, 2, 3, 5, 6, 6, 4, 7, 7, 5, 8, 8]
RUN_VALUES = [0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00]


class Item:
    def __init__(self, w, h, size, flag, tab, data):
        self.w, self.h, self.size, self.flag, self.tab, self.data = w, h, size, flag, tab, data

    def decode(self, values=None):
        values = values or RUN_VALUES
        lut = {t: i + 1 for i, t in enumerate(self.tab)}
        out = bytearray()
        d = self.data
        i = 0
        while i < len(d):
            b = d[i]; i += 1
            if b in (0x00, 0xFF):
                out += bytes([b]) * (d[i] + 1); i += 1
            elif b in lut:
                k = lut[b] - 1
                out += bytes([values[k]]) * RUN_COUNTS[k]
            else:
                out.append(b)
        return bytes(out)


def load(path):
    b = open(path, 'rb').read()
    n, = struct.unpack('<H', b[:2])
    base = 2 + 30 * n
    items = []
    for k in range(n):
        e = b[2 + 30 * k: 32 + 30 * k]
        w, h, size = struct.unpack('<HHH', e[:6])
        flag = e[6]
        tab = list(e[7:23])
        off, = struct.unpack('<I', e[24:28])
        ln, = struct.unpack('<H', e[28:30])
        blk = b[base + off: base + off + ln + 1]
        assert blk[0] == 0xBC and blk[-1] == 0xBC
        items.append(Item(w, h, size, flag, tab, blk[1:-1]))
    return items
