"""Convert private DMX samples to OPL4 RAM waves; no Yamaha ROM is required."""
import struct

SOUNDS = (
    ('PISTOL', 'DSPISTOL'), ('SHOTGUN', 'DSSHOTGN'), ('FIRE', 'DSFIRSHT'),
    ('PAIN', 'DSPLPAIN'), ('DEATH', 'DSPODTH1'), ('IMP_DEATH', 'DSBGDTH1'),
    ('BARREL', 'DSBAREXP'), ('ITEM', 'DSITEMUP'), ('MON_PAIN', 'DSPOPAIN'),
    ('PLAYER_DEATH', 'DSPLDETH'),
)
RAM_BASE = 0x200000
RAM_SIZE = 128 * 1024
RATE = 11025


def sample(lump):
    if len(lump) < 8:
        raise ValueError('Truncated DMX header')
    fmt, rate, count = struct.unpack_from('<HHI', lump)
    if fmt != 3 or rate != RATE or not 48 < count <= len(lump) - 8:
        raise ValueError('Expected DMX unsigned PCM8 at 11025 Hz')
    pcm = bytearray((v ^ 128) for v in lump[24:8 + count - 16])
    # Short endpoint ramps avoid clicks without changing the body of the sample.
    for i in range(16):
        for at in (i, len(pcm) - 1 - i):
            value = pcm[at] if pcm[at] < 128 else pcm[at] - 256
            pcm[at] = int(value * i / 16) & 255
    return bytes(pcm)


def build(wad, first_bank):
    data = bytearray(256)
    for i, (_, lump) in enumerate(SOUNDS):
        pcm = sample(wad.lump(lump))
        length = len(pcm)
        if length + 16 >= 65536:
            raise ValueError('OPL4 wave exceeds 16-bit end offset')
        start = RAM_BASE + len(data)
        end = (65536 - length - 16) & 65535
        header = start.to_bytes(3, 'big') + struct.pack('>HH', length, end)
        header += bytes((0, 0xF0, 0, 0x0F, 0))
        data[i * 12:i * 12 + 12] = header
        data += pcm + bytes(16)  # one-shot ends in a silent hardware loop
    size = len(data)
    if size > RAM_SIZE:
        raise ValueError('Sound set exceeds minimum MoonSound sample RAM')
    data += bytes((-size) % 16384)
    banks = [bytes(data[i:i + 16384]) for i in range(0, len(data), 16384)]
    if first_bank + len(banks) > 256:
        raise ValueError('Sound banks exceed ASCII16 mapper capacity')
    defines = [f'#define SFX_BANK0 {first_bank}', f'#define NSFX_BANKS {len(banks)}',
               f'#define NSFX {len(SOUNDS)}', f'#define SFX_RAM_BYTES {size}']
    defines += [f'#define SFX_{name} {i}' for i, (name, _) in enumerate(SOUNDS)]
    return banks, defines
