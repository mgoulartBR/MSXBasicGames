"""SIMULATED: private conversion and Z80 driver/protocol; not sound quality/timing."""
import os
import re
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import e1m1_sound as S
from wad import Wad


class Conversion(unittest.TestCase):
    def test_reject_bad_headers(self):
        for data in (b'', struct.pack('<HHI', 3, 8000, 64) + bytes(64),
                     struct.pack('<HHI', 3, 11025, 100) + bytes(64)):
            with self.assertRaises(ValueError):
                S.sample(data)

    def test_signed_pcm_and_endpoints(self):
        raw = bytes(16) + bytes((0, 64, 128, 255)) * 32 + bytes(16)
        converted = S.sample(struct.pack('<HHI', 3, S.RATE, len(raw)) + raw)
        self.assertEqual(len(converted), len(raw) - 32)
        self.assertEqual(converted[16:112], bytes((128, 192, 0, 127)) * 24)
        self.assertEqual((converted[0], converted[-1]), (0, 0))

    def test_private_bank_headers(self):
        wad = Wad(os.environ['E1M1_WAD'])
        banks, _ = S.build(wad, 242)
        data = b''.join(banks)
        self.assertLessEqual(len(data), S.RAM_SIZE)
        for i, (_, name) in enumerate(S.SOUNDS):
            header = data[i * 12:i * 12 + 12]
            start = int.from_bytes(header[:3], 'big') - S.RAM_BASE
            loop, end = struct.unpack('>HH', header[3:7])
            pcm = S.sample(wad.lump(name))
            self.assertEqual(loop, len(pcm))
            self.assertEqual(65536 - end, loop + 16)
            self.assertEqual(data[start:start + loop], pcm)
            self.assertEqual(data[start + loop:start + loop + 16], bytes(16))
        _, defines = S.build(wad, 242)
        ticks = list(map(int, defines[-1].split('{')[1].split('}')[0].split(',')))
        for duration, (_, name) in zip(ticks, S.SOUNDS):
            seconds = (len(S.sample(wad.lump(name))) + 16) / S.RATE
            self.assertGreater(duration * 3584 / 3579545, seconds)
            self.assertLess(duration * 3584 / 3579545, seconds + .004)
        with self.assertRaises(ValueError):
            S.build(wad, 255)

    def test_z80_driver(self):
        from e1m1_sim import make, symbols
        m = make()
        sym = symbols()
        regs = bytearray(256)
        ram = bytearray(S.RAM_SIZE)
        state = {'reg': 0, 'addr': 0, 'fmreg': 0, 'new2': False, 'clock': 0}
        events = []
        loads = []
        original_in, original_out = m.inp, m.out

        def inp(port):
            p = port & 255
            if p == 0xC4:
                return 0
            if p == 0xE7:
                return state['clock']
            if p == 0x7F:
                r = state['reg']
                if r == 2:
                    return regs[2] | 0x20
                if r == 6:
                    offset = state['addr'] - S.RAM_BASE
                    value = ram[offset] if 0 <= offset < len(ram) else 255
                    state['addr'] += 1
                    return value
                return regs[r]
            return original_in(port)

        def out(port, value):
            p = port & 255
            if p == 0xC6:
                state['fmreg'] = value
            elif p == 0xC7 and state['fmreg'] == 5:
                state['new2'] = bool(value & 2)
            elif p == 0x7E:
                if state['new2']:
                    state['reg'] = value
            elif p == 0x7F and state['new2']:
                r = state['reg']
                regs[r] = value
                if r == 5:
                    state['addr'] = int.from_bytes(regs[3:6], 'big')
                if r == 6 and regs[2] & 1:
                    offset = state['addr'] - S.RAM_BASE
                    if 0 <= offset < len(ram):
                        ram[offset] = value
                    state['addr'] += 1
                if 0x68 <= r < 0x68 + 24 and value & 128:
                    events.append(r - 0x68)
                if 8 <= r < 16:
                    loads.append((r - 8, value - 128))
            else:
                original_out(port, value)

        m.m.set_input_callback(inp)
        m.m.set_output_callback(out)
        m.run_frames(2)
        self.assertEqual(m.m.memory[sym['audio_moon']], 1)
        self.assertEqual(regs[2], 0x10)
        first = int(re.search(r'SFX_BANK0 (\d+)',
                    Path('private/e1m1_assets.h').read_text()).group(1))
        banks, _ = S.build(Wad(os.environ['E1M1_WAD']), first)
        self.assertEqual(ram[:len(banks) * 16384], b''.join(banks))
        # Keyboard shot triggers both pistol and hit/pain, without changing the bank.
        m.keys_fn = lambda row: 0xFE if row == 8 else 255
        m.run_frames(5)
        self.assertTrue(events)
        self.assertEqual(regs[0x38 + events[0]], 0xF0)
        self.assertEqual(regs[0x50 + events[0]], 41)
        self.assertTrue(all(v < 8 for v in events))
        count = int(re.search(r'NTHING (\d+)',
                    Path('private/e1m1_assets.h').read_text()).group(1))
        for i in range(count):
            m.m.memory[sym['th']+11*i+9] = 0
        # Free-running turbo R timer, including wrap. Real PCM timing is in openMSX.
        m.keys_fn = lambda row: 255
        for i in range(12):
            state['clock'] = (state['clock'] + 180) & 255
            m.run_frames(1)
        self.assertTrue(all(regs[0x68 + i] & 128 == 0 for i in range(8)))
        self.assertEqual(bytes(m.m.memory[sym['audio_left']:sym['audio_left']+16]), bytes(16))
        # Holding fire in an empty room cycles through the eight cached pistol waves.
        m.m.memory[sym['over']] = 0
        events.clear(); loads.clear()
        m.keys_fn = lambda row: 0xFE if row == 8 else 255
        m.run_frames(220)
        self.assertGreater(len(events), 16)
        self.assertLessEqual(len(loads), 8)


if __name__ == '__main__':
    unittest.main()
